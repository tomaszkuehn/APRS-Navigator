#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * RAW FRAME EDITOR — Copy/edit/replay raw APRS frames
 *
 * Extracted from:
 *   - config.c (raw frame view/edit — minimal in original)
 *   - sta_manager.c (generic frame buffer handling)
 * ======================================================================== */

/* Copy the last received frame from a given station into the editor buffer */
int rfe_copy_from_station(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return 0;

    int16_t idx = sm_find_station(e, callsign);
    if (idx < 0) return 0;

    /* We store the raw frame in the station's info field; for now
     * build a template frame from the station data */
    aprs_station_t* st = &e->stations[idx];

    if (st->has_position) {
        /* Build a position frame template */
        char pos_frame[512];
        int n = fi_encode_position(st->lat, st->lon,
                                    st->symbol_table, st->symbol_code,
                                    pos_frame, sizeof(pos_frame));
        if (n > 0) {
            /* Replace leading '!' with proper header */
            char full_frame[512];
            snprintf(full_frame, sizeof(full_frame), "%s>APRS:%s",
                     callsign, pos_frame);
            strncpy(e->editor_frame_raw, full_frame, sizeof(e->editor_frame_raw) - 1);
            e->editor_frame_raw[sizeof(e->editor_frame_raw) - 1] = '\0';
            e->editor_frame_len = strlen(e->editor_frame_raw);
            return 1;
        }
    }

    /* Fallback: build a simple status frame */
    snprintf(e->editor_frame_raw, sizeof(e->editor_frame_raw),
             "%s>APRS:>Captured from %s",
             e->config.callsign, callsign);
    e->editor_frame_raw[sizeof(e->editor_frame_raw) - 1] = '\0';
    e->editor_frame_len = strlen(e->editor_frame_raw);

    return 1;
}

/* Get the current editor buffer content as an aprs_frame_t */
void rfe_get(aprs_engine_t* e, aprs_frame_t* out) {
    if (!e || !out) return;

    memset(out, 0, sizeof(*out));
    memcpy(out->raw, e->editor_frame_raw, e->editor_frame_len);
    out->raw_len = e->editor_frame_len;
    /* Parse into a temp frame, then copy fields back (avoid memcpy overlap) */
    aprs_frame_t parsed;
    memset(&parsed, 0, sizeof(parsed));
    fi_parse_frame(e->editor_frame_raw, e->editor_frame_len, &parsed);
    /* Preserve raw content, copy parsed metadata */
    strncpy(out->src, parsed.src, sizeof(out->src) - 1);
    strncpy(out->dst, parsed.dst, sizeof(out->dst) - 1);
    strncpy(out->path, parsed.path, sizeof(out->path) - 1);
    out->is_valid = parsed.is_valid;
}

/* Set the editor buffer from a raw frame string */
void rfe_set_text(aprs_engine_t* e, const char* raw) {
    if (!e || !raw) return;

    strncpy(e->editor_frame_raw, raw, sizeof(e->editor_frame_raw) - 1);
    e->editor_frame_raw[sizeof(e->editor_frame_raw) - 1] = '\0';
    e->editor_frame_len = strlen(e->editor_frame_raw);

    /* Publish frame.raw event */
    eb_publish(e, APRS_EVT_FRAME_RAW, "frame.raw",
               e->editor_frame_raw, e->editor_frame_len + 1);
}

/* Edit a single byte at the given offset */
void rfe_edit_byte(aprs_engine_t* e, size_t offset, uint8_t value) {
    if (!e) return;
    if (offset >= sizeof(e->editor_frame_raw)) return;

    e->editor_frame_raw[offset] = (char)value;

    /* If we wrote past current length, extend */
    if (offset >= e->editor_frame_len) {
        e->editor_frame_len = offset + 1;
        e->editor_frame_raw[e->editor_frame_len] = '\0';
    }

    eb_publish(e, APRS_EVT_FRAME_RAW, "frame.raw",
               e->editor_frame_raw, e->editor_frame_len + 1);
}

/* Send the editor frame (queue for transmission) */
int rfe_send(aprs_engine_t* e) {
    if (!e) return 0;
    if (e->editor_frame_len == 0) return 0;

    /* Honor TX lock */
    if (!e->config.tx_enabled) return 0;

    /* Publish frame TX event */
    aprs_frame_t af;
    memset(&af, 0, sizeof(af));
    memcpy(af.raw, e->editor_frame_raw, e->editor_frame_len);
    af.raw_len = e->editor_frame_len;
    fi_parse_frame(af.raw, af.raw_len, &af);

    eb_publish(e, APRS_EVT_FRAME_TX, "frame.tx", &af, sizeof(af));

    /* Record TX */
    se_record_tx(e);

    return 1;
}

/* Clear the editor buffer */
void rfe_clear(aprs_engine_t* e) {
    if (!e) return;

    memset(e->editor_frame_raw, 0, sizeof(e->editor_frame_raw));
    e->editor_frame_len = 0;

    eb_publish(e, APRS_EVT_FRAME_RAW, "frame.raw", NULL, 0);
}
