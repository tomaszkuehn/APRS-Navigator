#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================================================
 * GPS MANAGER — NMEA sentence parsing (GPGGA, GPRMC)
 *
 * Extracted from:
 *   - gps.c (SerHandler, gps_process, gps_read, update_bygps)
 * ======================================================================== */

/* Internal GPS parsing state */
typedef struct {
    char field[16];
    int field_idx;
    int field_count;
    char sentence_type[8];
    char checksum_buf[3];
    int in_checksum;
    int checksum_val;
    int calc_checksum;

    /* GPGGA fields */
    char gg_time[12];
    char gg_lat[12];
    char gg_ns;
    char gg_lon[13];
    char gg_ew;
    char gg_quality;
    char gg_sats[3];
    char gg_alt[8];

    /* GPRMC fields */
    char rm_time[12];
    char rm_status;
    char rm_lat[12];
    char rm_ns;
    char rm_lon[13];
    char rm_ew;
    char rm_speed[8];
    char rm_course[8];

    int gg_complete;
    int rm_complete;
} gps_parser_t;

static gps_parser_t g_parser;

void gm_init(aprs_engine_t* e) {
    if (!e) return;
    memset(&e->gps, 0, sizeof(e->gps));
    e->gps_present = 0;
    e->gps_enabled = 1;
    e->gps_fix_counter = 0;
    e->gps_data_counter = 0;
    memset(&g_parser, 0, sizeof(g_parser));
}

void gm_set_enabled(aprs_engine_t* e, uint8_t enabled) {
    if (!e) return;
    e->gps_enabled = enabled;
}

void gm_get_state(aprs_engine_t* e, aprs_gps_t* out) {
    if (!e || !out) return;
    memcpy(out, &e->gps, sizeof(aprs_gps_t));
    out->fix = (e->gps_fix_counter > 0) ? 1 : 0;
}

/* Parse a single NMEA sentence character-by-character.
 * Called with each char; resets on '$' and completes on checksum.
 *
 * Returns 1 when a complete valid sentence has been parsed,
 * 0 while still accumulating.
 */
static int gm_parse_char(char c, aprs_engine_t* e) {
    /* Start of sentence */
    if (c == '$') {
        memset(&g_parser, 0, sizeof(g_parser));
        /* field_idx will accumulate sentence type first, then reset at first comma */
        return 0;
    }

    /* Checksum marker */
    if (c == '*') {
        g_parser.field[g_parser.field_idx] = '\0';
        g_parser.in_checksum = 1;
        g_parser.checksum_buf[0] = '\0';
        return 0;
    }

    /* End of line — sentence complete (with or without checksum) */
    if (c == '\r' || c == '\n') {
        return 2;
    }

    /* Backslash-r or backslash-n from JSON strings — also treat as EOL */
    if (c == '\\') {
        /* Next char might be r or n — handled on next call, but we're
         * at the end anyway. Accept what we have. */
    }

    /* Checksum chars — collect but always accept (lenient) */
    if (g_parser.in_checksum) {
        int len = (int)strlen(g_parser.checksum_buf);
        if (len < 2) {
            g_parser.checksum_buf[len] = c;
            g_parser.checksum_buf[len + 1] = '\0';
        }
        if (len == 1) {
            /* Got both hex chars — sentence complete regardless of match */
            return 2;
        }
        return 0;
    }

    /* Update running checksum (XOR of all chars between $ and *) */
    if (g_parser.field_count > 0 || g_parser.field_idx > 0 || c != ',') {
        g_parser.calc_checksum ^= (unsigned char)c;
    }

    /* Field separator */
    if (c == ',') {
        g_parser.field[g_parser.field_idx] = '\0';
        g_parser.field_idx = 0;
        g_parser.field_count++;  /* raw count (0=sentence type, 1=time, 2=lat, ...) */

        /* Skip field 0 (sentence type — already stored in sentence_type[]) */
        if (g_parser.field_count > 1) {
            int fn = g_parser.field_count - 1;  /* NMEA field number (1=time, 2=lat, ...) */

            if (strcmp(g_parser.sentence_type, "GPGGA") == 0 ||
                strcmp(g_parser.sentence_type, "GNGGA") == 0) {
                switch (fn) {
                    case 1: strncpy(g_parser.gg_time, g_parser.field, sizeof(g_parser.gg_time)-1); break;
                    case 2: strncpy(g_parser.gg_lat,  g_parser.field, sizeof(g_parser.gg_lat)-1); break;
                    case 3: g_parser.gg_ns = g_parser.field[0]; break;
                    case 4: strncpy(g_parser.gg_lon,  g_parser.field, sizeof(g_parser.gg_lon)-1); break;
                    case 5: g_parser.gg_ew = g_parser.field[0]; break;
                    case 6: g_parser.gg_quality = g_parser.field[0]; break;
                    case 7: strncpy(g_parser.gg_sats, g_parser.field, sizeof(g_parser.gg_sats)-1); break;
                    case 9: strncpy(g_parser.gg_alt,  g_parser.field, sizeof(g_parser.gg_alt)-1); break;
                }
            } else if (strcmp(g_parser.sentence_type, "GPRMC") == 0 ||
                       strcmp(g_parser.sentence_type, "GNRMC") == 0) {
                switch (fn) {
                    case 1: strncpy(g_parser.rm_time, g_parser.field, sizeof(g_parser.rm_time)-1); break;
                    case 2: g_parser.rm_status = g_parser.field[0]; break;
                    case 3: strncpy(g_parser.rm_lat, g_parser.field, sizeof(g_parser.rm_lat)-1); break;
                    case 4: g_parser.rm_ns = g_parser.field[0]; break;
                    case 5: strncpy(g_parser.rm_lon, g_parser.field, sizeof(g_parser.rm_lon)-1); break;
                    case 6: g_parser.rm_ew = g_parser.field[0]; break;
                    case 7: strncpy(g_parser.rm_speed, g_parser.field, sizeof(g_parser.rm_speed)-1); break;
                    case 8: strncpy(g_parser.rm_course, g_parser.field, sizeof(g_parser.rm_course)-1); break;
                }
            }
        }
        return 0;
    }

    /* Regular character — accumulate in field */
    if (g_parser.field_count == 0) {
        /* Sentence type */
        if (g_parser.field_idx < (int)sizeof(g_parser.sentence_type) - 1) {
            g_parser.sentence_type[g_parser.field_idx++] = c;
            g_parser.sentence_type[g_parser.field_idx] = '\0';
        }
    } else {
        if (g_parser.field_idx < (int)sizeof(g_parser.field) - 1) {
            g_parser.field[g_parser.field_idx++] = c;
        }
    }

    return 0;
}

