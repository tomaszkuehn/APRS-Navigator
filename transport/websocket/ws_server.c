#include "ws_server.h"
#include "../../engine/include/aprs_engine_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define socklen_t int
    #define SSOCKET SOCKET
    #define SCLOSE closesocket
    #define SERR INVALID_SOCKET
    /* Windows: use WinCrypt for SHA1 */
    #include <wincrypt.h>
    #pragma comment(lib, "crypt32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SSOCKET int
    #define SCLOSE close
    #define SERR -1
#endif

#define MAX_CLIENTS       16
#define WS_MAX_FRAME    8192
#define WS_PORT         8081

/* WebSocket opcodes */
#define WS_OP_CONT  0x0
#define WS_OP_TEXT  0x1
#define WS_OP_CLOSE 0x8
#define WS_OP_PING  0x9
#define WS_OP_PONG  0xA

/* Client connection state */
typedef struct {
    SSOCKET fd;
    uint8_t handshake_done;
    uint64_t sub_id;
    void* cb_ctx;  /* callback context for event subscription */
    char rx_buf[WS_MAX_FRAME];
    size_t rx_len;
} ws_client_t;

struct aprs_ws_server {
    int port;
    int running;
    aprs_engine_t* engine;
    SSOCKET listen_fd;
    ws_client_t clients[MAX_CLIENTS];
    int client_count;
    char auth_token[64];  /* Optional auth token for API access */
};

/* ========================================================================
 * BASE64 ENCODING (for WebSocket accept key)
 * ======================================================================== */

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64_encode(const uint8_t* data, size_t len, char* out) {
    size_t i = 0, j = 0;
    while (i < len) {
        uint32_t a = (uint8_t)(i < len ? data[i++] : 0);
        uint32_t b = (uint8_t)(i < len ? data[i++] : 0);
        uint32_t c = (uint8_t)(i < len ? data[i++] : 0);
        uint32_t triple = (a << 16) | (b << 8) | c;
        out[j++] = b64_table[(triple >> 18) & 0x3F];
        out[j++] = b64_table[(triple >> 12) & 0x3F];
        out[j++] = b64_table[(triple >> 6) & 0x3F];
        out[j++] = b64_table[triple & 0x3F];
    }
    /* Padding */
    size_t mod = len % 3;
    if (mod == 1) { out[j-2] = '='; out[j-1] = '='; }
    else if (mod == 2) { out[j-1] = '='; }
    out[j] = '\0';
}

/* ========================================================================
 * SHA1 — minimal verified implementation for WebSocket handshake
 * ======================================================================== */

typedef struct { uint32_t h[5]; uint64_t len; uint8_t buf[64]; int idx; } sha1_ctx;

static uint32_t sha1_rot(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void sha1_block(sha1_ctx* s) {
    uint32_t w[80], a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3], e = s->h[4];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)s->buf[i*4]<<24 | (uint32_t)s->buf[i*4+1]<<16 | (uint32_t)s->buf[i*4+2]<<8 | s->buf[i*4+3];
    for (int i = 16; i < 80; i++)
        w[i] = sha1_rot(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | (~b & d);          k = 0x5A827999; }
        else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
        else             { f = b ^ c ^ d;                   k = 0xCA62C1D6; }
        uint32_t t = sha1_rot(a, 5) + f + e + k + w[i];
        e = d; d = c; c = sha1_rot(b, 30); b = a; a = t;
    }
    s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d; s->h[4] += e;
}

static void sha1_init(sha1_ctx* s) {
    s->h[0] = 0x67452301; s->h[1] = 0xEFCDAB89; s->h[2] = 0x98BADCFE;
    s->h[3] = 0x10325476; s->h[4] = 0xC3D2E1F0; s->len = 0; s->idx = 0;
}

static void sha1_update(sha1_ctx* s, const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        s->buf[s->idx++] = p[i]; s->len++;
        if (s->idx == 64) { sha1_block(s); s->idx = 0; }
    }
}

