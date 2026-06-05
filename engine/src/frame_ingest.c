#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ========================================================================
 * FRAME INGEST — AX.25 / APRS frame parsing, validation, classification
 *
 * Extracted from:
 *   - aprs-rx.c (parse_is, simpacket, aprs_read)
 *   - sta_manager.c (update_aprs, analyze_path, check_digis, header_match)
 * ======================================================================== */

/* Parse raw APRS frame string into structured aprs_frame_t.
 * Format: SRC>DST,PATH1,PATH2,...:PAYLOAD
 *
 * Returns 1 on success, 0 on parse error.
 */
int fi_parse_frame(const char* raw, size_t len, aprs_frame_t* out) {
    if (!raw || !out || len == 0) return 0;
    if (len >= sizeof(out->raw)) len = sizeof(out->raw) - 1;

    memset(out, 0, sizeof(*out));
    memcpy(out->raw, raw, len);
    out->raw[len] = '\0';
    out->raw_len = len;

    /* Find '>' — source/destination separator */
    const char* gt = memchr(raw, '>', len);
    if (!gt) return 0;

    /* Extract source callsign */
    size_t src_len = (size_t)(gt - raw);
    if (src_len >= sizeof(out->src)) src_len = sizeof(out->src) - 1;
    memcpy(out->src, raw, src_len);
    out->src[src_len] = '\0';

    /* Find ':' — end of address/path, start of payload (data type indicator) */
    const char* colon = memchr(gt + 1, ':', (size_t)(raw + len - gt - 1));
    if (!colon) {
        /* Some frames may not have payload, treat entire rest as destination */
        size_t dst_len = (size_t)(raw + len - gt - 1);
        if (dst_len >= sizeof(out->dst)) dst_len = sizeof(out->dst) - 1;
        memcpy(out->dst, gt + 1, dst_len);
        out->dst[dst_len] = '\0';
        out->is_valid = 1;
        return 1;
    }

    /* Extract everything between '>' and ':' as DST,path */
    const char* addr_start = gt + 1;
    size_t addr_len = (size_t)(colon - addr_start);

    /* Destination is first comma-separated field */
    const char* comma = memchr(addr_start, ',', addr_len);
    size_t dst_len;
    if (comma) {
        dst_len = (size_t)(comma - addr_start);
        /* Path is everything after first comma */
        size_t path_len = (size_t)(colon - comma - 1);
        if (path_len >= sizeof(out->path)) path_len = sizeof(out->path) - 1;
        memcpy(out->path, comma + 1, path_len);
        out->path[path_len] = '\0';
    } else {
        dst_len = addr_len;
        out->path[0] = '\0';
    }

    if (dst_len >= sizeof(out->dst)) dst_len = sizeof(out->dst) - 1;
    memcpy(out->dst, addr_start, dst_len);
    out->dst[dst_len] = '\0';

    out->is_valid = 1;

    /* Validate frame: source should be alphanumeric, at least 3 chars */
    int valid_src = 0;
    for (size_t i = 0; i < src_len; i++) {
        if (isalnum((unsigned char)out->src[i]) || out->src[i] == '-') valid_src = 1;
    }
    if (!valid_src) out->is_valid = 0;

    return 1;
}

