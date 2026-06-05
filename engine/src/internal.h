#ifndef APRS_ENGINE_INTERNAL_H
#define APRS_ENGINE_INTERNAL_H

#include "aprs_engine_v2.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * INTERNAL CONSTANTS
 * ======================================================================== */

#define MAX_STATIONS      151
#define MAX_RX_MESSAGES    20
#define MAX_TX_MESSAGES     8
#define MAX_DIGI_QUEUE     30
#define MAX_ALIASES         4
#define MAX_PATHS           3
#define MAX_STATUS_TEXTS    3
#define MAX_POS_SLOTS       3
#define MAX_REMOTE_KEYS     4
#define MAX_PREDEFINED_MSGS 5
#define MAX_SOUNDS          7
#define MAX_RECENT_DIGI     6
#define MAX_UNIQUE_CHECK   32
#define MAX_FRAME_RAW     512
#define MAX_INFO_LEN      128
#define MAX_COMMENT_LEN   128
#define MAX_PATH_LEN       64
#define MAX_ALIAS_LEN       8
#define MAX_TEXT_LEN       48
#define MAX_SUBSCRIBERS    64
#define MAX_CMD_QUEUE      64
#define MAX_FRAME_DATA    256
#define MAX_UDEST_LEN       8

#define HOURLY_BUCKETS      60
#define TWENTYH_BUCKETS    120
#define TRAFFIC_UPDATE_MS 60000

/* Sound event IDs */
#define SND_STATION_RX    0
#define SND_SELF_RX       1
#define SND_MARKED_RX     2
#define SND_MESSAGE_RX    3
#define SND_BEACON_TX     4
#define SND_DIGI_TX       5
#define SND_ERROR         6

/* Filter masks */
#define FILTER_DIRECT     0x02
#define FILTER_POSITION   0x04
#define FILTER_NOPOSITION 0x08
#define FILTER_VIADIGI    0x10
#define FILTER_WX         0x20
#define FILTER_DIGI       0x40
#define FILTER_GATE       0x80

/* Frame type characters (APRS data type identifiers) */
#define FRAME_TYPE_MESSAGE    ':'
#define FRAME_TYPE_POSITION   '!'
#define FRAME_TYPE_POS_COMP   '='
#define FRAME_TYPE_POS_COMP2  '/'
#define FRAME_TYPE_POS_COMP3  '@'
#define FRAME_TYPE_STATUS     '>'
#define FRAME_TYPE_QUERY      '?'
#define FRAME_TYPE_ITEM       ')'
#define FRAME_TYPE_OBJECT     ';'
#define FRAME_TYPE_WX         '_'
#define FRAME_TYPE_WX2        '#'
#define FRAME_TYPE_WX3        '*'
#define FRAME_TYPE_MICE       '`'
#define FRAME_TYPE_MICE2      '\''
#define FRAME_TYPE_THIRDPARTY '}'

/* APRS MIC-E message types from destination field */
#define MICE_MESSAGE_OFF_DUTY   0
#define MICE_MESSAGE_EN_ROUTE   1
#define MICE_MESSAGE_IN_SERVICE 2
#define MICE_MESSAGE_RETURNING  3
#define MICE_MESSAGE_SPECIAL    4
#define MICE_MESSAGE_PRIORITY   5
#define MICE_MESSAGE_EMERGENCY  6

/* Config persistence magic */
#define CONFIG_MAGIC  0x41505253  /* "APRS" */
#define CONFIG_VERSION 2

/* ========================================================================
 * INTERNAL TYPE DEFINITIONS
 * ======================================================================== */

typedef struct {
    char text[48];
    uint8_t enabled;
} beacon_status_t;

typedef struct {
    char path[40];
    uint8_t enabled;
    uint8_t msg_enabled;
} beacon_path_t;

typedef struct {
    char alias[8];
    aprs_digi_type_t type;
    uint8_t max_hops;
    uint8_t enabled;
} digi_alias_t;