static void sha1_final(sha1_ctx* s, uint8_t out[20]) {
    uint64_t bits = s->len * 8;
    s->buf[s->idx++] = 0x80;
    if (s->idx > 56) { while (s->idx < 64) s->buf[s->idx++] = 0; sha1_block(s); s->idx = 0; }
    while (s->idx < 56) s->buf[s->idx++] = 0;
    for (int i = 7; i >= 0; i--) s->buf[s->idx++] = (uint8_t)(bits >> (i * 8));
    sha1_block(s);
    for (int i = 0; i < 5; i++) {
        out[i*4]   = (uint8_t)(s->h[i] >> 24); out[i*4+1] = (uint8_t)(s->h[i] >> 16);
        out[i*4+2] = (uint8_t)(s->h[i] >> 8);  out[i*4+3] = (uint8_t)s->h[i];
    }
}

/* ========================================================================
 * WEBSOCKET FRAMING
 * ======================================================================== */

/* Send a WebSocket text frame */
static int ws_send_text(SSOCKET fd, const char* data, size_t len) {
    uint8_t header[10];
    size_t header_len;

    header[0] = 0x81; /* FIN + text opcode */

    if (len < 126) {
        header[1] = (uint8_t)(len);  /* no mask: server→client per RFC 6455 */
        header_len = 2;
    } else if (len < 65536) {
        header[1] = (uint8_t)(126);
        header[2] = (uint8_t)(len >> 8);
        header[3] = (uint8_t)(len);
        header_len = 4;
    } else {
        header[1] = (uint8_t)(127);
        for (int i = 7; i >= 0; i--) header[2+i] = (uint8_t)(len >> (i*8));
        header_len = 10;
    }

    /* Send header + payload directly (no masking server→client) */
    send(fd, (const char*)header, (int)header_len, 0);
    send(fd, data, (int)len, 0);
    return 0;
}

/* ========================================================================
 * JSON HELPERS
 * ======================================================================== */

static void json_escape_ws(char* dst, size_t dst_size, const char* src) {
    size_t pos = 0;
    while (*src && pos < dst_size - 2) {
        switch (*src) {
            case '"':  dst[pos++] = '\\'; dst[pos++] = '"'; break;
            case '\\': dst[pos++] = '\\'; dst[pos++] = '\\'; break;
            case '\n': dst[pos++] = '\\'; dst[pos++] = 'n'; break;
            case '\r': dst[pos++] = '\\'; dst[pos++] = 'r'; break;
            default:
                if ((unsigned char)*src < 0x20) {
                    pos += (size_t)snprintf(dst + pos, dst_size - pos,
                                            "\\u%04X", (unsigned)*src);
                } else {
                    dst[pos++] = *src;
                }
        }
        src++;
    }
    dst[pos] = '\0';
}

/* Send JSON message to client */
static void ws_send_json(SSOCKET fd, const char* json) {
    ws_send_text(fd, json, strlen(json));
}

/* ========================================================================
 * PROTOCOL HANDLING
 * ======================================================================== */

/* Send hello message */
static void ws_send_hello(SSOCKET fd) {
    char msg[512];
    snprintf(msg, sizeof(msg),
        "{"
        "\"type\":\"hello\","
        "\"tsMs\":0,"
        "\"payload\":{"
            "\"engineVersion\":\"%s\","
            "\"protocolVersion\":\"1.0\","
            "\"serverName\":\"aprs-engine\","
            "\"capabilities\":[\"snapshot\",\"events\",\"commands\",\"filters\",\"editor\",\"digi\",\"stats-history\"]"
        "}"
        "}",
        aprs_engine_version()
    );
    ws_send_json(fd, msg);
}

/* Send ack */
static void ws_send_ack(SSOCKET fd, const char* msg_id) {
    char msg[256];
    snprintf(msg, sizeof(msg),
        "{\"type\":\"ack\",\"id\":\"%s\",\"tsMs\":0,\"payload\":{\"status\":\"ok\"}}",
        msg_id);
    ws_send_json(fd, msg);
}

/* Send error */
static void ws_send_error(SSOCKET fd, const char* msg_id, const char* code, const char* message) {
    char msg[512];
    char esc_msg[256];
    json_escape_ws(esc_msg, sizeof(esc_msg), message);
    snprintf(msg, sizeof(msg),
        "{\"type\":\"error\",\"id\":\"%s\",\"tsMs\":0,\"payload\":{\"code\":\"%s\",\"message\":\"%s\"}}",
        msg_id ? msg_id : "", code, esc_msg);
    ws_send_json(fd, msg);
}