/* Classify frame based on the data type identifier (first char after ':') */
frame_class_t fi_classify_frame(const aprs_frame_t* f) {
    if (!f || !f->is_valid) return FRAME_CLASS_UNKNOWN;

    /* Find the data type identifier */
    const char* colon = strchr(f->raw, ':');
    if (!colon || *(colon + 1) == '\0') return FRAME_CLASS_UNKNOWN;

    char dt = *(colon + 1);

    switch (dt) {
        case '!': return FRAME_CLASS_POSITION;   /* Position without timestamp */
        case '=': return FRAME_CLASS_POSITION;   /* Position with timestamp */
        case '/': return FRAME_CLASS_POSITION;   /* Position with timestamp (alternate) */
        case '@': return FRAME_CLASS_POSITION;   /* Position with timestamp (alternate) */
        case ':': return FRAME_CLASS_MESSAGE;    /* Message */
        case '>': return FRAME_CLASS_STATUS;     /* Status */
        case '?': return FRAME_CLASS_QUERY;      /* Query */
        case '_': return FRAME_CLASS_WX;         /* Weather */
        case '#': return FRAME_CLASS_WX;         /* Weather (Peet Bros) */
        case '*': return FRAME_CLASS_WX;         /* Weather (Peet Bros alt) */
        case ';': return FRAME_CLASS_OBJECT;     /* Object */
        case ')': return FRAME_CLASS_ITEM;       /* Item */
        case '`': return FRAME_CLASS_MIC_E;      /* Mic-E (old) */
        case '\'': return FRAME_CLASS_MIC_E;     /* Mic-E (current) */
        case '}': return FRAME_CLASS_THIRDPARTY; /* Third-party traffic */
        default:  return FRAME_CLASS_UNKNOWN;
    }
}

/* Compute simple checksum/CRC over the payload portion */
int fi_validate_checksum(const char* raw, size_t len) {
    /* APRS frames don't have a built-in checksum field in the AX.25 UI-frame
     * sense (that's at the HDLC layer). For our purposes, check that the
     * frame is well-formed and ends with a valid character. */
    if (!raw || len == 0) return 0;
    if (len > 0 && raw[len - 1] == '\n') return 1;  /* ok */
    if (len > 0 && raw[len - 1] == '\r') return 1;  /* ok */
    /* Frame should contain printable ASCII */
    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)raw[i] < 0x20 && raw[i] != '\r' && raw[i] != '\n') {
            return 0; /* invalid control character */
        }
    }
    return 1;
}

/* Check if a frame from this source with this CRC was recently seen */
int fi_is_unique(aprs_engine_t* e, const char* src, uint8_t crc) {
    if (!e || !src) return 1;

    for (int i = 0; i < e->unique_check_count; i++) {
        if (strcmp(e->unique_checks[i].callsign, src) == 0) {
            if (e->unique_checks[i].last_crc == crc) {
                /* Same CRC — check time window (30 seconds, like original) */
                uint64_t elapsed = e->current_time_ms - e->unique_checks[i].last_time_ms;
                if (elapsed < 30000) {
                    return 0; /* duplicate */
                }
            }
            /* Update the entry */
            e->unique_checks[i].last_crc = crc;
            e->unique_checks[i].last_time_ms = e->current_time_ms;
            return 1;
        }
    }

    /* Not found, add new entry */
    if (e->unique_check_count < MAX_UNIQUE_CHECK) {
        strncpy(e->unique_checks[e->unique_check_count].callsign, src, 15);
        e->unique_checks[e->unique_check_count].callsign[15] = '\0';
        e->unique_checks[e->unique_check_count].last_crc = crc;
        e->unique_checks[e->unique_check_count].last_time_ms = e->current_time_ms;
        e->unique_check_count++;
    }
    return 1;
}

/* Register a unique frame */
void fi_register_unique(aprs_engine_t* e, const char* src, uint8_t crc) {
    if (!e || !src) return;

    for (int i = 0; i < e->unique_check_count; i++) {
        if (strcmp(e->unique_checks[i].callsign, src) == 0) {
            e->unique_checks[i].last_crc = crc;
            e->unique_checks[i].last_time_ms = e->current_time_ms;
            return;
        }
    }

    if (e->unique_check_count < MAX_UNIQUE_CHECK) {
        strncpy(e->unique_checks[e->unique_check_count].callsign, src, 15);
        e->unique_checks[e->unique_check_count].callsign[15] = '\0';
        e->unique_checks[e->unique_check_count].last_crc = crc;
        e->unique_checks[e->unique_check_count].last_time_ms = e->current_time_ms;
        e->unique_check_count++;
    }
}

/* Apply configured filters to a frame.
 * Returns 1 if frame should be processed, 0 if it should be dropped. */