/* Convert NMEA DDMM.MMMM to decimal degrees */
static double nmea_to_decimal(const char* nmea_str, char hemisphere) {
    if (!nmea_str || nmea_str[0] == '\0') return 0.0;

    double val = atof(nmea_str);
    int deg = (int)(val / 100.0);
    double minutes = val - (deg * 100.0);
    double decimal = deg + minutes / 60.0;

    if (hemisphere == 'S' || hemisphere == 'W') {
        decimal = -decimal;
    }
    return decimal;
}

/* ========================================================================
 * PUBLIC API
 * ======================================================================== */

int gm_push_nmea(aprs_engine_t* e, const char* sentence) {
    if (!e || !sentence) return 0;

    /* Feed each character to parser */
    int result = 0;
    for (const char* p = sentence; *p; p++) {
        int r = gm_parse_char(*p, e);
        if (r == 2) result = 1; /* valid complete sentence */
    }

    /* Also handle newline termination */
    gm_parse_char('\n', e);

    if (!result) return 0;

    /* GPGGA: position and fix quality */
    if (g_parser.gg_quality > '0') {
        double lat = nmea_to_decimal(g_parser.gg_lat, g_parser.gg_ns);
        double lon = nmea_to_decimal(g_parser.gg_lon, g_parser.gg_ew);

        e->gps.lat = lat;
        e->gps.lon = lon;
        e->gps.altitude_m = atof(g_parser.gg_alt);
        e->gps.valid = 1;
        e->gps.fix = 1;
        e->gps.ts_ms = e->current_time_ms;

        e->gps_fix_counter = 50;   /* ~5 seconds at 100ms/tick */
        e->gps_data_counter = 50;  /* ~5 seconds at 100ms/tick */
        e->gps_present = 1;

        /* Update engine position if in GPS mode */
        if (e->use_mode == APRS_POSITION_GPS) {
            e->my_lat = lat;
            e->my_lon = lon;
            sm_recalc_all(e);
        }

        /* Publish GPS updated event */
        eb_publish(e, APRS_EVT_GPS_UPDATED, "gps.updated",
                   &e->gps, sizeof(aprs_gps_t));

        /* Publish position changed event */
        eb_publish(e, APRS_EVT_POSITION_CHANGED, "position.changed",
                   &e->gps, sizeof(aprs_gps_t));
    }

    /* GPRMC: speed and course */
    if (g_parser.rm_status == 'A') {
        e->gps.speed_kmh = atof(g_parser.rm_speed) * 1.852;  /* knots to km/h */
        e->gps.course_deg = atof(g_parser.rm_course);
        e->my_course = (int)e->gps.course_deg;
        e->gps_present = 1;
    }

    return 1;
}

/* Tick: decrement GPS counters. GPS is considered "online" while
 * fix_counter > 0. After it reaches 0, GPS is considered lost. */
void gm_tick(aprs_engine_t* e) {
    if (!e) return;

    if (e->gps_fix_counter > 0) {
        e->gps_fix_counter--;
        if (e->gps_fix_counter == 0) {
            e->gps.fix = 0;
            /* Publish GPS updated to notify fix lost */
            eb_publish(e, APRS_EVT_GPS_UPDATED, "gps.updated",
                       &e->gps, sizeof(aprs_gps_t));
        }
    }
    if (e->gps_data_counter > 0) {
        e->gps_data_counter--;
    }
}
