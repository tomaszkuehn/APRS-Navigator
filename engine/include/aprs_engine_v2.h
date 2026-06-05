#ifndef APRS_ENGINE_V2_H
#define APRS_ENGINE_V2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque engine handle */
typedef struct aprs_engine aprs_engine_t;

/* --- Status codes --- */
typedef enum {
    APRS_OK = 0,
    APRS_ERR_INVALID_ARG,
    APRS_ERR_NOT_FOUND,
    APRS_ERR_BUSY,
    APRS_ERR_DENIED,
    APRS_ERR_NO_MEMORY,
    APRS_ERR_TIMEOUT,
    APRS_ERR_INTERNAL
} aprs_status_t;

/* --- Position modes --- */
typedef enum {
    APRS_POSITION_AUTO = 0,
    APRS_POSITION_STATIC,
    APRS_POSITION_GPS
} aprs_position_mode_t;

/* --- Digipeater types --- */
typedef enum {
    APRS_DIGI_SIMPLE = 0,
    APRS_DIGI_FLOOD,
    APRS_DIGI_TRACE
} aprs_digi_type_t;

/* --- Message states --- */
typedef enum {
    APRS_MSG_NEW = 0,
    APRS_MSG_SENT,
    APRS_MSG_ACKED,
    APRS_MSG_REJECTED,
    APRS_MSG_READ
} aprs_message_state_t;

/* --- Event types --- */
typedef enum {
    APRS_EVT_FRAME_RX = 0,
    APRS_EVT_FRAME_TX,
    APRS_EVT_FRAME_RAW,
    APRS_EVT_STATION_UPDATED,
    APRS_EVT_STATION_MARKED,
    APRS_EVT_MESSAGE_RECEIVED,
    APRS_EVT_MESSAGE_SENT,
    APRS_EVT_MESSAGE_STATE,
    APRS_EVT_GPS_UPDATED,
    APRS_EVT_POSITION_CHANGED,
    APRS_EVT_POSITION_CAPTURED,
    APRS_EVT_BEACON_SENT,
    APRS_EVT_DIGI_REPEATED,
    APRS_EVT_DIGI_RECENT,
    APRS_EVT_TX_STATE_CHANGED,
    APRS_EVT_CONFIG_CHANGED,
    APRS_EVT_STATS_UPDATED,
    APRS_EVT_STATS_HOURLY,
    APRS_EVT_STATS_20H,
    APRS_EVT_SOUND,
    APRS_EVT_ERROR
} aprs_event_type_t;

/* --- Configuration structure --- */
typedef struct {
    char callsign[16];
    char ssid[8];
    char symbol_table;
    char symbol_code;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
    uint8_t unique_frames_only;
    uint8_t tcpip_enabled;
    uint8_t invalid_frames_enabled;
    uint16_t gps_baudrate;
    aprs_position_mode_t use_position_mode;
    aprs_position_mode_t report_position_mode;
} aprs_engine_config_t;

/* --- Station entry --- */
typedef struct {
    char callsign[16];
    char last_path[64];
    char symbol_table;
    char symbol_code;
    double lat;
    double lon;
    double distance_km;
    double bearing_deg;
    uint32_t packet_count;
    uint64_t last_heard_ms;
    char info[128];
    char comment[128];
    uint8_t has_position;
    uint8_t has_weather;
    uint8_t marked;
    uint8_t frame_type;     /* primary frame type char */
} aprs_station_t;

/* --- Message entry --- */
typedef struct {
    char id[16];
    char src[16];
    char dst[16];
    char text[256];
    aprs_message_state_t state;
    uint64_t ts_ms;
    uint8_t retry_count;
} aprs_message_t;

/* --- Raw frame --- */
typedef struct {
    char raw[512];
    size_t raw_len;
    char src[16];
    char dst[16];
    char path[128];
    uint8_t is_valid;
    uint8_t is_unique;
} aprs_frame_t;

/* --- GPS state --- */
typedef struct {
    uint8_t fix;
    uint8_t valid;
    double lat;
    double lon;
    double speed_kmh;
    double course_deg;
    double altitude_m;
    uint64_t ts_ms;
} aprs_gps_t;

/* --- Statistics --- */
typedef struct {
    uint64_t uptime_ms;
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t digi_repeats;
    uint32_t msg_received;
    uint32_t msg_sent;
    uint32_t invalid_frames;
    uint32_t unique_frames;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
} aprs_stats_t;

/* --- Event --- */
typedef struct {
    aprs_event_type_t type;
    uint64_t ts_ms;
    const char* topic;
    const void* payload;
    size_t payload_size;
} aprs_event_t;

