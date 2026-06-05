#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * BEACON MANAGER — Beacon composition, path/status rotation, scheduling
 *
 * Extracted from:
 *   - sta_manager.c (send_beacon)
 *   - timer.c (tc1 beacon timing)
 *   - config.c (config_path, config_status, config_beacon_delay)
 * ======================================================================== */

/* Set status text for a slot */
void bm_set_status_text(aprs_engine_t* e, int slot, const char* text) {
    if (!e || !text) return;
    if (slot < 0 || slot >= MAX_STATUS_TEXTS) return;

    strncpy(e->status_texts[slot].text, text, sizeof(e->status_texts[slot].text) - 1);
    e->status_texts[slot].text[sizeof(e->status_texts[slot].text) - 1] = '\0';
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Set path for a slot */
void bm_set_path(aprs_engine_t* e, int slot, const char* path) {
    if (!e || !path) return;
    if (slot < 0 || slot >= MAX_PATHS) return;

    strncpy(e->paths[slot].path, path, sizeof(e->paths[slot].path) - 1);
    e->paths[slot].path[sizeof(e->paths[slot].path) - 1] = '\0';
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Enable/disable a status text */
void bm_select_status(aprs_engine_t* e, int slot, uint8_t enabled) {
    if (!e) return;
    if (slot < 0 || slot >= MAX_STATUS_TEXTS) return;

    e->status_texts[slot].enabled = enabled;
    if (enabled) {
        e->status_use |= (1 << slot);
    } else {
        e->status_use &= ~(1 << slot);
    }
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Enable/disable a path */
void bm_select_path(aprs_engine_t* e, int slot, uint8_t enabled) {
    if (!e) return;
    if (slot < 0 || slot >= MAX_PATHS) return;

    e->paths[slot].enabled = enabled;
    if (enabled) {
        e->path_use |= (1 << slot);
    } else {
        e->path_use &= ~(1 << slot);
    }
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Set beacon delay in seconds */
void bm_set_delay(aprs_engine_t* e, int delay_sec) {
    if (!e) return;
    e->beacon_delay_sec = (uint32_t)delay_sec;
    e->next_beacon_time_ms = e->current_time_ms + (uint64_t)delay_sec * 1000;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Set beacon auto mode: 0=fixed interval, 1=SmartBeaconing (speed-based) */
void bm_set_auto_mode(aprs_engine_t* e, uint8_t smart_enabled) {
    if (!e) return;
    e->beacon_auto_smart = smart_enabled;
    e->beacon_auto_enabled = 1;
    /* Recalculate next beacon time with new mode */
    if (smart_enabled) {
        uint32_t interval = bm_compute_smart_interval(e);
        e->next_beacon_time_ms = e->current_time_ms + interval;
    } else {
        e->next_beacon_time_ms = e->current_time_ms + (uint64_t)e->beacon_delay_sec * 1000;
    }
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Rotate to the next enabled path index.
 * Returns the index of the selected path, or -1 if none enabled. */
static int bm_rotate_path(aprs_engine_t* e) {
    /* Find next enabled path */
    for (int attempt = 0; attempt < MAX_PATHS; attempt++) {
        e->current_path_idx = (e->current_path_idx + 1) % MAX_PATHS;
        if (e->paths[e->current_path_idx].enabled) {
            return e->current_path_idx;
        }
    }
    /* Try any enabled path */
    for (int i = 0; i < MAX_PATHS; i++) {
        if (e->paths[i].enabled) return i;
    }
    return -1;
}

/* Rotate to the next enabled status text index. */
static int bm_rotate_status(aprs_engine_t* e) {
    for (int attempt = 0; attempt < MAX_STATUS_TEXTS; attempt++) {
        e->current_status_idx = (e->current_status_idx + 1) % MAX_STATUS_TEXTS;
        if (e->status_texts[e->current_status_idx].enabled) {
            return e->current_status_idx;
        }
    }
    for (int i = 0; i < MAX_STATUS_TEXTS; i++) {
        if (e->status_texts[i].enabled) return i;
    }
    return -1;
}

/* Compose a complete APRS beacon frame.
 * Format: CALLSIGN-SSID>UDEST,PATH:!LATLONSymTabSymCodeStatusText
 */
int bm_compose_beacon(aprs_engine_t* e, char* out, size_t* out_len) {
    if (!e || !out || !out_len) return 0;

    /* Get effective position for beacon */
    double lat, lon;
    pm_get_effective_report_position(e, &lat, &lon);

    /* Select path */
    int path_idx = bm_rotate_path(e);
    const char* path = (path_idx >= 0) ? e->paths[path_idx].path : "WIDE1-1";

    /* Select status text */
    int status_idx = bm_rotate_status(e);
    const char* status = (status_idx >= 0) ? e->status_texts[status_idx].text : "";

    /* Build source: CALLSIGN-SSID */
    char src[24];
    if (e->config.ssid[0]) {
        snprintf(src, sizeof(src), "%s-%s", e->config.callsign, e->config.ssid);
    } else {
        snprintf(src, sizeof(src), "%s", e->config.callsign);
    }

    /* Unproto destination */
    const char* udest = (e->udest[0]) ? e->udest : "APRS";

    /* Build header */
    char header[128];
    if (path && path[0]) {
        snprintf(header, sizeof(header), "%s>%s,%s", src, udest, path);
    } else {
        snprintf(header, sizeof(header), "%s>%s", src, udest);
    }

    /* Build position string using internal encoder */
    char pos_str[64];
    fi_encode_position(lat, lon, e->config.symbol_table, e->config.symbol_code,
                       pos_str, sizeof(pos_str));

    /* Append status text (no leading '>' needed here, it's inline) */
    char full_frame[512];
    if (status && status[0]) {
        /* Add NAV mark if enabled */
        const char* nav = e->nav_mark_enabled ? "{NAV}" : "";
        snprintf(full_frame, sizeof(full_frame), "%s:%s%s%s",
                 header, pos_str, status, nav);
    } else {
        snprintf(full_frame, sizeof(full_frame), "%s:%s",
                 header, pos_str);
    }

    size_t frame_len = strlen(full_frame);
    if (frame_len >= *out_len) {
        *out_len = frame_len + 1;
        return 0;
    }

    memcpy(out, full_frame, frame_len + 1);
    *out_len = frame_len;

    /* Save last beacon */
    strncpy(e->last_beacon_raw, full_frame, sizeof(e->last_beacon_raw) - 1);
    e->last_beacon_raw[sizeof(e->last_beacon_raw) - 1] = '\0';
    e->last_beacon_len = frame_len;

    return 1;
}

/* Send beacon immediately (manual trigger).
 * Composes and queues the frame for transmission.
 */
int bm_send_beacon(aprs_engine_t* e) {
    if (!e) return 0;

    /* Honor TX lock */
    if (!e->config.tx_enabled || !e->config.beacon_enabled) return 0;

    char frame[512];
    size_t frame_len = sizeof(frame);
    if (!bm_compose_beacon(e, frame, &frame_len)) return 0;

    /* Record TX */
    se_record_tx(e);

    /* Publish beacon sent event */
    aprs_frame_t af;
    memset(&af, 0, sizeof(af));
    memcpy(af.raw, frame, frame_len);
    af.raw_len = frame_len;
    af.is_valid = 1;
    strncpy(af.src, e->config.callsign, sizeof(af.src) - 1);

    eb_publish(e, APRS_EVT_BEACON_SENT, "beacon.sent", &af, sizeof(af));
    eb_publish(e, APRS_EVT_FRAME_TX, "frame.tx", &af, sizeof(af));

    /* Reset next beacon timer (SmartBeaconing or fixed) */
    e->last_beacon_sent_ms = e->current_time_ms;

    if (e->beacon_auto_smart) {
        /* SmartBeaconing: interval based on speed */
        uint32_t interval = bm_compute_smart_interval(e);
        e->next_beacon_time_ms = e->current_time_ms + interval;
    } else {
        e->next_beacon_time_ms = e->current_time_ms + (uint64_t)e->beacon_delay_sec * 1000;
    }

    /* Arm digisens auto-retry if enabled */
    if (e->beacon_digi_sensitivity) {
        e->digisens_retry_pending = 1;
    }

    /* Sound notification */
    nm_trigger(e, SND_BEACON_TX);

    return 1;
}

/* Compute SmartBeaconing interval based on current speed.
 * Rules (matching APRS SmartBeaconing convention):
 *   - Speed < 5 km/h (stationary): use configured max delay
 *   - Speed 5-110 km/h: interval = max(30s, min(max_delay, 3600 / speed_kmh * 1000ms))
 *   - Speed > 110 km/h: use minimum 30 seconds
 * Returns interval in milliseconds. */
uint32_t bm_compute_smart_interval(aprs_engine_t* e) {
    if (!e) return e->beacon_delay_sec * 1000;

    uint32_t max_ms = e->beacon_delay_sec * 1000;
    if (max_ms < 30000) max_ms = 30000; /* absolute minimum 30s */

    double speed = e->gps.speed_kmh;
    if (!e->gps.valid || speed < 1.0) {
        /* Stationary or no GPS: use maximum interval */
        return max_ms;
    }

    if (speed < 5.0) {
        /* Very slow: use max */
        return max_ms;
    }

    if (speed > 110.0) {
        /* Very fast: minimum 30s */
        return 30000;
    }

    /* Dynamic: interval = distance to cover ~1km at current speed, but not less than 30s
     * At 100 km/h: 3600/100 = 36s → 36 seconds between beacons
     * At 50 km/h: 3600/50 = 72s → 72 seconds
     * At 10 km/h: 3600/10 = 360s → 6 minutes */
    uint32_t dynamic = (uint32_t)(3600.0 / speed * 1000.0);

    /* Clamp: at least 30 seconds, at most the configured max */
    if (dynamic < 30000) dynamic = 30000;
    if (dynamic > max_ms) dynamic = max_ms;

    return dynamic;
}

/* Periodic tick: check if auto-beacon should fire */
void bm_tick(aprs_engine_t* e) {
    if (!e) return;
    if (!e->beacon_auto_enabled) return;
    if (!e->config.tx_enabled || !e->config.beacon_enabled) return;

    /* Digisens auto-retry: if 15s elapsed since last beacon and no digi repeated it */
    if (e->digisens_retry_pending && e->beacon_digi_sensitivity) {
        uint64_t elapsed = e->current_time_ms - e->last_beacon_sent_ms;
        if (elapsed >= 15000) {
            /* Re-send the last beacon */
            bm_send_beacon(e);
            return;
        }
    }

    if (e->current_time_ms >= e->next_beacon_time_ms) {
        bm_send_beacon(e);
    }
}

/* Preview the last transmitted beacon frame */
void bm_preview_last(aprs_engine_t* e, aprs_frame_t* out) {
    if (!e || !out) return;

    memset(out, 0, sizeof(*out));
    memcpy(out->raw, e->last_beacon_raw, e->last_beacon_len);
    out->raw_len = e->last_beacon_len;
    out->is_valid = 1;
    strncpy(out->src, e->config.callsign, sizeof(out->src) - 1);
}