/* Send snapshot */
static void ws_send_snapshot(SSOCKET fd, aprs_engine_t* e) {
    size_t len = 4096;
    char* snap = (char*)malloc(len);
    if (!snap) return;
    aprs_engine_get_snapshot(e, snap, &len);
    char msg[4608];
    snprintf(msg, sizeof(msg),
        "{\"type\":\"snapshot\",\"tsMs\":0,\"payload\":%s}", snap);
    ws_send_json(fd, msg);
    free(snap);
}

/* Event callback for WebSocket clients */
typedef struct {
    SSOCKET fd;
} ws_cb_ctx_t;

static void ws_event_callback(const aprs_event_t* ev, void* user) {
    ws_cb_ctx_t* ctx = (ws_cb_ctx_t*)user;
    if (!ctx || ctx->fd == SERR) return;

    /* Build JSON event message */
    char msg[4096];
    char payload_json[3072] = "null";

    /* Serialize known event types to JSON */
    switch (ev->type) {
        case APRS_EVT_FRAME_RX: {
            const aprs_frame_t* f = (const aprs_frame_t*)ev->payload;
            if (f) {
                char esc_raw[1100];
                json_escape_ws(esc_raw, sizeof(esc_raw), f->raw);
                snprintf(payload_json, sizeof(payload_json),
                    "{\"src\":\"%s\",\"dst\":\"%s\",\"path\":\"%s\","
                    "\"raw\":\"%s\",\"isValid\":%s}",
                    f->src, f->dst, f->path, esc_raw,
                    f->is_valid ? "true" : "false");
            }
            break;
        }
        case APRS_EVT_STATION_UPDATED: {
            const aprs_station_t* st = (const aprs_station_t*)ev->payload;
            if (st) {
                snprintf(payload_json, sizeof(payload_json),
                    "{\"callsign\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,"
                    "\"distanceKm\":%.2f,\"bearingDeg\":%.1f,"
                    "\"packetCount\":%u,\"hasPosition\":%s,\"hasWeather\":%s}",
                    st->callsign, st->lat, st->lon,
                    st->distance_km, st->bearing_deg,
                    st->packet_count,
                    st->has_position ? "true" : "false",
                    st->has_weather ? "true" : "false");
            }
            break;
        }
        case APRS_EVT_GPS_UPDATED: {
            const aprs_gps_t* g = (const aprs_gps_t*)ev->payload;
            if (g) {
                snprintf(payload_json, sizeof(payload_json),
                    "{\"fix\":%s,\"valid\":%s,\"lat\":%.6f,\"lon\":%.6f,"
                    "\"speedKmh\":%.1f,\"courseDeg\":%.1f,\"altitudeM\":%.1f}",
                    g->fix ? "true" : "false", g->valid ? "true" : "false",
                    g->lat, g->lon, g->speed_kmh, g->course_deg, g->altitude_m);
            }
            break;
        }
        case APRS_EVT_STATS_UPDATED: {
            const aprs_stats_t* st = (const aprs_stats_t*)ev->payload;
            if (st) {
                snprintf(payload_json, sizeof(payload_json),
                    "{\"rxFrames\":%u,\"txFrames\":%u,\"digiRepeats\":%u,"
                    "\"msgReceived\":%u,\"msgSent\":%u,\"invalidFrames\":%u}",
                    st->rx_frames, st->tx_frames, st->digi_repeats,
                    st->msg_received, st->msg_sent, st->invalid_frames);
            }
            break;
        }
        case APRS_EVT_FRAME_TX:
        case APRS_EVT_BEACON_SENT: {
            const aprs_frame_t* f = (const aprs_frame_t*)ev->payload;
            if (f) {
                char esc_raw[1100];
                json_escape_ws(esc_raw, sizeof(esc_raw), f->raw);
                snprintf(payload_json, sizeof(payload_json),
                    "{\"src\":\"%s\",\"dst\":\"%s\",\"path\":\"%s\",\"raw\":\"%s\"}",
                    f->src, f->dst, f->path, esc_raw);
            }
            break;
        }
        case APRS_EVT_MESSAGE_RECEIVED:
        case APRS_EVT_MESSAGE_SENT:
        case APRS_EVT_MESSAGE_STATE: {
            const aprs_message_t* m = (const aprs_message_t*)ev->payload;
            if (m) {
                char esc_text[520];
                json_escape_ws(esc_text, sizeof(esc_text), m->text);
                snprintf(payload_json, sizeof(payload_json),
                    "{\"id\":\"%s\",\"src\":\"%s\",\"dst\":\"%s\",\"text\":\"%s\",\"state\":%d}",
                    m->id, m->src, m->dst, esc_text, (int)m->state);
            }
            break;
        }
        case APRS_EVT_DIGI_REPEATED:
            if (ev->payload && ev->payload_size > 0)
                snprintf(payload_json, sizeof(payload_json), "{\"frame\":\"%.200s\"}", (const char*)ev->payload);
            break;
        case APRS_EVT_TX_STATE_CHANGED:
            snprintf(payload_json, sizeof(payload_json), "{}"); break;
        case APRS_EVT_POSITION_CHANGED:
            snprintf(payload_json, sizeof(payload_json), "{}"); break;
        case APRS_EVT_POSITION_CAPTURED:
            snprintf(payload_json, sizeof(payload_json), "{}"); break;
        case APRS_EVT_SOUND: {
            const uint8_t* level = (const uint8_t*)ev->payload;
            snprintf(payload_json, sizeof(payload_json), "{\"level\":%u}", level ? *level : 0);
            break;
        }
        case APRS_EVT_CONFIG_CHANGED:
            snprintf(payload_json, sizeof(payload_json), "{}"); break;
        default:
            if (ev->payload && ev->payload_size > 0)
                snprintf(payload_json, sizeof(payload_json), "{\"size\":%zu}", ev->payload_size);
            break;
    }

    snprintf(msg, sizeof(msg),
        "{\"type\":\"event\",\"tsMs\":%llu,\"topic\":\"%s\",\"payload\":%s}",
        (unsigned long long)ev->ts_ms, ev->topic, payload_json);

    ws_send_json(ctx->fd, msg);
}