typedef struct {
    char frame_data[256];
    uint8_t slot;
    uint64_t repeat_time_ms;
    uint8_t status;      /* 0=free, 1=waiting, 2=repeated */
    char alias[8];
    uint8_t alias_pos;
    uint8_t crc;
    uint8_t station_id;
} digi_queue_entry_t;

typedef struct {
    double lat;
    double lon;
    double alt;
} position_slot_t;

typedef struct {
    uint32_t key;
    uint8_t enabled;
} remote_key_t;

typedef struct {
    char text[56];
} predefined_msg_t;

typedef struct {
    uint8_t level;  /* 0-9, 0=off */
} sound_map_t;

typedef struct {
    uint8_t tx_delay;
    uint8_t tx_header;
    uint8_t bit_delay;
    uint8_t rx_tune;
    uint8_t squelch;
} trx_params_t;

typedef struct {
    char callsign[16];
    uint8_t last_crc;
    uint64_t last_time_ms;
} unique_check_t;

/* Frame classification result */
typedef enum {
    FRAME_CLASS_UNKNOWN = 0,
    FRAME_CLASS_POSITION,
    FRAME_CLASS_MESSAGE,
    FRAME_CLASS_STATUS,
    FRAME_CLASS_WX,
    FRAME_CLASS_QUERY,
    FRAME_CLASS_OBJECT,
    FRAME_CLASS_ITEM,
    FRAME_CLASS_MIC_E,
    FRAME_CLASS_THIRDPARTY
} frame_class_t;

/* Command queue entry */
typedef enum {
    CMD_NONE = 0,
    CMD_SET_CONFIG,
    CMD_SET_CALLSIGN,
    CMD_SET_POSITION,
    CMD_SET_POSITION_MODE,
    CMD_SEND_BEACON,
    CMD_SEND_MESSAGE,
    CMD_LOCK_TX,
    CMD_SET_FILTER,
    CMD_FACTORY_RESET,
    CMD_MARK_STATION,
    CMD_SORT_STATIONS,
    CMD_SET_USE_POS_MODE,
    CMD_SET_REPORT_POS_MODE,
    CMD_SET_STATIC_POS,
    CMD_SELECT_POS_SLOT,
    CMD_CAPTURE_POS,
    CMD_SET_MANUAL_POS,
    CMD_SET_STATUS_TEXT,
    CMD_SET_PATH,
    CMD_SELECT_STATUS,
    CMD_SELECT_PATH,
    CMD_SET_BEACON_DELAY,
    CMD_SET_MSG_RECV_MODE,
    CMD_SET_MSG_RETRY,
    CMD_MARK_MSG_READ,
    CMD_COPY_FRAME_EDITOR,
    CMD_SET_EDITOR_TEXT,
    CMD_EDIT_EDITOR_BYTE,
    CMD_SEND_EDITOR_FRAME,
    CMD_CLEAR_EDITOR_FRAME,
    CMD_SET_DIGI_ENABLED,
    CMD_SET_DIGI_ALIAS,
    CMD_SET_DIGI_TIMING,
    CMD_SET_DIGI_TX_MONITOR,
    CMD_SET_FRAME_FILTER,
    CMD_SET_FILTER_EXPR,
    CMD_PUSH_NMEA,
    CMD_GENERATE_REMOTE_KEYS,
    CMD_EXEC_REMOTE_CMD,
    CMD_SET_TRX_PARAMS,
    CMD_SET_SOUND_LEVEL,
    CMD_SAVE_CONFIG,
    CMD_LOAD_CONFIG
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    union {
        struct { aprs_engine_config_t cfg; } set_config;
        struct { char callsign[16]; } set_callsign;
        struct { double lat; double lon; double alt; } set_position;
        struct { aprs_position_mode_t mode; } set_position_mode;
        struct { int path_idx; int status_idx; } send_beacon;
        struct { char dest[16]; char text[256]; } send_message;
        struct { uint8_t disable_tx; uint8_t disable_digi; uint8_t disable_query; } lock_tx;
        struct { char expr[64]; } set_filter;
        struct { char callsign[16]; uint8_t mark; } mark_station;
        struct { char column[16]; uint8_t ascending; } sort_stations;
        struct { aprs_position_mode_t mode; } set_use_pos_mode;
        struct { aprs_position_mode_t mode; } set_report_pos_mode;
        struct { int slot; double lat; double lon; double alt; } set_static_pos;
        struct { int slot; } select_pos_slot;
        struct { char callsign[16]; } capture_pos;
        struct { double lat; double lon; double alt; } set_manual_pos;
        struct { int slot; char text[48]; } set_status_text;
        struct { int slot; char path[40]; } set_path;
        struct { int slot; uint8_t enabled; } select_status;
        struct { int slot; uint8_t enabled; } select_path;
        struct { int delay_sec; } set_beacon_delay;
        struct { uint8_t receive_all; } set_msg_recv_mode;
        struct { int retries; int delay_sec; } set_msg_retry;
        struct { char id[16]; } mark_msg_read;
        struct { char callsign[16]; } copy_frame_editor;
        struct { char raw[512]; } set_editor_text;
        struct { size_t offset; uint8_t value; } edit_editor_byte;
        struct { uint8_t enabled; } set_digi_enabled;
        struct { int slot; char alias[8]; aprs_digi_type_t type; int max_hops; } set_digi_alias;
        struct { int dupetime_sec; int dupedelay_sec; } set_digi_timing;
        struct { uint8_t enabled; } set_digi_tx_monitor;
        struct { uint8_t unique_only; uint8_t tcpip; uint8_t invalid_ok; } set_frame_filter;
        struct { char expr[64]; } set_filter_expr;
        struct { char sentence[128]; } push_nmea;
        struct { char src[16]; char msg[256]; } exec_remote_cmd;
        struct { uint8_t tx_delay; uint8_t tx_header; uint8_t bit_delay; uint8_t rx_tune; uint8_t squelch; } set_trx;
        struct { int sound_id; uint8_t level; } set_sound;
    } data;
} cmd_entry_t;