/* --- Subscription callback --- */
typedef void (*aprs_event_cb)(const aprs_event_t* ev, void* user);
typedef uint64_t aprs_subscription_id_t;

/* ========================================================================
 * LIFECYCLE
 * ======================================================================== */

aprs_status_t aprs_engine_create(const aprs_engine_config_t* cfg, aprs_engine_t** out);
aprs_status_t aprs_engine_start(aprs_engine_t* e);
aprs_status_t aprs_engine_stop(aprs_engine_t* e);
void          aprs_engine_destroy(aprs_engine_t* e);

/* Tick: call periodically from platform timer. delta_ms is elapsed since last call. */
aprs_status_t aprs_engine_tick(aprs_engine_t* e, uint64_t delta_ms);

/* Feed a raw APRS frame string into the engine (e.g., "SP3XYZ-9>APRS,WIDE1-1:!5210.12N/01655.22E-Test") */
aprs_status_t aprs_engine_feed_frame(aprs_engine_t* e, const char* raw, size_t len);

/* ========================================================================
 * CONFIGURATION
 * ======================================================================== */

aprs_status_t aprs_engine_set_config(aprs_engine_t* e, const aprs_engine_config_t* cfg);
aprs_status_t aprs_engine_get_config(aprs_engine_t* e, aprs_engine_config_t* out);
aprs_status_t aprs_engine_save_config(aprs_engine_t* e);
aprs_status_t aprs_engine_load_config(aprs_engine_t* e);
aprs_status_t aprs_engine_factory_reset(aprs_engine_t* e);
aprs_status_t aprs_engine_set_callsign(aprs_engine_t* e, const char* callsign);

/* ========================================================================
 * STATION API
 * ======================================================================== */

aprs_status_t aprs_engine_list_stations(aprs_engine_t* e, aprs_station_t* out, size_t* inout_count);
aprs_status_t aprs_engine_get_station(aprs_engine_t* e, const char* callsign, aprs_station_t* out);
aprs_status_t aprs_engine_mark_station(aprs_engine_t* e, const char* callsign, uint8_t mark);
aprs_status_t aprs_engine_sort_stations(aprs_engine_t* e, const char* column, uint8_t ascending);

/* ========================================================================
 * POSITION API
 * ======================================================================== */

aprs_status_t aprs_engine_set_use_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_report_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt);
aprs_status_t aprs_engine_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt);
aprs_status_t aprs_engine_select_position_slot(aprs_engine_t* e, int slot);
aprs_status_t aprs_engine_capture_station_position(aprs_engine_t* e, const char* callsign);
aprs_status_t aprs_engine_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt);

/* ========================================================================
 * BEACON API
 * ======================================================================== */

