#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ========================================================================
 * STATION MANAGER — Station database, sort, filter, distance/bearing math
 *
 * Extracted from:
 *   - sta_manager.c (station_sort, station_list, dist_calc, update_aprs)
 *   - globals.c (station array, sort array)
 * ======================================================================== */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========================================================================
 * STATION LOOKUP
 * ======================================================================== */

/* Find station index by callsign. Returns index or -1 if not found. */
int16_t sm_find_station(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return -1;

    for (uint16_t i = 0; i < e->num_stations; i++) {
        if (strcmp(e->stations[i].callsign, callsign) == 0) {
            return (int16_t)i;
        }
    }
    return -1;
}

/* Find station by callsign, or create a new entry if not found.
 * Returns index, or -1 if station list is full.
 */
int16_t sm_find_or_create(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return -1;

    /* Search existing stations */
    int16_t idx = sm_find_station(e, callsign);
    if (idx >= 0) return idx;

    /* Create new station */
    if (e->num_stations >= MAX_STATIONS) {
        /* List is full — replace the oldest station (index 0 is typically "self") */
        /* Find oldest station by last_heard_ms */
        uint64_t oldest_time = UINT64_MAX;
        uint16_t oldest_idx = 0;
        for (uint16_t i = 1; i < e->num_stations; i++) {  /* skip self at index 0 */
            if (e->stations[i].last_heard_ms < oldest_time) {
                oldest_time = e->stations[i].last_heard_ms;
                oldest_idx = i;
            }
        }
        idx = (int16_t)oldest_idx;
        /* Clear the slot */
        memset(&e->stations[idx], 0, sizeof(aprs_station_t));
    } else {
        idx = (int16_t)e->num_stations;
        e->num_stations++;
    }

    /* Initialize new entry */
    strncpy(e->stations[idx].callsign, callsign, sizeof(e->stations[idx].callsign) - 1);
    e->stations[idx].callsign[sizeof(e->stations[idx].callsign) - 1] = '\0';
    e->stations[idx].last_heard_ms = e->current_time_ms;
    e->stations[idx].packet_count = 0;
    e->stations[idx].has_position = 0;
    e->stations[idx].has_weather = 0;
    e->stations[idx].marked = 0;

    return idx;
}

/* ========================================================================
 * STATION UPDATE FROM FRAME
 * ======================================================================== */

void sm_update_from_frame(aprs_engine_t* e, uint16_t idx, const aprs_frame_t* f, frame_class_t fc) {
    if (!e || !f || idx >= e->num_stations) return;

    aprs_station_t* st = &e->stations[idx];

    st->packet_count++;
    st->last_heard_ms = e->current_time_ms;

    /* Copy path */
    strncpy(st->last_path, f->path, sizeof(st->last_path) - 1);
    st->last_path[sizeof(st->last_path) - 1] = '\0';

    /* Store frame type character */
    const char* colon = strchr(f->raw, ':');
    if (colon && *(colon + 1)) {
        st->frame_type = (uint8_t)*(colon + 1);
    }

    /* Try to decode position from position-type frames */
    if (fc == FRAME_CLASS_POSITION) {
        const char* payload = colon ? colon + 1 : f->raw;
        double lat, lon;
        char sym_tab, sym_code;
        if (fi_decode_position(payload,
                f->raw_len - (size_t)(payload - f->raw),
                &lat, &lon, &sym_tab, &sym_code)) {
            st->lat = lat;
            st->lon = lon;
            st->symbol_table = sym_tab;
            st->symbol_code = sym_code;
            st->has_position = 1;
        }
    }

    /* Try Mic-E decode */
    if (fc == FRAME_CLASS_MIC_E) {
        double lat = 0, lon = 0;
        if (fi_decode_mic_e(f->raw, f->raw_len, &lat, &lon, st->info)) {
            st->lat = lat;
            st->lon = lon;
            st->has_position = 1;
        }
    }

    /* Extract info/comment from status frames */
    if (fc == FRAME_CLASS_STATUS && colon) {
        const char* status_text = colon + 2; /* skip '>' */
        size_t text_len = strlen(status_text);
        if (text_len >= sizeof(st->info)) text_len = sizeof(st->info) - 1;
        memcpy(st->info, status_text, text_len);
        st->info[text_len] = '\0';
    }

    /* Weather: extract temperature if present */
    if (fc == FRAME_CLASS_WX && colon) {
        st->has_weather = 1;
        const char* wx_data = colon + 1;
        /* Look for "t" temperature field: t075 = 75°F */
        const char* tpos = strstr(wx_data, "t");
        if (tpos && isdigit((unsigned char)tpos[1])) {
            int temp_f = atoi(tpos + 1);
            /* Store as-is in comment for now */
            snprintf(st->comment, sizeof(st->comment), "WX: %dF", temp_f);
        }
    }
}

/* ========================================================================
 * SORTING AND FILTERING
 * ======================================================================== */

/* Sort station indices by the given column.
 * Columns: "callsign", "distance", "bearing", "packets", "last_heard", "symbol"
 */