/* Event subscription */
typedef struct event_subscription {
    uint64_t id;
    char topic_filter[64];
    aprs_event_cb callback;
    void* user_data;
    uint8_t active;
} event_subscription_t;

/* ========================================================================
 * THE ENGINE STRUCT (opaque to users)
 * ======================================================================== */

typedef struct aprs_engine {
    /* --- Configuration --- */
    aprs_engine_config_t config;
    beacon_status_t status_texts[3];
    beacon_path_t paths[3];
    digi_alias_t aliases[4];
    position_slot_t pos_slots[3];
    remote_key_t remote_keys[4];
    predefined_msg_t predefined_msgs[5];
    sound_map_t sounds[7];
    trx_params_t trx;

    /* --- Core identity --- */
    char udest[8];
    uint8_t path_use;
    uint8_t msg_path_use;
    uint8_t status_use;
    uint8_t current_status_idx;
    uint8_t current_path_idx;
    uint8_t nav_mark_enabled;

    /* --- Position --- */
    double my_lat, my_lon, my_alt;
    int my_course;
    int active_pos_slot;
    aprs_position_mode_t use_mode;
    aprs_position_mode_t report_mode;

    /* --- GPS --- */
    aprs_gps_t gps;
    uint8_t gps_present;
    uint8_t gps_enabled;
    uint8_t gps_fix_counter;   /* decremented by tick; GPS ok while > 0 */
    uint8_t gps_data_counter;  /* decremented by tick; data flowing while > 0 */

    /* --- Stations --- */
    aprs_station_t stations[151];
    uint16_t sort_order[151];
    uint16_t num_stations;
    uint16_t marked_station_idx;
    uint16_t filtered_count;
    char sort_column[16];
    uint8_t sort_ascending;
    uint8_t filter_include_mask;
    uint8_t filter_exclude_mask;

    /* --- Messages --- */
    aprs_message_t rx_messages[20];
    aprs_message_t tx_messages[8];
    uint8_t rx_msg_count;
    uint8_t tx_msg_count;
    uint8_t receive_all_msgs;
    uint8_t msg_retries;
    uint8_t msg_retry_delay_sec;
    uint32_t msg_next_id;  /* auto-increment for message IDs */

    /* --- Beacon --- */
    uint64_t next_beacon_time_ms;
    uint32_t beacon_delay_sec;       /* configured max interval */
    uint8_t beacon_auto_enabled;
    uint8_t beacon_digi_sensitivity; /* 0=off, 1=digisens retry enabled */
    uint8_t beacon_auto_smart;       /* 1=SmartBeaconing speed-based */
    char last_beacon_raw[512];
    size_t last_beacon_len;
    /* SmartBeaconing state */
    double  sb_last_lat, sb_last_lon;
    uint64_t sb_last_beacon_ms;
    uint32_t sb_min_interval_ms;

    /* --- Digi --- */
    digi_queue_entry_t digi_queue[30];
    uint8_t digi_queue_count;
    uint32_t digi_dupe_time_sec;
    uint32_t digi_dupe_delay_sec;
    uint8_t digi_tx_monitor;
    char recent_digi[6][10];
    uint8_t recent_digi_count;

    /* Digisens — stations that digipeated MY beacon */
    char self_digis[6][10];
    uint8_t self_digis_count;
    uint64_t last_beacon_sent_ms;
    uint8_t digisens_retry_pending;

    /* --- Filters --- */
    uint8_t filter_unique_only;
    uint8_t filter_tcpip;
    uint8_t filter_invalid_ok;
    char filter_expr[64];
    uint8_t announce_filter;

    /* --- Stats --- */
    aprs_stats_t stats;
    uint32_t traffic_buckets[60];
    uint32_t htraffic_buckets[120];
    uint64_t last_bucket_rotate_ms;
    uint64_t last_10min_rotate_ms;
    uint8_t current_minute_bucket;
    uint8_t current_10min_bucket;

    /* --- Raw frame editor --- */
    char editor_frame_raw[512];
    size_t editor_frame_len;

    /* --- Unique frame tracking --- */
    unique_check_t unique_checks[32];
    uint8_t unique_check_count;

    /* --- Runtime --- */
    uint8_t running;
    uint64_t start_time_ms;
    uint64_t current_time_ms;

    /* --- Event bus --- */
    event_subscription_t subscribers[64];
    uint32_t subscriber_count;
    uint64_t sub_id_counter;

    /* --- Command queue --- */
    cmd_entry_t cmd_queue[64];
    uint32_t cmd_queue_head;
    uint32_t cmd_queue_tail;
    uint32_t cmd_queue_count;

    /* --- Mutex --- */
#ifdef _WIN32
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
} aprs_engine_t;