aprs_status_t aprs_engine_set_status_text(aprs_engine_t* e, int slot, const char* text);
aprs_status_t aprs_engine_set_path(aprs_engine_t* e, int slot, const char* path);
aprs_status_t aprs_engine_select_status_text(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_select_path(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_set_beacon_delay(aprs_engine_t* e, int delay_sec);
aprs_status_t aprs_engine_send_beacon(aprs_engine_t* e);
aprs_status_t aprs_engine_preview_last_tx_frame(aprs_engine_t* e, aprs_frame_t* out);

/* Set SmartBeaconing auto mode: 0=fixed interval, 1=speed-based dynamic interval */
aprs_status_t aprs_engine_set_beacon_auto_mode(aprs_engine_t* e, uint8_t smart_enabled);

/* ========================================================================
 * MESSAGE API
 * ======================================================================== */

aprs_status_t aprs_engine_send_message(aprs_engine_t* e, const char* dest, const char* text);
aprs_status_t aprs_engine_list_messages(aprs_engine_t* e, aprs_message_t* out, size_t* inout_count);
aprs_status_t aprs_engine_get_message(aprs_engine_t* e, const char* id, aprs_message_t* out);
aprs_status_t aprs_engine_set_message_receive_mode(aprs_engine_t* e, uint8_t receive_all);
aprs_status_t aprs_engine_set_message_retry(aprs_engine_t* e, int retries, int delay_sec);
aprs_status_t aprs_engine_mark_message_read(aprs_engine_t* e, const char* id);

/* ========================================================================
 * FRAME EDITOR API
 * ======================================================================== */

aprs_status_t aprs_engine_copy_frame_to_editor(aprs_engine_t* e, const char* callsign);
aprs_status_t aprs_engine_get_editor_frame(aprs_engine_t* e, aprs_frame_t* out);
aprs_status_t aprs_engine_set_editor_frame_text(aprs_engine_t* e, const char* raw);
aprs_status_t aprs_engine_edit_editor_frame(aprs_engine_t* e, size_t offset, uint8_t value);
aprs_status_t aprs_engine_send_editor_frame(aprs_engine_t* e);
aprs_status_t aprs_engine_clear_editor_frame(aprs_engine_t* e);

/* ========================================================================
 * DIGIPEATER API
 * ======================================================================== */

aprs_status_t aprs_engine_set_digi_enabled(aprs_engine_t* e, uint8_t enabled);
aprs_status_t aprs_engine_set_digi_alias(aprs_engine_t* e, int slot, const char* alias,
                                          aprs_digi_type_t type, int max_hops);
aprs_status_t aprs_engine_set_digi_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec);
aprs_status_t aprs_engine_get_recent_digi(aprs_engine_t* e, char* out, size_t out_len);
/* DIGISENS: get list of digipeaters that repeated MY last beacon */
aprs_status_t aprs_engine_get_self_digis(aprs_engine_t* e, char* out, size_t out_len);
aprs_status_t aprs_engine_set_digi_tx_monitor(aprs_engine_t* e, uint8_t enabled);

/* ========================================================================
 * FILTER API
 * ======================================================================== */

aprs_status_t aprs_engine_set_frame_filter(aprs_engine_t* e, uint8_t unique_only,
                                            uint8_t tcpip, uint8_t invalid_ok);
aprs_status_t aprs_engine_set_filter_expression(aprs_engine_t* e, const char* expr);

/* ========================================================================
 * GPS API
 * ======================================================================== */

aprs_status_t aprs_engine_push_nmea(aprs_engine_t* e, const char* sentence);
aprs_status_t aprs_engine_get_gps(aprs_engine_t* e, aprs_gps_t* out);

/* ========================================================================
 * STATISTICS API
 * ======================================================================== */

aprs_status_t aprs_engine_get_stats(aprs_engine_t* e, aprs_stats_t* out);
aprs_status_t aprs_engine_get_hourly_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);
aprs_status_t aprs_engine_get_20h_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);

/* ========================================================================
 * REMOTE CONTROL API
 * ======================================================================== */

aprs_status_t aprs_engine_generate_remote_keys(aprs_engine_t* e, char keys[4][16]);
aprs_status_t aprs_engine_execute_remote_command(aprs_engine_t* e, const char* src, const char* msg);

/* ========================================================================
 * SUBSCRIPTIONS (Event Bus)
 * ======================================================================== */

aprs_subscription_id_t aprs_engine_subscribe(aprs_engine_t* e, const char* topic_filter,
                                              aprs_event_cb cb, void* user);
aprs_status_t aprs_engine_unsubscribe(aprs_engine_t* e, aprs_subscription_id_t id);

/* ========================================================================
 * SNAPSHOT
 * ======================================================================== */

/* Get full engine state as JSON string. out_len is buffer size on input,
 * bytes written (excluding null) on output. Returns APRS_ERR_NO_MEMORY if
 * buffer too small. */
aprs_status_t aprs_engine_get_snapshot(aprs_engine_t* e, char* out_json, size_t* out_len);

/* ========================================================================
 * TRX CONFIGURATION
 * ======================================================================== */

aprs_status_t aprs_engine_set_trx_params(aprs_engine_t* e,
                                          uint8_t tx_delay, uint8_t tx_header,
                                          uint8_t bit_delay, uint8_t rx_tune,
                                          uint8_t squelch);
aprs_status_t aprs_engine_get_trx_params(aprs_engine_t* e,
                                          uint8_t* tx_delay, uint8_t* tx_header,
                                          uint8_t* bit_delay, uint8_t* rx_tune,
                                          uint8_t* squelch);

/* ========================================================================
 * NOTIFICATION / SOUND
 * ======================================================================== */

aprs_status_t aprs_engine_set_sound_level(aprs_engine_t* e, int sound_id, uint8_t level);
aprs_status_t aprs_engine_get_sound_level(aprs_engine_t* e, int sound_id, uint8_t* level);

/* ========================================================================
 * TX LOCK
 * ======================================================================== */

aprs_status_t aprs_engine_lock_tx(aprs_engine_t* e, uint8_t disable_tx,
                                   uint8_t disable_digi, uint8_t disable_query);

/* ========================================================================
 * VERSION
 * ======================================================================== */

const char* aprs_engine_version(void);

#ifdef __cplusplus
}
#endif

#endif /* APRS_ENGINE_V2_H */
