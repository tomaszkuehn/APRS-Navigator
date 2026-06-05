#include "http_server.h"
#include "../../engine/include/aprs_engine_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define socklen_t int
    #define SSOCKET SOCKET
    #define SCLOSE closesocket
    #define SERR INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SSOCKET int
    #define SCLOSE close
    #define SERR -1
#endif

#define MAX_REQUEST_SIZE  8192
#define MAX_RESPONSE_SIZE 32768
#define MAX_PATH_LEN       256
#define HTTP_PORT           8080

struct aprs_http_server {
    int port;
    int running;
    aprs_engine_t* engine;
#ifdef _WIN32
    SOCKET listen_fd;
#else
    int listen_fd;
#endif
    char auth_token[64];  /* Optional auth token for API access */
};

/* Minimal URL decode */
static void url_decode(char* str) {
    char* src = str;
    char* dst = str;
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], '\0' };
            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/* Minimal JSON string escape */
static void json_escape(char* dst, size_t dst_size, const char* src) {
    size_t pos = 0;
    while (*src && pos < dst_size - 2) {
        switch (*src) {
            case '"':  dst[pos++] = '\\'; dst[pos++] = '"'; break;
            case '\\': dst[pos++] = '\\'; dst[pos++] = '\\'; break;
            case '\n': dst[pos++] = '\\'; dst[pos++] = 'n'; break;
            case '\r': dst[pos++] = '\\'; dst[pos++] = 'r'; break;
            case '\t': dst[pos++] = '\\'; dst[pos++] = 't'; break;
            default:
                if ((unsigned char)*src < 0x20) {
                    pos += (size_t)snprintf(dst + pos, dst_size - pos, "\\u%04X", (unsigned)*src);
                } else {
                    dst[pos++] = *src;
                }
        }
        src++;
    }
    dst[pos] = '\0';
}

/* Parse query string for a parameter value. Returns NULL if not found. */
static const char* get_query_param(const char* query, const char* key) {
    if (!query) return NULL;
    static char value[128];
    const char* p = query;
    size_t key_len = strlen(key);

    while (*p) {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;
            size_t i = 0;
            while (*p && *p != '&' && i < sizeof(value) - 1) {
                if (*p == '+') value[i++] = ' ';
                else if (*p == '%' && isxdigit((unsigned char)p[1])) {
                    char hex[3] = { p[1], p[2], '\0' };
                    value[i++] = (char)strtol(hex, NULL, 16);
                    p += 2;
                } else {
                    value[i++] = *p;
                }
                p++;
            }
            value[i] = '\0';
            return value;
        }
        /* Skip to next & */
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }
    return NULL;
}

/* Read JSON body from request. Simple: body starts after \r\n\r\n */
static const char* get_json_body(const char* request) {
    const char* body = strstr(request, "\r\n\r\n");
    return body ? body + 4 : NULL;
}

/* Check if request has valid auth token (if auth is enabled) */
static int check_auth(aprs_http_server_t* srv, const char* request) {
    if (srv->auth_token[0] == '\0') return 1;  /* No auth required */
    
    const char* auth_header = strstr(request, "Authorization:");
    if (!auth_header) return 0;
    
    auth_header += 14;  /* Skip "Authorization:" */
    while (*auth_header == ' ') auth_header++;
    
    /* Expect "Bearer <token>" */
    if (strncmp(auth_header, "Bearer ", 7) != 0) return 0;
    auth_header += 7;
    
    return strcmp(auth_header, srv->auth_token) == 0;
}

/* ========================================================================
 * ROUTE HANDLERS
 * ======================================================================== */

static void handle_get_state(aprs_engine_t* e, char* response, size_t* resp_len) {
    size_t len = *resp_len;
    aprs_engine_get_snapshot(e, response, &len);
    *resp_len = len;
}