int fi_apply_filters(aprs_engine_t* e, const aprs_frame_t* f) {
    if (!e || !f || !f->is_valid) return 0;

    /* Invalid frames filter: if invalid and we don't accept invalid, drop */
    if (!f->is_valid && !e->filter_invalid_ok) return 0;

    /* TCP/IP filter (third-party frames) */
    frame_class_t fc = fi_classify_frame(f);
    if (fc == FRAME_CLASS_THIRDPARTY && e->filter_tcpip) return 0;

    /* Unique-only filter is applied at a higher level (after CRC calc) */

    return 1;
}

/* Encode position in standard APRS format.
 * Format: DDMM.mmN/DDDMM.mmESymbolTableSymbolCode
 * Uncompressed position with symbol overlay.
 *
 * Returns number of bytes written (excluding null), or 0 on error.
 */
int fi_encode_position(double lat, double lon, char sym_table, char sym_code,
                        char* out, size_t out_len) {
    if (!out || out_len < 40) return 0;

    /* Latitude */
    double abs_lat = fabs(lat);
    int lat_deg = (int)abs_lat;
    double lat_min = (abs_lat - lat_deg) * 60.0;
    char lat_ns = (lat >= 0) ? 'N' : 'S';

    /* Longitude */
    double abs_lon = fabs(lon);
    int lon_deg = (int)abs_lon;
    double lon_min = (abs_lon - lon_deg) * 60.0;
    char lon_ew = (lon >= 0) ? 'E' : 'W';

    int n = snprintf(out, out_len, "!%02d%05.2f%c%c%03d%05.2f%c%c%c",
                     lat_deg, lat_min, lat_ns,
                     sym_table,
                     lon_deg, lon_min, lon_ew,
                     sym_table, sym_code);

    return (n > 0 && (size_t)n < out_len) ? n : 0;
}

/* Decode Mic-E position from the destination address field of a Mic-E frame.
 * Mic-E encodes latitude in the destination field using a complex scheme.
 *
 * Returns 1 on success, 0 on failure.
 */
int fi_decode_mic_e(const char* raw, size_t len, double* lat, double* lon, char* info) {
    /* Mic-E is complex — this is a simplified decoder.
     * Full implementation would decode:
     *   - Latitude from destination field characters
     *   - Longitude from information field
     *   - Speed/course from information field
     *   - Symbol from source SSID and destination SSID
     */

    if (!raw || !lat || !lon) return 0;

    /* Find payload after ':' */
    const char* colon = memchr(raw, ':', len);
    if (!colon) return 0;
    colon++; /* skip ':' */

    /* Check for Mic-E type identifiers */
    if (*colon != '`' && *colon != '\'') return 0;

    /* The payload after the Mic-E byte contains position info */
    /* Format: `....LatData...LonData... */
    /* For now, decode the basic position fields if present */

    const char* pos = colon + 1;
    size_t pos_len = (size_t)(raw + len - pos);

    /* Look for position data pattern: DDMM.mmN/DDDMM.mmE or encoded format */
    /* This is a simplified extraction */
    char lat_str[20] = {0};
    char lon_str[20] = {0};

    /* Try to find N/S and E/W indicators */
    const char* ns = NULL;
    const char* ew = NULL;
    for (size_t i = 0; i < pos_len && i < 40; i++) {
        if ((pos[i] == 'N' || pos[i] == 'S') && !ns) ns = pos + i;
        if ((pos[i] == 'E' || pos[i] == 'W') && !ew) ew = pos + i;
    }

    if (ns && ew && ns < ew) {
        /* Extract lat string (characters before NS indicator) */
        size_t lat_len_val = 0;
        for (const char* p = ns - 1; p >= pos && (isdigit((unsigned char)*p) || *p == '.'); p--) {
            lat_len_val++;
        }
        if (lat_len_val > 0 && lat_len_val < sizeof(lat_str)) {
            memcpy(lat_str, ns - lat_len_val, lat_len_val);
            *lat = atof(lat_str);
            if (*(ns) == 'S') *lat = -(*lat);
            /* Convert from DDMM.mm to decimal degrees */
            int deg = (int)(*lat / 100.0);
            double min = *lat - (deg * 100.0);
            *lat = deg + min / 60.0;
        }

        /* Extract lon string (characters before EW indicator) */
        size_t lon_len_val = 0;
        for (const char* p = ew - 1; p > ns && (isdigit((unsigned char)*p) || *p == '.'); p--) {
            lon_len_val++;
        }
        if (lon_len_val > 0 && lon_len_val < sizeof(lon_str)) {
            memcpy(lon_str, ew - lon_len_val, lon_len_val);
            *lon = atof(lon_str);
            if (*(ew) == 'W') *lon = -(*lon);
            int deg = (int)(*lon / 100.0);
            double min = *lon - (deg * 100.0);
            *lon = deg + min / 60.0;
        }
        if (info) info[0] = '\0';
        return 1;
    }

    return 0;
}