/* ========================================================================
 * INTERNAL MODULE FUNCTION DECLARATIONS
 * ======================================================================== */

/* event_bus.c */
void     eb_init(aprs_engine_t* e);
uint64_t eb_subscribe(aprs_engine_t* e, const char* topic_filter, aprs_event_cb cb, void* user);
int      eb_unsubscribe(aprs_engine_t* e, uint64_t id);
void     eb_publish(aprs_engine_t* e, aprs_event_type_t type, const char* topic, const void* payload, size_t payload_size);
int      eb_topic_match(const char* filter, const char* topic);
void     eb_publish_simple(aprs_engine_t* e, aprs_event_type_t type);
void     eb_publish_station_updated(aprs_engine_t* e, const aprs_station_t* st);
void     eb_publish_frame_rx(aprs_engine_t* e, const aprs_frame_t* f);
void     eb_publish_error(aprs_engine_t* e, const char* message);
const char* eb_topic_for_type(aprs_event_type_t type);

/* frame_ingest.c */
int          fi_parse_frame(const char* raw, size_t len, aprs_frame_t* out);
frame_class_t fi_classify_frame(const aprs_frame_t* f);
int          fi_validate_checksum(const char* raw, size_t len);
int          fi_is_unique(aprs_engine_t* e, const char* src, uint8_t crc);
int          fi_apply_filters(aprs_engine_t* e, const aprs_frame_t* f);
int          fi_encode_position(double lat, double lon, char sym_table, char sym_code, char* out, size_t out_len);
int          fi_decode_mic_e(const char* raw, size_t len, double* lat, double* lon, char* info);
int          fi_decode_position(const char* payload, size_t len, double* lat, double* lon, char* sym_table, char* sym_code);
void         fi_register_unique(aprs_engine_t* e, const char* src, uint8_t crc);
uint8_t      fi_compute_crc(const char* data, size_t len);

