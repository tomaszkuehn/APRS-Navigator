#include "internal.h"
#include <string.h>

/* ========================================================================
 * STATS ENGINE — Counters, hourly buckets, 20-hour buckets
 *
 * Extracted from:
 *   - timer.c (traffic[], utraffic[], dtraffic[], htraffic[])
 *   - globals.c (packet_count, unique_packet_count, etc.)
 * ======================================================================== */

/* Record an RX frame */
void se_record_rx(aprs_engine_t* e) {
    if (!e) return;
    e->stats.rx_frames++;
    e->traffic_buckets[e->current_minute_bucket]++;
    e->htraffic_buckets[e->current_10min_bucket]++;
}

/* Record a TX frame */
void se_record_tx(aprs_engine_t* e) {
    if (!e) return;
    e->stats.tx_frames++;
}

/* Record a digipeat */
void se_record_digi(aprs_engine_t* e) {
    if (!e) return;
    e->stats.digi_repeats++;
}

/* Record an invalid frame */
void se_record_invalid(aprs_engine_t* e) {
    if (!e) return;
    e->stats.invalid_frames++;
}

/* Record a unique frame */
void se_record_unique(aprs_engine_t* e) {
    if (!e) return;
    e->stats.unique_frames++;
}

/* Record a received message */
void se_record_msg_rx(aprs_engine_t* e) {
    if (!e) return;
    e->stats.msg_received++;
}

/* Record a sent message */
void se_record_msg_tx(aprs_engine_t* e) {
    if (!e) return;
    e->stats.msg_sent++;
}

/* Get current stats snapshot */
void se_get_stats(aprs_engine_t* e, aprs_stats_t* out) {
    if (!e || !out) return;
    memcpy(out, &e->stats, sizeof(aprs_stats_t));
    out->uptime_ms = e->current_time_ms - e->start_time_ms;
    out->tx_enabled = e->config.tx_enabled;
    out->digi_enabled = e->config.digi_enabled;
    out->beacon_enabled = e->config.beacon_enabled;
}

/* Get hourly bucket data (60 buckets, one per minute) */
int se_get_hourly(aprs_engine_t* e, uint32_t* buckets, size_t* count) {
    if (!e || !buckets || !count) return 0;

    size_t max_count = *count;
    if (max_count > HOURLY_BUCKETS) max_count = HOURLY_BUCKETS;

    /* Rotate so the oldest bucket is first, newest last */
    uint8_t start = (e->current_minute_bucket + 1) % HOURLY_BUCKETS;
    for (size_t i = 0; i < max_count; i++) {
        buckets[i] = e->traffic_buckets[(start + i) % HOURLY_BUCKETS];
    }
    *count = max_count;
    return 1;
}

/* Get 20-hour bucket data (120 buckets, one per 10 minutes) */
int se_get_20h(aprs_engine_t* e, uint32_t* buckets, size_t* count) {
    if (!e || !buckets || !count) return 0;

    size_t max_count = *count;
    if (max_count > TWENTYH_BUCKETS) max_count = TWENTYH_BUCKETS;

    uint8_t start = (e->current_10min_bucket + 1) % TWENTYH_BUCKETS;
    for (size_t i = 0; i < max_count; i++) {
        buckets[i] = e->htraffic_buckets[(start + i) % TWENTYH_BUCKETS];
    }
    *count = max_count;
    return 1;
}

/* Tick: rotate buckets on time boundaries.
 * Should be called every second or so.
 */
void se_tick(aprs_engine_t* e) {
    if (!e) return;

    /* Check for minute boundary (rotate per-minute bucket) */
    uint64_t elapsed_since_rotate = e->current_time_ms - e->last_bucket_rotate_ms;
    if (elapsed_since_rotate >= TRAFFIC_UPDATE_MS) {
        e->current_minute_bucket = (e->current_minute_bucket + 1) % HOURLY_BUCKETS;
        e->traffic_buckets[e->current_minute_bucket] = 0;
        e->last_bucket_rotate_ms = e->current_time_ms;

        /* Publish hourly stats */
        eb_publish_simple(e, APRS_EVT_STATS_HOURLY);
    }

    /* Check for 10-minute boundary */
    uint64_t elapsed_10min = e->current_time_ms - e->last_10min_rotate_ms;
    if (elapsed_10min >= (10UL * 60UL * 1000UL)) {
        e->current_10min_bucket = (e->current_10min_bucket + 1) % TWENTYH_BUCKETS;
        e->htraffic_buckets[e->current_10min_bucket] = 0;
        e->last_10min_rotate_ms = e->current_time_ms;

        /* Publish 20h stats */
        eb_publish_simple(e, APRS_EVT_STATS_20H);
    }
}