static void handle_get_stations(aprs_engine_t* e, char* response, size_t* resp_len,
                                 const char* query) {
    aprs_station_t sts[50];
    size_t count = 50;

    /* Check for sort parameter */
    const char* sort_col = get_query_param(query, "sort");
    const char* order_str = get_query_param(query, "order");
    const char* count_str = get_query_param(query, "count");

    if (sort_col) {
        uint8_t asc = 1;
        if (order_str && strcmp(order_str, "desc") == 0) asc = 0;
        aprs_engine_sort_stations(e, sort_col, asc);
    }
    if (count_str) {
        size_t n = (size_t)atoi(count_str);
        if (n > 0 && n < 151) count = n;
    }

    aprs_engine_list_stations(e, sts, &count);

    char* p = response;
    size_t remaining = *resp_len;

    int wrote = snprintf(p, remaining, "{\"stations\":[");
    p += wrote; remaining -= (size_t)wrote;

    for (size_t i = 0; i < count; i++) {
        char esc_callsign[32], esc_info[260], esc_comment[260];
        json_escape(esc_callsign, sizeof(esc_callsign), sts[i].callsign);
        json_escape(esc_info, sizeof(esc_info), sts[i].info);
        json_escape(esc_comment, sizeof(esc_comment), sts[i].comment);

        wrote = snprintf(p, remaining,
            "%s{\"callsign\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,"
            "\"distanceKm\":%.2f,\"bearingDeg\":%.1f,"
            "\"packetCount\":%u,\"lastHeardMs\":%llu,"
            "\"hasPosition\":%s,\"hasWeather\":%s,\"marked\":%s}",
            (i > 0 ? "," : ""),
            esc_callsign, sts[i].lat, sts[i].lon,
            sts[i].distance_km, sts[i].bearing_deg,
            sts[i].packet_count, (unsigned long long)sts[i].last_heard_ms,
            sts[i].has_position ? "true" : "false",
            sts[i].has_weather ? "true" : "false",
            sts[i].marked ? "true" : "false"
        );
        p += wrote;
        if (remaining > (size_t)wrote) remaining -= (size_t)wrote; else remaining = 0;
    }

    wrote = snprintf(p, remaining, "]}");
    p += wrote;
    *resp_len = (size_t)(p - response);
}

static void handle_get_station(aprs_engine_t* e, const char* callsign,
                                char* response, size_t* resp_len) {
    aprs_station_t st;
    if (aprs_engine_get_station(e, callsign, &st) == APRS_OK) {
        char esc_callsign[32], esc_path[130], esc_info[260], esc_comment[260];
        json_escape(esc_callsign, sizeof(esc_callsign), st.callsign);
        json_escape(esc_path, sizeof(esc_path), st.last_path);
        json_escape(esc_info, sizeof(esc_info), st.info);
        json_escape(esc_comment, sizeof(esc_comment), st.comment);

        *resp_len = (size_t)snprintf(response, *resp_len,
            "{\"callsign\":\"%s\",\"lastPath\":\"%s\","
            "\"symbolTable\":\"%c\",\"symbolCode\":\"%c\","
            "\"lat\":%.6f,\"lon\":%.6f,"
            "\"distanceKm\":%.2f,\"bearingDeg\":%.1f,"
            "\"packetCount\":%u,\"hasPosition\":%s,\"hasWeather\":%s,"
            "\"info\":\"%s\",\"comment\":\"%s\"}",
            esc_callsign, esc_path,
            st.symbol_table, st.symbol_code,
            st.lat, st.lon,
            st.distance_km, st.bearing_deg,
            st.packet_count,
            st.has_position ? "true" : "false",
            st.has_weather ? "true" : "false",
            esc_info, esc_comment
        );
    } else {
        *resp_len = (size_t)snprintf(response, *resp_len,
            "{\"error\":\"not_found\",\"message\":\"Station not found\"}");
    }
}

static void handle_get_messages(aprs_engine_t* e, char* response, size_t* resp_len) {
    aprs_message_t msgs[28];
    size_t count = 28;
    aprs_engine_list_messages(e, msgs, &count);

    char* p = response;
    size_t remaining = *resp_len;
    int wrote = snprintf(p, remaining, "{\"messages\":[");
    p += wrote; remaining -= (size_t)wrote;

    for (size_t i = 0; i < count; i++) {
        char esc_text[520];
        json_escape(esc_text, sizeof(esc_text), msgs[i].text);
        wrote = snprintf(p, remaining,
            "%s{\"id\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\","
            "\"text\":\"%s\",\"state\":%d,\"tsMs\":%llu}",
            (i > 0 ? "," : ""),
            msgs[i].id, msgs[i].src, msgs[i].dst,
            esc_text, (int)msgs[i].state, (unsigned long long)msgs[i].ts_ms
        );
        p += wrote;
        if (remaining > (size_t)wrote) remaining -= (size_t)wrote; else remaining = 0;
    }
    wrote = snprintf(p, remaining, "]}");
    p += wrote;
    *resp_len = (size_t)(p - response);
}