/* Parse a JSON field from a message */
static int ws_json_get_string(const char* json, const char* key, char* out, size_t out_size) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char* pos = strstr(json, search);
    if (!pos) return 0;
    pos += strlen(search);
    while (*pos == ' ' || *pos == ':' || *pos == '"') pos++;
    size_t i = 0;
    while (*pos && *pos != '"' && i < out_size - 1) {
        if (*pos == '\\' && *(pos+1)) pos++; /* skip escaped */
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 1;
}

/* Handle a JSON message from client */
static void ws_handle_message(aprs_ws_server_t* srv, ws_client_t* client, const char* msg) {
    char type[32] = {0};
    char msg_id[64] = {0};

    ws_json_get_string(msg, "type", type, sizeof(type));
    ws_json_get_string(msg, "id", msg_id, sizeof(msg_id));

    if (strcmp(type, "subscribe") == 0) {
        /* Subscribe to topics */
        /* Extract topics array (simple: look for first string after "topics") */
        char topics[512] = {0};
        const char* topics_start = strstr(msg, "\"topics\"");
        if (topics_start) {
            /* Find array contents */
            const char* bracket = strchr(topics_start, '[');
            if (bracket) {
                const char* end = strchr(bracket, ']');
                if (end) {
                    size_t tlen = (size_t)(end - bracket - 1);
                    if (tlen >= sizeof(topics)) tlen = sizeof(topics) - 1;
                    memcpy(topics, bracket + 1, tlen);
                    topics[tlen] = '\0';
                }
            }
        }

        /* Parse comma-separated topic list */
        if (topics[0]) {
            char* token = strtok(topics, ",");
            while (token) {
                /* Strip quotes and whitespace */
                while (*token == ' ' || *token == '"') token++;
                char* end_q = token + strlen(token) - 1;
                while (end_q > token && (*end_q == ' ' || *end_q == '"')) {
                    *end_q = '\0';
                    end_q--;
                }

                if (token[0]) {
                    /* Create a callback context */
                    ws_cb_ctx_t* cb_ctx = (ws_cb_ctx_t*)malloc(sizeof(ws_cb_ctx_t));
                    if (cb_ctx) {
                        cb_ctx->fd = client->fd;
                        uint64_t sub = aprs_engine_subscribe(srv->engine, token,
                                                              ws_event_callback, cb_ctx);
                        if (sub) {
                            client->sub_id = sub;
                            client->cb_ctx = cb_ctx;  /* Store for cleanup */
                        } else {
                            free(cb_ctx);
                        }
                    }
                }
                token = strtok(NULL, ",");
            }
        }

        ws_send_ack(client->fd, msg_id);
        ws_send_snapshot(client->fd, srv->engine);

    } else if (strcmp(type, "unsubscribe") == 0) {
        if (client->sub_id) {
            aprs_engine_unsubscribe(srv->engine, client->sub_id);
            client->sub_id = 0;
        }
        ws_send_ack(client->fd, msg_id);

    } else if (strcmp(type, "command") == 0) {
        /* Route command by topic */
        char topic[64] = {0};
        ws_json_get_string(msg, "topic", topic, sizeof(topic));

        if (strcmp(topic, "beacon.send") == 0) {
            aprs_engine_send_beacon(srv->engine);
            ws_send_ack(client->fd, msg_id);
        } else if (strcmp(topic, "message.send") == 0) {
            char dest[16] = {0}, text[256] = {0};
            ws_json_get_string(msg, "dest", dest, sizeof(dest));
            ws_json_get_string(msg, "text", text, sizeof(text));
            aprs_engine_send_message(srv->engine, dest, text);
            ws_send_ack(client->fd, msg_id);
        } else if (strcmp(topic, "position.set") == 0) {
            char mode[16] = {0}, lat_s[32] = {0}, lon_s[32] = {0}, alt_s[32] = {0};
            ws_json_get_string(msg, "mode", mode, sizeof(mode));
            ws_json_get_string(msg, "lat", lat_s, sizeof(lat_s));
            ws_json_get_string(msg, "lon", lon_s, sizeof(lon_s));
            ws_json_get_string(msg, "alt", alt_s, sizeof(alt_s));
            aprs_engine_set_manual_position(srv->engine, atof(lat_s), atof(lon_s), atof(alt_s));
            ws_send_ack(client->fd, msg_id);
        } else if (strcmp(topic, "tx.lock") == 0) {
            /* Parse boolean values */
            int dtx = strstr(msg, "\"disableTx\":true") ? 1 : 0;
            int ddigi = strstr(msg, "\"disableDigi\":true") ? 1 : 0;
            int dquery = strstr(msg, "\"disableQuery\":true") ? 1 : 0;
            aprs_engine_lock_tx(srv->engine, (uint8_t)dtx, (uint8_t)ddigi, (uint8_t)dquery);
            ws_send_ack(client->fd, msg_id);
        } else if (strcmp(topic, "frame.edit") == 0) {
            char raw[512] = {0};
            ws_json_get_string(msg, "raw", raw, sizeof(raw));
            aprs_engine_set_editor_frame_text(srv->engine, raw);
            ws_send_ack(client->fd, msg_id);
        } else if (strcmp(topic, "frame.send_editor") == 0) {
            aprs_engine_send_editor_frame(srv->engine);
            ws_send_ack(client->fd, msg_id);
        } else {
            ws_send_error(client->fd, msg_id, "unsupported", "Unknown command topic");
        }

    } else if (strcmp(type, "ping") == 0) {
        /* Respond with pong */
        char pong[128];
        snprintf(pong, sizeof(pong),
            "{\"type\":\"pong\",\"id\":\"%s\",\"tsMs\":0,\"payload\":{}}",
            msg_id);
        ws_send_json(client->fd, pong);

    } else {
        ws_send_error(client->fd, msg_id, "unsupported", "Unknown message type");
    }
}

