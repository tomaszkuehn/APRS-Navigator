#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * POSITION MANAGER — USE/REPORT positions, AUTO/STATIC/GPS modes, 3 slots
 *
 * Extracted from:
 *   - config.c (config_position, capture_position)
 *   - globals.c (posbank, mylat, mylon, position_mode, posuse)
 * ======================================================================== */

void pm_set_use_mode(aprs_engine_t* e, aprs_position_mode_t mode) {
    if (!e) return;
    e->use_mode = mode;

    /* If switching to GPS, immediately use GPS coords if available */
    if (mode == APRS_POSITION_GPS && e->gps.valid) {
        e->my_lat = e->gps.lat;
        e->my_lon = e->gps.lon;
    }

    sm_recalc_all(e);
    eb_publish(e, APRS_EVT_POSITION_CHANGED, "position.changed", NULL, 0);
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void pm_set_report_mode(aprs_engine_t* e, aprs_position_mode_t mode) {
    if (!e) return;
    e->report_mode = mode;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

int pm_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt) {
    if (!e) return 0;
    if (slot < 0 || slot >= MAX_POS_SLOTS) return 0;

    e->pos_slots[slot].lat = lat;
    e->pos_slots[slot].lon = lon;
    e->pos_slots[slot].alt = alt;

    /* If this is the active slot and mode is STATIC, update position */
    if (slot == e->active_pos_slot && e->use_mode == APRS_POSITION_STATIC) {
        e->my_lat = lat;
        e->my_lon = lon;
        e->my_alt = alt;
        sm_recalc_all(e);
    }

    eb_publish(e, APRS_EVT_POSITION_CHANGED, "position.changed", NULL, 0);
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
    return 1;
}

int pm_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt) {
    if (!e) return 0;
    if (slot < 0 || slot >= MAX_POS_SLOTS) return 0;

    if (lat) *lat = e->pos_slots[slot].lat;
    if (lon) *lon = e->pos_slots[slot].lon;
    if (alt) *alt = e->pos_slots[slot].alt;
    return 1;
}

void pm_select_slot(aprs_engine_t* e, int slot) {
    if (!e) return;
    if (slot < 0 || slot >= MAX_POS_SLOTS) return;

    e->active_pos_slot = slot;

    /* Update position from selected slot if in STATIC mode */
    if (e->use_mode == APRS_POSITION_STATIC) {
        e->my_lat = e->pos_slots[slot].lat;
        e->my_lon = e->pos_slots[slot].lon;
        e->my_alt = e->pos_slots[slot].alt;
        sm_recalc_all(e);
    }

    eb_publish(e, APRS_EVT_POSITION_CHANGED, "position.changed", NULL, 0);
}

int pm_capture_station(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return 0;

    int16_t idx = sm_find_station(e, callsign);
    if (idx < 0) return 0;

    aprs_station_t* st = &e->stations[idx];
    if (!st->has_position) return 0;

    int slot = e->active_pos_slot;
    e->pos_slots[slot].lat = st->lat;
    e->pos_slots[slot].lon = st->lon;
    /* altitude not available from APRS position, keep existing */

    /* If in STATIC mode, update current position */
    if (e->use_mode == APRS_POSITION_STATIC) {
        e->my_lat = st->lat;
        e->my_lon = st->lon;
        sm_recalc_all(e);
    }

    /* Publish position captured event */
    eb_publish(e, APRS_EVT_POSITION_CAPTURED, "position.captured",
               st, sizeof(aprs_station_t));
    return 1;
}

void pm_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt) {
    if (!e) return;

    /* Store in current slot */
    int slot = e->active_pos_slot;
    e->pos_slots[slot].lat = lat;
    e->pos_slots[slot].lon = lon;
    e->pos_slots[slot].alt = alt;

    /* Update current position */
    e->my_lat = lat;
    e->my_lon = lon;
    e->my_alt = alt;
    e->use_mode = APRS_POSITION_STATIC;

    sm_recalc_all(e);
    eb_publish(e, APRS_EVT_POSITION_CHANGED, "position.changed", NULL, 0);
}

/* Get the effective USE position based on current mode */
void pm_get_effective_position(aprs_engine_t* e, double* lat, double* lon) {
    if (!e) return;

    switch (e->use_mode) {
        case APRS_POSITION_GPS:
            if (e->gps.valid) {
                *lat = e->gps.lat;
                *lon = e->gps.lon;
                return;
            }
            /* Fall through to static if GPS invalid */
        case APRS_POSITION_STATIC:
            *lat = e->pos_slots[e->active_pos_slot].lat;
            *lon = e->pos_slots[e->active_pos_slot].lon;
            return;
        case APRS_POSITION_AUTO:
        default:
            /* AUTO: prefer GPS, fall back to static, then slot 0 */
            if (e->gps.valid) {
                *lat = e->gps.lat;
                *lon = e->gps.lon;
            } else {
                *lat = e->pos_slots[e->active_pos_slot].lat;
                *lon = e->pos_slots[e->active_pos_slot].lon;
            }
            return;
    }
}

/* Get the effective REPORT position (for beacons) */
void pm_get_effective_report_position(aprs_engine_t* e, double* lat, double* lon) {
    if (!e) return;

    switch (e->report_mode) {
        case APRS_POSITION_GPS:
            if (e->gps.valid) {
                *lat = e->gps.lat;
                *lon = e->gps.lon;
                return;
            }
            /* Fall through */
        case APRS_POSITION_STATIC:
            *lat = e->pos_slots[e->active_pos_slot].lat;
            *lon = e->pos_slots[e->active_pos_slot].lon;
            return;
        case APRS_POSITION_AUTO:
        default:
            pm_get_effective_position(e, lat, lon);
            return;
    }
}