static void handle_get_stats(aprs_engine_t* e, char* response, size_t* resp_len) {
    aprs_stats_t st;
    aprs_engine_get_stats(e, &st);
    *resp_len = (size_t)snprintf(response, *resp_len,
        "{\"uptimeMs\":%llu,\"rxFrames\":%u,\"txFrames\":%u,"
        "\"digiRepeats\":%u,\"msgReceived\":%u,\"msgSent\":%u,"
        "\"invalidFrames\":%u,\"uniqueFrames\":%u,"
        "\"txEnabled\":%s,\"digiEnabled\":%s,\"beaconEnabled\":%s}",
        (unsigned long long)st.uptime_ms, st.rx_frames, st.tx_frames,
        st.digi_repeats, st.msg_received, st.msg_sent,
        st.invalid_frames, st.unique_frames,
        st.tx_enabled ? "true" : "false",
        st.digi_enabled ? "true" : "false",
        st.beacon_enabled ? "true" : "false"
    );
}

static void handle_get_gps(aprs_engine_t* e, char* response, size_t* resp_len) {
    aprs_gps_t gps;
    aprs_engine_get_gps(e, &gps);
    *resp_len = (size_t)snprintf(response, *resp_len,
        "{\"fix\":%s,\"valid\":%s,\"lat\":%.6f,\"lon\":%.6f,"
        "\"speedKmh\":%.1f,\"courseDeg\":%.1f,\"altitudeM\":%.1f,\"tsMs\":%llu}",
        gps.fix ? "true" : "false", gps.valid ? "true" : "false",
        gps.lat, gps.lon, gps.speed_kmh, gps.course_deg, gps.altitude_m,
        (unsigned long long)gps.ts_ms
    );
}

/* Simple JSON field extractor (no library dependency) */
static int json_get_string(const char* json, const char* key, char* out, size_t out_size) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char* pos = strstr(json, search);
    if (!pos) return 0;
    pos += strlen(search);
    while (*pos == ' ' || *pos == ':' || *pos == '"') pos++;
    size_t i = 0;
    while (*pos && *pos != '"' && i < out_size - 1) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 1;
}

static int json_get_double(const char* json, const char* key, double* out) {
    char buf[32];
    if (!json_get_string(json, key, buf, sizeof(buf))) return 0;
    *out = atof(buf);
    return 1;
}

static int json_get_int(const char* json, const char* key, int* out) {
    char buf[32];
    if (!json_get_string(json, key, buf, sizeof(buf))) {
        /* Try direct numeric value */
        char search[64];
        snprintf(search, sizeof(search), "\"%s\"", key);
        const char* pos = strstr(json, search);
        if (!pos) return 0;
        pos += strlen(search);
        while (*pos == ' ' || *pos == ':') pos++;
        *out = atoi(pos);
        return 1;
    }
    *out = atoi(buf);
    return 1;
}

/* ========================================================================
 * HTTP SERVER
 * ======================================================================== */

aprs_http_server_t* aprs_http_server_create(int port, aprs_engine_t* engine) {
    aprs_http_server_t* srv = (aprs_http_server_t*)calloc(1, sizeof(*srv));
    if (!srv) return NULL;

    srv->port = port;
    srv->engine = engine;
    srv->running = 0;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        free(srv);
        return NULL;
    }