void sm_sort(aprs_engine_t* e, const char* column, uint8_t ascending) {
    if (!e || !column) return;

    /* Rebuild sort order */
    for (uint16_t i = 0; i < e->num_stations; i++) {
        e->sort_order[i] = i;
    }

    /* Bubble sort (small N, simple) */
    for (uint16_t i = 0; i < e->num_stations; i++) {
        for (uint16_t j = i + 1; j < e->num_stations; j++) {
            int cmp = 0;
            aprs_station_t* a = &e->stations[e->sort_order[i]];
            aprs_station_t* b = &e->stations[e->sort_order[j]];

            if (strcmp(column, "callsign") == 0) {
                cmp = strcmp(a->callsign, b->callsign);
            } else if (strcmp(column, "distance") == 0) {
                cmp = (a->distance_km < b->distance_km) ? -1 :
                      (a->distance_km > b->distance_km) ? 1 : 0;
            } else if (strcmp(column, "bearing") == 0) {
                cmp = (a->bearing_deg < b->bearing_deg) ? -1 :
                      (a->bearing_deg > b->bearing_deg) ? 1 : 0;
            } else if (strcmp(column, "packets") == 0) {
                cmp = (int)(a->packet_count - b->packet_count);
            } else if (strcmp(column, "last_heard") == 0) {
                cmp = (a->last_heard_ms < b->last_heard_ms) ? -1 :
                      (a->last_heard_ms > b->last_heard_ms) ? 1 : 0;
            } else if (strcmp(column, "symbol") == 0) {
                cmp = (a->symbol_code - b->symbol_code);
            }

            if (!ascending) cmp = -cmp;

            if (cmp > 0) {
                uint16_t tmp = e->sort_order[i];
                e->sort_order[i] = e->sort_order[j];
                e->sort_order[j] = tmp;
            }
        }
    }

    /* Store sort parameters */
    strncpy(e->sort_column, column, sizeof(e->sort_column) - 1);
    e->sort_column[sizeof(e->sort_column) - 1] = '\0';
    e->sort_ascending = ascending;
}

/* Apply include/exclude filter masks to the station list.
 * Updates filtered_count.
 */
void sm_apply_filter_mask(aprs_engine_t* e) {
    if (!e) return;

    e->filtered_count = 0;
    for (uint16_t i = 0; i < e->num_stations; i++) {
        uint16_t si = e->sort_order[i];
        aprs_station_t* st = &e->stations[si];

        /* Check include mask */
        if (e->filter_include_mask) {
            uint8_t matched = 0;
            if ((e->filter_include_mask & FILTER_POSITION) && st->has_position) matched = 1;
            if ((e->filter_include_mask & FILTER_NOPOSITION) && !st->has_position) matched = 1;
            if ((e->filter_include_mask & FILTER_WX) && st->has_weather) matched = 1;
            if (!matched) continue;
        }

        /* Check exclude mask */
        if (e->filter_exclude_mask) {
            if ((e->filter_exclude_mask & FILTER_POSITION) && st->has_position) continue;
            if ((e->filter_exclude_mask & FILTER_NOPOSITION) && !st->has_position) continue;
            if ((e->filter_exclude_mask & FILTER_WX) && st->has_weather) continue;
        }

        e->filtered_count++;
    }
}

/* ========================================================================
 * MARKING
 * ======================================================================== */

void sm_mark(aprs_engine_t* e, const char* callsign, uint8_t mark) {
    if (!e || !callsign) return;

    int16_t idx = sm_find_station(e, callsign);
    if (idx < 0) return;

    e->stations[idx].marked = mark;
    if (mark) {
        e->marked_station_idx = (uint16_t)idx;
    } else if (e->marked_station_idx == (uint16_t)idx) {
        e->marked_station_idx = 0;
    }

    /* Publish event */
    eb_publish(e, APRS_EVT_STATION_MARKED, "station.marked",
               &e->stations[idx], sizeof(aprs_station_t));
}

void sm_clear_all_marks(aprs_engine_t* e) {
    if (!e) return;
    for (uint16_t i = 0; i < e->num_stations; i++) {
        e->stations[i].marked = 0;
    }
    e->marked_station_idx = 0;
}

/* ========================================================================
 * DISTANCE AND BEARING (Great Circle)
 * ======================================================================== */

void sm_get_distance_bearing(double lat1, double lon1, double lat2, double lon2,
                              double* dist_km, double* bearing_deg) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double rlat1 = lat1 * M_PI / 180.0;
    double rlat2 = lat2 * M_PI / 180.0;

    /* Haversine formula for distance */
    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(rlat1) * cos(rlat2) *
               sin(dlon / 2.0) * sin(dlon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    /* Earth radius: 6371 km */
    if (dist_km) *dist_km = 6371.0 * c;

    /* Bearing */
    if (bearing_deg) {
        double y = sin(dlon) * cos(rlat2);
        double x = cos(rlat1) * sin(rlat2) -
                   sin(rlat1) * cos(rlat2) * cos(dlon);
        double bearing = atan2(y, x) * 180.0 / M_PI;
        if (bearing < 0) bearing += 360.0;
        *bearing_deg = bearing;
    }
}

/* Recalculate distances and bearings for all stations from engine's current position */
void sm_recalc_all(aprs_engine_t* e) {
    if (!e) return;

    double my_lat, my_lon;
    pm_get_effective_position(e, &my_lat, &my_lon);

    for (uint16_t i = 0; i < e->num_stations; i++) {
        if (e->stations[i].has_position) {
            sm_get_distance_bearing(my_lat, my_lon,
                                     e->stations[i].lat, e->stations[i].lon,
                                     &e->stations[i].distance_km,
                                     &e->stations[i].bearing_deg);
        } else {
            e->stations[i].distance_km = 0;
            e->stations[i].bearing_deg = 0;
        }
    }
}