/* Check if request has valid auth token (if auth is enabled) */
static int check_auth(aprs_ws_server_t* srv, const char* request) {
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
 * WEBSOCKET HANDSHAKE
 * ======================================================================== */

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static int ws_do_handshake(SSOCKET fd, const char* request) {
    /* Extract Sec-WebSocket-Key */
    const char* key_start = strstr(request, "Sec-WebSocket-Key:");
    if (!key_start) return 0;
    key_start += 19;
    while (*key_start == ' ') key_start++;
    const char* key_end = strstr(key_start, "\r\n");
    if (!key_end) return 0;

    char key[256];
    size_t key_len = (size_t)(key_end - key_start);
    if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
    memcpy(key, key_start, key_len);
    key[key_len] = '\0';

    /* Concatenate with GUID */
    char combined[512];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_GUID);

    /* SHA1 hash */
    sha1_ctx ctx; uint8_t digest[20];
    sha1_init(&ctx);
    sha1_update(&ctx, (const uint8_t*)combined, strlen(combined));
    sha1_final(&ctx, digest);

    /* Base64 encode */
    char accept_key[64];
    base64_encode(digest, 20, accept_key);

    /* Send handshake response */
    char response[512];
    int resp_len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key);

    return send(fd, response, resp_len, 0) > 0;
}