#endif

    srv->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->listen_fd == SERR) {
        free(srv);
        return NULL;
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(srv->listen_fd, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(srv->listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        SCLOSE(srv->listen_fd);
        free(srv);
        return NULL;
    }

    if (listen(srv->listen_fd, 10) < 0) {
        SCLOSE(srv->listen_fd);
        free(srv);
        return NULL;
    }

    return srv;
}

void aprs_http_server_stop(aprs_http_server_t* srv) {
    if (srv) srv->running = 0;
}

void aprs_http_server_destroy(aprs_http_server_t* srv) {
    if (!srv) return;
    SCLOSE(srv->listen_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    free(srv);
}

void aprs_http_server_set_auth_token(aprs_http_server_t* srv, const char* token) {
    if (!srv || !token) return;
    strncpy(srv->auth_token, token, sizeof(srv->auth_token) - 1);
    srv->auth_token[sizeof(srv->auth_token) - 1] = '\0';
}

int aprs_http_server_run(aprs_http_server_t* srv) {
    if (!srv) return -1;

    srv->running = 1;
    char request[MAX_REQUEST_SIZE];
    char response[MAX_RESPONSE_SIZE];

    printf("HTTP server listening on port %d\n", srv->port);

    while (srv->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SSOCKET client_fd = accept(srv->listen_fd,
                                    (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == SERR) {
            if (!srv->running) break;
            continue;
        }

        /* Read request */
        int n = recv(client_fd, request, sizeof(request) - 1, 0);
        if (n <= 0) {
            SCLOSE(client_fd);
            continue;
        }
        request[n] = '\0';

        /* Parse method and path */
        char method[8] = {0};
        char path[MAX_PATH_LEN] = {0};
        sscanf(request, "%7s %255s", method, path);
        url_decode(path);

        /* Extract query string */
        char* query = strchr(path, '?');
        if (query) {
            *query = '\0';
            query++;
        }

        const char* body = NULL;
        if (strcmp(method, "POST") == 0) {
            body = get_json_body(request);
        }

        size_t resp_len = sizeof(response) - 1;
        int status_code = 200;
        const char* content_type = "application/json";

        /* Check authentication */
        if (!check_auth(srv, request)) {
            resp_len = (size_t)snprintf(response, sizeof(response),
                "{\"error\":\"unauthorized\",\"message\":\"Invalid or missing authentication token\"}");
            status_code = 401;
            content_type = "application/json";

            /* Build HTTP response */
            char http_resp[MAX_RESPONSE_SIZE + 512];
            const char* status = "401 Unauthorized";
            int resp_n = snprintf(http_resp, sizeof(http_resp),
                "HTTP/1.1 %s\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %zu\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s",
                status, content_type, resp_len, response);

            send(client_fd, http_resp, resp_n, 0);
            SCLOSE(client_fd);
            continue;
        }

        /* Route requests */
        if (strcmp(method, "GET") == 0) {
            if (strcmp(path, "/v2/state") == 0) {
                handle_get_state(srv->engine, response, &resp_len);
            } else if (strcmp(path, "/v2/stations") == 0) {
                handle_get_stations(srv->engine, response, &resp_len, query);
            } else if (strncmp(path, "/v2/stations/", 13) == 0) {
                handle_get_station(srv->engine, path + 13, response, &resp_len);
            } else if (strcmp(path, "/v2/messages") == 0) {
                handle_get_messages(srv->engine, response, &resp_len);
            } else if (strcmp(path, "/v2/stats") == 0) {
                handle_get_stats(srv->engine, response, &resp_len);
            } else if (strcmp(path, "/v2/stats/hourly") == 0) {
                uint32_t buckets[60];
                size_t count = 60;
                aprs_engine_get_hourly_stats(srv->engine, buckets, &count);
                char* p = response;
                int w = snprintf(p, sizeof(response), "{\"buckets\":[");
                p += w;
                for (size_t i = 0; i < count; i++) {
                    p += snprintf(p, sizeof(response) - (size_t)(p - response),
                                  "%s%u", (i > 0 ? "," : ""), buckets[i]);
                }
                w = snprintf(p, sizeof(response) - (size_t)(p - response), "]}");
                resp_len = (size_t)(p + w - response);
            } else if (strcmp(path, "/v2/stats/20h") == 0) {
                uint32_t buckets[120];
                size_t count = 120;
                aprs_engine_get_20h_stats(srv->engine, buckets, &count);
                char* p = response;
                int w = snprintf(p, sizeof(response), "{\"buckets\":[");
                p += w;
                for (size_t i = 0; i < count; i++) {
                    p += snprintf(p, sizeof(response) - (size_t)(p - response),
                                  "%s%u", (i > 0 ? "," : ""), buckets[i]);
                }
                w = snprintf(p, sizeof(response) - (size_t)(p - response), "]}");
                resp_len = (size_t)(p + w - response);
            } else if (strcmp(path, "/v2/gps") == 0) {
                handle_get_gps(srv->engine, response, &resp_len);
            } else if (strcmp(path, "/v2/digi/recent") == 0) {
                aprs_engine_get_recent_digi(srv->engine, response, sizeof(response));
                resp_len = strlen(response);
            } else if (strcmp(path, "/v2/frame/editor") == 0) {
                aprs_frame_t ef;
                aprs_engine_get_editor_frame(srv->engine, &ef);
                char escaped[1030];
                json_escape(escaped, sizeof(escaped), ef.raw);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"raw\":\"%s\",\"rawLen\":%zu}", escaped, ef.raw_len);
            } else {
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"error\":\"not_found\"}");
                status_code = 404;
            }
        } else if (strcmp(method, "POST") == 0) {
            /* POST endpoints */
            if (strcmp(path, "/v2/beacon/send") == 0) {
                aprs_engine_send_beacon(srv->engine);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/messages/send") == 0 && body) {
                char dest[16] = {0}, text[256] = {0};
                json_get_string(body, "dest", dest, sizeof(dest));
                json_get_string(body, "text", text, sizeof(text));
                if (dest[0] && text[0]) {
                    aprs_engine_send_message(srv->engine, dest, text);
                    resp_len = (size_t)snprintf(response, sizeof(response),
                        "{\"status\":\"ok\"}");
                } else {
                    resp_len = (size_t)snprintf(response, sizeof(response),
                        "{\"error\":\"invalid_argument\"}");
                    status_code = 400;
                }
            } else if (strcmp(path, "/v2/position/set") == 0 && body) {
                double lat, lon, alt = 0;
                if (json_get_double(body, "lat", &lat) &&
                    json_get_double(body, "lon", &lon)) {
                    json_get_double(body, "alt", &alt);
                    aprs_engine_set_manual_position(srv->engine, lat, lon, alt);
                    resp_len = (size_t)snprintf(response, sizeof(response),
                        "{\"status\":\"ok\"}");
                } else {
                    status_code = 400;
                    resp_len = (size_t)snprintf(response, sizeof(response),
                        "{\"error\":\"invalid_argument\"}");
                }
            } else if (strcmp(path, "/v2/position/capture") == 0 && body) {
                char callsign[16] = {0};
                json_get_string(body, "callsign", callsign, sizeof(callsign));
                aprs_engine_capture_station_position(srv->engine, callsign);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/position/select") == 0 && body) {
                int slot = 0;
                json_get_int(body, "slot", &slot);
                aprs_engine_select_position_slot(srv->engine, slot);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/tx/lock") == 0 && body) {
                int dtx = 0, ddigi = 0, dquery = 0;
                json_get_int(body, "disableTx", &dtx);
                json_get_int(body, "disableDigi", &ddigi);
                json_get_int(body, "disableQuery", &dquery);
                aprs_engine_lock_tx(srv->engine, (uint8_t)dtx, (uint8_t)ddigi, (uint8_t)dquery);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/frame/send") == 0) {
                aprs_engine_send_editor_frame(srv->engine);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/frame/edit") == 0 && body) {
                char raw[512] = {0};
                json_get_string(body, "raw", raw, sizeof(raw));
                aprs_engine_set_editor_frame_text(srv->engine, raw);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/config/reset") == 0) {
                aprs_engine_factory_reset(srv->engine);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/gps/nmea") == 0 && body) {
                char sentence[128] = {0};
                json_get_string(body, "sentence", sentence, sizeof(sentence));
                aprs_engine_push_nmea(srv->engine, sentence);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"status\":\"ok\"}");
            } else if (strcmp(path, "/v2/remote/keys") == 0) {
                char keys[4][16];
                aprs_engine_generate_remote_keys(srv->engine, keys);
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"keys\":[\"%s\",\"%s\",\"%s\",\"%s\"]}",
                    keys[0], keys[1], keys[2], keys[3]);
            } else {
                resp_len = (size_t)snprintf(response, sizeof(response),
                    "{\"error\":\"not_found\"}");
                status_code = 404;
            }
        } else {
            resp_len = (size_t)snprintf(response, sizeof(response),
                "{\"error\":\"method_not_allowed\"}");
            status_code = 405;
        }

        /* Build HTTP response */
        char http_resp[MAX_RESPONSE_SIZE + 512];
        const char* status = (status_code == 200) ? "200 OK" :
                             (status_code == 400) ? "400 Bad Request" :
                             (status_code == 404) ? "404 Not Found" :
                             "405 Method Not Allowed";
        int resp_n = snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            status, content_type, resp_len, response);

        send(client_fd, http_resp, resp_n, 0);
        SCLOSE(client_fd);
    }

    return 0;
}