/* station_manager.c */
int16_t sm_find_station(aprs_engine_t* e, const char* callsign);
int16_t sm_find_or_create(aprs_engine_t* e, const char* callsign);
void    sm_update_from_frame(aprs_engine_t* e, uint16_t idx, const aprs_frame_t* f, frame_class_t fc);
void    sm_sort(aprs_engine_t* e, const char* column, uint8_t ascending);
void    sm_apply_filter_mask(aprs_engine_t* e);
void    sm_mark(aprs_engine_t* e, const char* callsign, uint8_t mark);
void    sm_clear_all_marks(aprs_engine_t* e);
void    sm_get_distance_bearing(double lat1, double lon1, double lat2, double lon2,
                                double* dist_km, double* bearing_deg);
void    sm_recalc_all(aprs_engine_t* e);

/* position_manager.c */
void pm_set_use_mode(aprs_engine_t* e, aprs_position_mode_t mode);
void pm_set_report_mode(aprs_engine_t* e, aprs_position_mode_t mode);
int  pm_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt);
int  pm_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt);
void pm_select_slot(aprs_engine_t* e, int slot);
int  pm_capture_station(aprs_engine_t* e, const char* callsign);
void pm_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt);
void pm_get_effective_position(aprs_engine_t* e, double* lat, double* lon);
void pm_get_effective_report_position(aprs_engine_t* e, double* lat, double* lon);

/* beacon_manager.c */
void bm_set_status_text(aprs_engine_t* e, int slot, const char* text);
void bm_set_path(aprs_engine_t* e, int slot, const char* path);
void bm_select_status(aprs_engine_t* e, int slot, uint8_t enabled);
void bm_select_path(aprs_engine_t* e, int slot, uint8_t enabled);
void bm_set_delay(aprs_engine_t* e, int delay_sec);
void bm_set_auto_mode(aprs_engine_t* e, uint8_t smart_enabled);
int  bm_send_beacon(aprs_engine_t* e);
int  bm_compose_beacon(aprs_engine_t* e, char* out, size_t* out_len);
void bm_tick(aprs_engine_t* e);
void bm_preview_last(aprs_engine_t* e, aprs_frame_t* out);
uint32_t bm_compute_smart_interval(aprs_engine_t* e);

/* message_manager.c */
int  mm_receive_message(aprs_engine_t* e, const char* src, const char* dst, const char* text, const char* msg_id);
int  mm_send_message(aprs_engine_t* e, const char* dest, const char* text);
void mm_process_ack(aprs_engine_t* e, const char* msg_id);
void mm_process_rej(aprs_engine_t* e, const char* msg_id);
void mm_retry_tick(aprs_engine_t* e);
void mm_set_receive_mode(aprs_engine_t* e, uint8_t receive_all);
void mm_set_retry(aprs_engine_t* e, int retries, int delay_sec);
int  mm_mark_read(aprs_engine_t* e, const char* id);
int  mm_find_message(aprs_engine_t* e, const char* id);