/* ========================================================================
 * SERVER
 * ======================================================================== */

aprs_ws_server_t* aprs_ws_server_create(int port, aprs_engine_t* engine) {
    aprs_ws_server_t* srv = (aprs_ws_server_t*)calloc(1, sizeof(*srv));
    if (!srv) return NULL;

    srv->port = port;
    srv->engine = engine;
    srv->running = 0;
    srv->client_count = 0;

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

void aprs_ws_server_stop(aprs_ws_server_t* srv) {
    if (srv) srv->running = 0;
}

void aprs_ws_server_destroy(aprs_ws_server_t* srv) {
    if (!srv) return;
    for (int i = 0; i < srv->client_count; i++) {
        SCLOSE(srv->clients[i].fd);
    }
    SCLOSE(srv->listen_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    free(srv);
}

void aprs_ws_server_set_auth_token(aprs_ws_server_t* srv, const char* token) {
    if (!srv || !token) return;
    strncpy(srv->auth_token, token, sizeof(srv->auth_token) - 1);
    srv->auth_token[sizeof(srv->auth_token) - 1] = '\0';
}

int aprs_ws_server_run(aprs_ws_server_t* srv) {
    if (!srv) return -1;

    srv->running = 1;
    char buf[WS_MAX_FRAME];

    printf("WebSocket server listening on port %d\n", srv->port);

    while (srv->running) {
        /* Use select() to multiplex between listener and clients */
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(srv->listen_fd, &read_fds);
        SSOCKET max_fd = srv->listen_fd;

        for (int i = 0; i < srv->client_count; i++) {
            FD_SET(srv->clients[i].fd, &read_fds);
            if (srv->clients[i].fd > max_fd) max_fd = srv->clients[i].fd;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; /* 100ms timeout for engine ticking */

        int ready = select((int)(max_fd + 1), &read_fds, NULL, NULL, &tv);
        if (ready < 0) {
            if (!srv->running) break;
            continue;
        }

        /* Tick the engine even when no socket activity */
        aprs_engine_tick(srv->engine, 100);

        /* Accept new connections */
        if (FD_ISSET(srv->listen_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            SSOCKET client_fd = accept(srv->listen_fd,
                                        (struct sockaddr*)&client_addr, &client_len);
            if (client_fd != SERR && srv->client_count < MAX_CLIENTS) {
                ws_client_t* cl = &srv->clients[srv->client_count++];
                memset(cl, 0, sizeof(*cl));
                cl->fd = client_fd;
                cl->handshake_done = 0;
            } else if (client_fd != SERR) {
                /* Too many clients */
                const char* resp = "HTTP/1.1 503 Service Unavailable\r\n\r\n";
                send(client_fd, resp, (int)strlen(resp), 0);
                SCLOSE(client_fd);
            }
        }

        /* Handle client data */
        for (int i = 0; i < srv->client_count; i++) {
            ws_client_t* cl = &srv->clients[i];
            if (!FD_ISSET(cl->fd, &read_fds)) continue;

            int n = recv(cl->fd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                /* Client disconnected */
                if (cl->sub_id) {
                    aprs_engine_unsubscribe(srv->engine, cl->sub_id);
                }
                if (cl->cb_ctx) {
                    free(cl->cb_ctx);
                    cl->cb_ctx = NULL;
                }
                SCLOSE(cl->fd);
                /* Remove from list */
                if (i < srv->client_count - 1) {
                    memcpy(&srv->clients[i], &srv->clients[srv->client_count - 1],
                           sizeof(ws_client_t));
                }
                srv->client_count--;
                i--;
                continue;
            }
            buf[n] = '\0';

            if (!cl->handshake_done) {
                /* Check authentication before handshake */
                if (!check_auth(srv, buf)) {
                    const char* resp = "HTTP/1.1 401 Unauthorized\r\n\r\n";
                    send(cl->fd, resp, (int)strlen(resp), 0);
                    SCLOSE(cl->fd);
                    if (i < srv->client_count - 1) {
                        memcpy(&srv->clients[i], &srv->clients[srv->client_count - 1],
                               sizeof(ws_client_t));
                    }
                    srv->client_count--;
                    i--;
                    continue;
                }
                
                /* WebSocket handshake */
                if (ws_do_handshake(cl->fd, buf)) {
                    cl->handshake_done = 1;
                    printf("WebSocket client connected\n");
                    ws_send_hello(cl->fd);
                } else {
                    SCLOSE(cl->fd);
                    if (i < srv->client_count - 1) {
                        memcpy(&srv->clients[i], &srv->clients[srv->client_count - 1],
                               sizeof(ws_client_t));
                    }
                    srv->client_count--;
                    i--;
                }
            } else {
                /* Parse WebSocket frame */
                uint8_t* frame = (uint8_t*)buf;
                size_t data_len = (size_t)n;
                size_t pos = 0;

                while (pos + 2 <= data_len) {
                    uint8_t opcode = frame[pos] & 0x0F;
                    uint8_t masked = (frame[pos + 1] >> 7) & 1;
                    uint64_t payload_len = frame[pos + 1] & 0x7F;

                    size_t header_len = 2;
                    if (payload_len == 126) {
                        if (pos + 4 > data_len) break;
                        payload_len = ((uint64_t)frame[pos+2] << 8) | frame[pos+3];
                        header_len = 4;
                    } else if (payload_len == 127) {
                        if (pos + 10 > data_len) break;
                        payload_len = 0;
                        for (int j = 0; j < 8; j++) {
                            payload_len = (payload_len << 8) | frame[pos+2+j];
                        }
                        header_len = 10;
                    }

                    uint8_t mask_key[4] = {0};
                    if (masked) {
                        if (pos + header_len + 4 > data_len) break;
                        memcpy(mask_key, frame + pos + header_len, 4);
                        header_len += 4;
                    }

                    if (pos + header_len + payload_len > data_len) break;

                    uint8_t* payload = frame + pos + header_len;
                    /* Unmask */
                    if (masked) {
                        for (uint64_t j = 0; j < payload_len; j++) {
                            payload[j] ^= mask_key[j % 4];
                        }
                    }

                    /* Handle frame */
                    switch (opcode) {
                        case WS_OP_TEXT: {
                            char* text = (char*)malloc((size_t)(payload_len + 1));
                            if (text) {
                                memcpy(text, payload, (size_t)payload_len);
                                text[payload_len] = '\0';
                                ws_handle_message(srv, cl, text);
                                free(text);
                            }
                            break;
                        }
                        case WS_OP_CLOSE:
                            SCLOSE(cl->fd);
                            if (i < srv->client_count - 1) {
                                memcpy(&srv->clients[i], &srv->clients[srv->client_count - 1],
                                       sizeof(ws_client_t));
                            }
                            srv->client_count--;
                            i--;
                            goto next_client;
                        case WS_OP_PING: {
                            /* Send pong */
                            uint8_t pong_header[2] = { 0x8A, (uint8_t)payload_len };
                            send(cl->fd, (const char*)pong_header, 2, 0);
                            if (payload_len > 0) {
                                send(cl->fd, (const char*)payload, (int)payload_len, 0);
                            }
                            break;
                        }
                    }

                    pos += header_len + (size_t)payload_len;
                }
            }
            next_client:;
        }
    }

    return 0;
}
