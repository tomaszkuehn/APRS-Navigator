#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

/* ========================================================================
 * EVENT BUS — Internal publish/subscribe with topic filtering
 * ======================================================================== */

void eb_init(aprs_engine_t* e) {
    memset(e->subscribers, 0, sizeof(e->subscribers));
    e->subscriber_count = 0;
    e->sub_id_counter = 1;
}

/*
 * Simple glob match: topic_filter matches topic.
 * Supported patterns:
 *   "*"         matches everything
 *   "station.*" matches "station.updated", "station.marked", etc.
 *   "frame.*"   matches "frame.rx", "frame.tx", "frame.raw"
 *   exact       matches only exact topic string
 *
 * Only supports single '*' at end of a segment.
 */
int eb_topic_match(const char* filter, const char* topic) {
    if (!filter || !topic) return 0;
    if (filter[0] == '*' && filter[1] == '\0') return 1;

    while (*filter && *topic) {
        if (*filter == '*') {
            /* '*' matches rest of current segment */
            if (*(filter + 1) == '\0') return 1;  /* trailing * matches all */
            /* skip topic until we match character after * */
            while (*topic && *topic != *(filter + 1)) topic++;
            if (*topic == '\0') return 0;
            filter++; /* skip the * */
            continue;
        }
        if (*filter != *topic) return 0;
        filter++;
        topic++;
    }

    /* Both must reach end simultaneously */
    if (*filter == '*' && *(filter + 1) == '\0') return 1;
    return (*filter == '\0' && *topic == '\0');
}

uint64_t eb_subscribe(aprs_engine_t* e, const char* topic_filter, aprs_event_cb cb, void* user) {
    if (!e || !topic_filter || !cb) return 0;
    if (e->subscriber_count >= MAX_SUBSCRIBERS) return 0;

    uint32_t idx = e->subscriber_count;
    e->subscribers[idx].id = e->sub_id_counter++;
    strncpy(e->subscribers[idx].topic_filter, topic_filter, sizeof(e->subscribers[idx].topic_filter) - 1);
    e->subscribers[idx].topic_filter[sizeof(e->subscribers[idx].topic_filter) - 1] = '\0';
    e->subscribers[idx].callback = cb;
    e->subscribers[idx].user_data = user;
    e->subscribers[idx].active = 1;
    e->subscriber_count++;

    return e->subscribers[idx].id;
}

int eb_unsubscribe(aprs_engine_t* e, uint64_t id) {
    if (!e || id == 0) return 0;

    for (uint32_t i = 0; i < e->subscriber_count; i++) {
        if (e->subscribers[i].id == id && e->subscribers[i].active) {
            e->subscribers[i].active = 0;
            return 1;
        }
    }
    return 0;
}

void eb_publish(aprs_engine_t* e, aprs_event_type_t type, const char* topic,
                const void* payload, size_t payload_size) {
    if (!e || !topic) return;

    aprs_event_t ev;
    ev.type = type;
    ev.ts_ms = e->current_time_ms;
    ev.topic = topic;
    ev.payload = payload;
    ev.payload_size = payload_size;

    for (uint32_t i = 0; i < e->subscriber_count; i++) {
        if (!e->subscribers[i].active) continue;
        if (!eb_topic_match(e->subscribers[i].topic_filter, topic)) continue;
        e->subscribers[i].callback(&ev, e->subscribers[i].user_data);
    }
}

/* ========================================================================
 * Convenience helpers — publish typed events with payload construction
 * ======================================================================== */

/* Helper to get topic string from event type */
static const char* event_topic_strings[] = {
    "frame.rx",           /* APRS_EVT_FRAME_RX */
    "frame.tx",           /* APRS_EVT_FRAME_TX */
    "frame.raw",          /* APRS_EVT_FRAME_RAW */
    "station.updated",    /* APRS_EVT_STATION_UPDATED */
    "station.marked",     /* APRS_EVT_STATION_MARKED */
    "message.received",   /* APRS_EVT_MESSAGE_RECEIVED */
    "message.sent",       /* APRS_EVT_MESSAGE_SENT */
    "message.state",      /* APRS_EVT_MESSAGE_STATE */
    "gps.updated",        /* APRS_EVT_GPS_UPDATED */
    "position.changed",   /* APRS_EVT_POSITION_CHANGED */
    "position.captured",  /* APRS_EVT_POSITION_CAPTURED */
    "beacon.sent",        /* APRS_EVT_BEACON_SENT */
    "digi.repeated",      /* APRS_EVT_DIGI_REPEATED */
    "digi.recent",        /* APRS_EVT_DIGI_RECENT */
    "tx.state",           /* APRS_EVT_TX_STATE_CHANGED */
    "config.changed",     /* APRS_EVT_CONFIG_CHANGED */
    "stats.updated",      /* APRS_EVT_STATS_UPDATED */
    "stats.hourly",       /* APRS_EVT_STATS_HOURLY */
    "stats.20h",          /* APRS_EVT_STATS_20H */
    "sound.event",        /* APRS_EVT_SOUND */
    "error",              /* APRS_EVT_ERROR */
};

const char* eb_topic_for_type(aprs_event_type_t type) {
    if (type <= APRS_EVT_ERROR) {
        return event_topic_strings[type];
    }
    return "unknown";
}

/* Publish a simple event with no payload */
void eb_publish_simple(aprs_engine_t* e, aprs_event_type_t type) {
    eb_publish(e, type, eb_topic_for_type(type), NULL, 0);
}

/* Publish a station.updated event */
void eb_publish_station_updated(aprs_engine_t* e, const aprs_station_t* st) {
    eb_publish(e, APRS_EVT_STATION_UPDATED, "station.updated", st, sizeof(aprs_station_t));
}

/* Publish a frame.rx event */
void eb_publish_frame_rx(aprs_engine_t* e, const aprs_frame_t* f) {
    eb_publish(e, APRS_EVT_FRAME_RX, "frame.rx", f, sizeof(aprs_frame_t));
}

/* Publish an error event */
void eb_publish_error(aprs_engine_t* e, const char* message) {
    eb_publish(e, APRS_EVT_ERROR, "error", message, message ? strlen(message) + 1 : 0);
}