/* digi_engine.c */
void de_set_enabled(aprs_engine_t* e, uint8_t enabled);
void de_set_alias(aprs_engine_t* e, int slot, const char* alias, aprs_digi_type_t type, int max_hops);
void de_set_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec);
void de_set_tx_monitor(aprs_engine_t* e, uint8_t enabled);
int  de_get_recent(aprs_engine_t* e, char* out, size_t out_len);
int  de_get_self_digis(aprs_engine_t* e, char* out, size_t out_len);
void de_check_and_queue(aprs_engine_t* e, const aprs_frame_t* f);
void de_tick(aprs_engine_t* e);
void de_check_own_digi(aprs_engine_t* e, const char* src, const char* path);

/* gps_manager.c */
void gm_init(aprs_engine_t* e);
int  gm_push_nmea(aprs_engine_t* e, const char* sentence);
void gm_get_state(aprs_engine_t* e, aprs_gps_t* out);
void gm_set_enabled(aprs_engine_t* e, uint8_t enabled);
void gm_tick(aprs_engine_t* e);

/* stats_engine.c */
void se_record_rx(aprs_engine_t* e);
void se_record_tx(aprs_engine_t* e);
void se_record_digi(aprs_engine_t* e);
void se_record_invalid(aprs_engine_t* e);
void se_record_unique(aprs_engine_t* e);
void se_record_msg_rx(aprs_engine_t* e);
void se_record_msg_tx(aprs_engine_t* e);
void se_get_stats(aprs_engine_t* e, aprs_stats_t* out);
int  se_get_hourly(aprs_engine_t* e, uint32_t* buckets, size_t* count);
int  se_get_20h(aprs_engine_t* e, uint32_t* buckets, size_t* count);
void se_tick(aprs_engine_t* e);

/* raw_frame_editor.c */
int  rfe_copy_from_station(aprs_engine_t* e, const char* callsign);
void rfe_get(aprs_engine_t* e, aprs_frame_t* out);
void rfe_set_text(aprs_engine_t* e, const char* raw);
void rfe_edit_byte(aprs_engine_t* e, size_t offset, uint8_t value);
int  rfe_send(aprs_engine_t* e);
void rfe_clear(aprs_engine_t* e);

/* remote_control.c */
void rc_generate_keys(aprs_engine_t* e, char keys[4][16]);
int  rc_execute_command(aprs_engine_t* e, const char* src, const char* msg);

/* trx_config.c */
void trx_set_params(aprs_engine_t* e, uint8_t tx_delay, uint8_t tx_header,
                    uint8_t bit_delay, uint8_t rx_tune, uint8_t squelch);
void trx_get_params(aprs_engine_t* e, uint8_t* tx_delay, uint8_t* tx_header,
                    uint8_t* bit_delay, uint8_t* rx_tune, uint8_t* squelch);

/* notification_mgr.c */
void nm_set_sound(aprs_engine_t* e, int sound_id, uint8_t level);
void nm_get_sound(aprs_engine_t* e, int sound_id, uint8_t* level);
void nm_trigger(aprs_engine_t* e, int sound_id);

/* config_persist.c */
int  cp_save(aprs_engine_t* e, const char* filepath);
int  cp_load(aprs_engine_t* e, const char* filepath);
void cp_reset_factory(aprs_engine_t* e);

/* aprs_engine.c (internal) */
void engine_process_commands(aprs_engine_t* e);
int  engine_enqueue_cmd(aprs_engine_t* e, const cmd_entry_t* cmd);
void engine_dispatch_frame(aprs_engine_t* e, const char* raw, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* APRS_ENGINE_INTERNAL_H */
