#ifndef APRS_ENGINE_V2_H
#define APRS_ENGINE_V2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aprs_engine aprs_engine_t;

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

typedef enum {
    APRS_POSITION_AUTO = 0,
    APRS_POSITION_STATIC,
    APRS_POSITION_GPS
} aprs_position_mode_t;

typedef enum {
    APRS_DIGI_SIMPLE = 0,
    APRS_DIGI_FLOOD,
    APRS_DIGI_TRACE
} aprs_digi_type_t;

typedef enum {
    APRS_MSG_NEW = 0,
    APRS_MSG_SENT,
    APRS_MSG_ACKED,
    APRS_MSG_REJECTED,
    APRS_MSG_READ
} aprs_message_state_t;

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
} aprs_station_t;

typedef struct {
    char id[16];
    char src[16];
    char dst[16];
    char text[256];
    aprs_message_state_t state;
    uint64_t ts_ms;
    uint8_t retry_count;
} aprs_message_t;

typedef struct {
    char raw[512];
    size_t raw_len;
    char src[16];
    char dst[16];
    char path[128];
    uint8_t is_valid;
    uint8_t is_unique;
} aprs_frame_t;

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

typedef struct {
    aprs_event_type_t type;
    uint64_t ts_ms;
    const char* topic;
    const void* payload;
    size_t payload_size;
} aprs_event_t;

typedef void (*aprs_event_cb)(const aprs_event_t* ev, void* user);
typedef uint64_t aprs_subscription_id_t;

aprs_status_t aprs_engine_create(const aprs_engine_config_t* cfg, aprs_engine_t** out);
aprs_status_t aprs_engine_start(aprs_engine_t* e);
aprs_status_t aprs_engine_stop(aprs_engine_t* e);
void aprs_engine_destroy(aprs_engine_t* e);

aprs_status_t aprs_engine_set_config(aprs_engine_t* e, const aprs_engine_config_t* cfg);
aprs_status_t aprs_engine_get_config(aprs_engine_t* e, aprs_engine_config_t* out);
aprs_status_t aprs_engine_save_config(aprs_engine_t* e);

aprs_status_t aprs_engine_list_stations(aprs_engine_t* e, aprs_station_t* out, size_t* inout_count);
aprs_status_t aprs_engine_get_station(aprs_engine_t* e, const char* callsign, aprs_station_t* out);
aprs_status_t aprs_engine_mark_station(aprs_engine_t* e, const char* callsign, uint8_t mark);
aprs_status_t aprs_engine_sort_stations(aprs_engine_t* e, const char* column, uint8_t ascending);

aprs_status_t aprs_engine_set_use_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_report_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt);
aprs_status_t aprs_engine_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt);
aprs_status_t aprs_engine_select_position_slot(aprs_engine_t* e, int slot);
aprs_status_t aprs_engine_capture_station_position(aprs_engine_t* e, const char* callsign);
aprs_status_t aprs_engine_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt);

aprs_status_t aprs_engine_set_status_text(aprs_engine_t* e, int slot, const char* text);
aprs_status_t aprs_engine_set_path(aprs_engine_t* e, int slot, const char* path);
aprs_status_t aprs_engine_select_status_text(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_select_path(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_set_beacon_delay(aprs_engine_t* e, int delay_sec);
aprs_status_t aprs_engine_send_beacon(aprs_engine_t* e);
aprs_status_t aprs_engine_preview_last_tx_frame(aprs_engine_t* e, aprs_frame_t* out);

aprs_status_t aprs_engine_send_message(aprs_engine_t* e, const char* dest, const char* text);
aprs_status_t aprs_engine_list_messages(aprs_engine_t* e, aprs_message_t* out, size_t* inout_count);
aprs_status_t aprs_engine_set_message_receive_mode(aprs_engine_t* e, uint8_t receive_all);
aprs_status_t aprs_engine_set_message_retry(aprs_engine_t* e, int retries, int delay_sec);
aprs_status_t aprs_engine_mark_message_read(aprs_engine_t* e, const char* id);

aprs_status_t aprs_engine_copy_frame_to_editor(aprs_engine_t* e, const char* frame_id);
aprs_status_t aprs_engine_get_editor_frame(aprs_engine_t* e, aprs_frame_t* out);
aprs_status_t aprs_engine_set_editor_frame_text(aprs_engine_t* e, const char* raw);
aprs_status_t aprs_engine_edit_editor_frame(aprs_engine_t* e, size_t offset, uint8_t value);
aprs_status_t aprs_engine_send_editor_frame(aprs_engine_t* e);
aprs_status_t aprs_engine_clear_editor_frame(aprs_engine_t* e);

aprs_status_t aprs_engine_set_digi_enabled(aprs_engine_t* e, uint8_t enabled);
aprs_status_t aprs_engine_set_digi_alias(aprs_engine_t* e, int slot, const char* alias, aprs_digi_type_t type, int max_hops);
aprs_status_t aprs_engine_set_digi_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec);
aprs_status_t aprs_engine_get_recent_digi(aprs_engine_t* e, char* out, size_t out_len);
aprs_status_t aprs_engine_set_digi_tx_monitor(aprs_engine_t* e, uint8_t enabled);

aprs_status_t aprs_engine_set_frame_filter(aprs_engine_t* e, uint8_t unique_only, uint8_t tcpip, uint8_t invalid_ok);
aprs_status_t aprs_engine_set_filter_expression(aprs_engine_t* e, const char* expr);

aprs_status_t aprs_engine_push_nmea(aprs_engine_t* e, const char* sentence);
aprs_status_t aprs_engine_get_gps(aprs_engine_t* e, aprs_gps_t* out);

aprs_status_t aprs_engine_get_stats(aprs_engine_t* e, aprs_stats_t* out);
aprs_status_t aprs_engine_get_hourly_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);
aprs_status_t aprs_engine_get_20h_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);

aprs_status_t aprs_engine_generate_remote_keys(aprs_engine_t* e, char keys[4][16]);
aprs_status_t aprs_engine_execute_remote_command(aprs_engine_t* e, const char* src, const char* msg);

aprs_subscription_id_t aprs_engine_subscribe(aprs_engine_t* e, const char* topic_filter, aprs_event_cb cb, void* user);
aprs_status_t aprs_engine_unsubscribe(aprs_engine_t* e, aprs_subscription_id_t id);

#ifdef __cplusplus
}
#endif

#endif