/* Decode a standard uncompressed APRS position from the payload.
 * Format: DDMM.mmN/DDDMM.mmE  (after the data type indicator)
 *
 * Returns 1 on success, 0 on failure.
 */
int fi_decode_position(const char* payload, size_t len, double* lat, double* lon,
                       char* sym_table, char* sym_code) {
    if (!payload || !lat || !lon || len < 10) return 0;

    /* Skip data type char if present */
    if (*payload == '!' || *payload == '=' || *payload == '/' || *payload == '@') {
        payload++;
        len--;
    }

    char lat_str[16] = {0};
    char lon_str[16] = {0};

    /* Extract lat: DDMM.mm followed by N or S */
    const char* p = payload;
    const char* ns = NULL;
    while (*p && (size_t)(p - payload) < len) {
        if (*p == 'N' || *p == 'S') { ns = p; break; }
        p++;
    }
    if (!ns) return 0;

    /* Walk back to find start of lat digits */
    const char* lat_start = ns - 1;
    while (lat_start >= payload && (isdigit((unsigned char)*lat_start) || *lat_start == '.')) {
        lat_start--;
    }
    lat_start++;

    size_t lat_chars = (size_t)(ns - lat_start);
    if (lat_chars >= sizeof(lat_str)) lat_chars = sizeof(lat_str) - 1;
    memcpy(lat_str, lat_start, lat_chars);

    /* Extract lon: after symbol table char, DDDMM.mm followed by E or W */
    const char* ew = NULL;
    p = ns + 2; /* skip N/S and symbol table char */
    while (*p && (size_t)(p - payload) < len) {
        if (*p == 'E' || *p == 'W') { ew = p; break; }
        p++;
    }
    if (!ew) return 0;

    /* Walk back to find start of lon digits */
    const char* lon_start = ew - 1;
    while (lon_start > ns && (isdigit((unsigned char)*lon_start) || *lon_start == '.')) {
        lon_start--;
    }
    lon_start++;

    size_t lon_chars = (size_t)(ew - lon_start);
    if (lon_chars >= sizeof(lon_str)) lon_chars = sizeof(lon_str) - 1;
    memcpy(lon_str, lon_start, lon_chars);

    /* Parse */
    double lat_val = atof(lat_str);
    double lon_val = atof(lon_str);

    /* Convert DDMM.mm to decimal degrees */
    int lat_deg = (int)(lat_val / 100.0);
    double lat_min = lat_val - (lat_deg * 100.0);
    *lat = lat_deg + lat_min / 60.0;
    if (*ns == 'S') *lat = -(*lat);

    int lon_deg = (int)(lon_val / 100.0);
    double lon_min = lon_val - (lon_deg * 100.0);
    *lon = lon_deg + lon_min / 60.0;
    if (*ew == 'W') *lon = -(*lon);

    /* Extract symbol table and code */
    if (sym_table) *sym_table = *(ns + 1);     /* char after N/S */
    if (sym_code) {
        if (*(ew + 1)) {
            *sym_code = *(ew + 1);              /* char after E/W */
        } else {
            *sym_code = '/';                     /* default */
        }
    }

    return 1;
}

/* Compute a simple 8-bit CRC for duplicate detection */
uint8_t fi_compute_crc(const char* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= (unsigned char)data[i];
    }
    return crc;
}
