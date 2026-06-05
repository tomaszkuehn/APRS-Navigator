#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * CONFIG PERSISTENCE — Binary config save/load with magic+version+checksum
 *
 * Extracted from:
 *   - globals.c (update_config, globals_initialize, restore_factory)
 *   - eeprom.c (flash_read, flash_write)
 *
 * Format (binary):
 *   uint32_t magic        = 0x41505253 ("APRS")
 *   uint32_t version      = 2
 *   uint32_t data_size    = sizeof(wire_config_t)
 *   uint32_t checksum     = CRC32 of data
 *   uint8_t  data[]       = wire_config_t
 *
 * This fixes the original flash corruption problem (no magic/version/checksum).
 * ======================================================================== */

/* Wire format — packed, fixed-size, no pointers */
#pragma pack(push, 1)
typedef struct {
    /* Config */
    char     callsign[16];
    char     ssid[8];
    char     symbol_table;
    char     symbol_code;
    uint8_t  tx_enabled;
    uint8_t  digi_enabled;
    uint8_t  beacon_enabled;
    uint8_t  unique_frames_only;
    uint8_t  tcpip_enabled;
    uint8_t  invalid_frames_enabled;
    uint16_t gps_baudrate;
    uint8_t  use_position_mode;
    uint8_t  report_position_mode;

    /* Core state */
    char     udest[8];
    uint8_t  path_use;
    uint8_t  msg_path_use;
    uint8_t  status_use;
    uint8_t  nav_mark_enabled;

    /* Position */
    double   pos_slot_lat[3];
    double   pos_slot_lon[3];
    double   pos_slot_alt[3];
    int      active_pos_slot;
    uint8_t  use_mode;
    uint8_t  report_mode;

    /* Beacon */
    beacon_status_t status_texts[3];
    beacon_path_t   paths[3];
    uint32_t beacon_delay_sec;
    uint8_t  beacon_auto_enabled;
    uint8_t  beacon_digi_sensitivity;

    /* Message */
    uint8_t  receive_all_msgs;
    uint8_t  msg_retries;
    uint8_t  msg_retry_delay_sec;

    /* Digi */
    digi_alias_t aliases[4];
    uint32_t digi_dupe_time_sec;
    uint32_t digi_dupe_delay_sec;
    uint8_t  digi_tx_monitor;

    /* Filters */
    uint8_t  filter_unique_only;
    uint8_t  filter_tcpip;
    uint8_t  filter_invalid_ok;
    char     filter_expr[64];
    uint8_t  announce_filter;
    uint8_t  filter_include_mask;
    uint8_t  filter_exclude_mask;

    /* TRX */
    uint8_t  trx_tx_delay;
    uint8_t  trx_tx_header;
    uint8_t  trx_bit_delay;
    uint8_t  trx_rx_tune;
    uint8_t  trx_squelch;

    /* Sounds */
    uint8_t  sound_levels[7];

    /* Predefined messages */
    char     predefined_msgs[5][56];

    /* Reserve space for future expansion */
    uint8_t  reserved[64];
} wire_config_t;
#pragma pack(pop)

/* Simple CRC32 for data integrity */
static uint32_t cp_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

/* Write engine config to wire format */
static void cp_engine_to_wire(aprs_engine_t* e, wire_config_t* w) {
    memset(w, 0, sizeof(*w));

    /* Config */
    memcpy(w->callsign, e->config.callsign, 16);
    memcpy(w->ssid, e->config.ssid, 8);
    w->symbol_table = e->config.symbol_table;
    w->symbol_code = e->config.symbol_code;
    w->tx_enabled = e->config.tx_enabled;
    w->digi_enabled = e->config.digi_enabled;
    w->beacon_enabled = e->config.beacon_enabled;
    w->unique_frames_only = e->config.unique_frames_only;
    w->tcpip_enabled = e->config.tcpip_enabled;
    w->invalid_frames_enabled = e->config.invalid_frames_enabled;
    w->gps_baudrate = e->config.gps_baudrate;
    w->use_position_mode = (uint8_t)e->config.use_position_mode;
    w->report_position_mode = (uint8_t)e->config.report_position_mode;

    /* Core state */
    memcpy(w->udest, e->udest, 8);
    w->path_use = e->path_use;
    w->msg_path_use = e->msg_path_use;
    w->status_use = e->status_use;
    w->nav_mark_enabled = e->nav_mark_enabled;

    /* Positions */
    for (int i = 0; i < 3; i++) {
        w->pos_slot_lat[i] = e->pos_slots[i].lat;
        w->pos_slot_lon[i] = e->pos_slots[i].lon;
        w->pos_slot_alt[i] = e->pos_slots[i].alt;
    }
    w->active_pos_slot = e->active_pos_slot;
    w->use_mode = (uint8_t)e->use_mode;
    w->report_mode = (uint8_t)e->report_mode;

    /* Beacon */
    memcpy(w->status_texts, e->status_texts, sizeof(w->status_texts));
    memcpy(w->paths, e->paths, sizeof(w->paths));
    w->beacon_delay_sec = e->beacon_delay_sec;
    w->beacon_auto_enabled = e->beacon_auto_enabled;
    w->beacon_digi_sensitivity = e->beacon_digi_sensitivity;

    /* Message */
    w->receive_all_msgs = e->receive_all_msgs;
    w->msg_retries = e->msg_retries;
    w->msg_retry_delay_sec = e->msg_retry_delay_sec;

    /* Digi */
    memcpy(w->aliases, e->aliases, sizeof(w->aliases));
    w->digi_dupe_time_sec = e->digi_dupe_time_sec;
    w->digi_dupe_delay_sec = e->digi_dupe_delay_sec;
    w->digi_tx_monitor = e->digi_tx_monitor;

    /* Filters */
    w->filter_unique_only = e->filter_unique_only;
    w->filter_tcpip = e->filter_tcpip;
    w->filter_invalid_ok = e->filter_invalid_ok;
    memcpy(w->filter_expr, e->filter_expr, 64);
    w->announce_filter = e->announce_filter;
    w->filter_include_mask = e->filter_include_mask;
    w->filter_exclude_mask = e->filter_exclude_mask;

    /* TRX */
    w->trx_tx_delay = e->trx.tx_delay;
    w->trx_tx_header = e->trx.tx_header;
    w->trx_bit_delay = e->trx.bit_delay;
    w->trx_rx_tune = e->trx.rx_tune;
    w->trx_squelch = e->trx.squelch;

    /* Sounds */
    for (int i = 0; i < 7; i++) {
        w->sound_levels[i] = e->sounds[i].level;
    }

    /* Predefined messages */
    memcpy(w->predefined_msgs, e->predefined_msgs, sizeof(w->predefined_msgs));
}

/* Read engine config from wire format */
static void cp_wire_to_engine(aprs_engine_t* e, const wire_config_t* w) {
    /* Config */
    memcpy(e->config.callsign, w->callsign, 16);
    memcpy(e->config.ssid, w->ssid, 8);
    e->config.symbol_table = w->symbol_table;
    e->config.symbol_code = w->symbol_code;
    e->config.tx_enabled = w->tx_enabled;
    e->config.digi_enabled = w->digi_enabled;
    e->config.beacon_enabled = w->beacon_enabled;
    e->config.unique_frames_only = w->unique_frames_only;
    e->config.tcpip_enabled = w->tcpip_enabled;
    e->config.invalid_frames_enabled = w->invalid_frames_enabled;
    e->config.gps_baudrate = w->gps_baudrate;
    e->config.use_position_mode = (aprs_position_mode_t)w->use_position_mode;
    e->config.report_position_mode = (aprs_position_mode_t)w->report_position_mode;

    /* Core */
    memcpy(e->udest, w->udest, 8);
    e->path_use = w->path_use;
    e->msg_path_use = w->msg_path_use;
    e->status_use = w->status_use;
    e->nav_mark_enabled = w->nav_mark_enabled;

    /* Positions */
    for (int i = 0; i < 3; i++) {
        e->pos_slots[i].lat = w->pos_slot_lat[i];
        e->pos_slots[i].lon = w->pos_slot_lon[i];
        e->pos_slots[i].alt = w->pos_slot_alt[i];
    }
    e->active_pos_slot = w->active_pos_slot;
    e->use_mode = (aprs_position_mode_t)w->use_mode;
    e->report_mode = (aprs_position_mode_t)w->report_mode;

    /* Beacon */
    memcpy(e->status_texts, w->status_texts, sizeof(e->status_texts));
    memcpy(e->paths, w->paths, sizeof(e->paths));
    e->beacon_delay_sec = w->beacon_delay_sec;
    e->beacon_auto_enabled = w->beacon_auto_enabled;
    e->beacon_digi_sensitivity = w->beacon_digi_sensitivity;

    /* Message */
    e->receive_all_msgs = w->receive_all_msgs;
    e->msg_retries = w->msg_retries;
    e->msg_retry_delay_sec = w->msg_retry_delay_sec;

    /* Digi */
    memcpy(e->aliases, w->aliases, sizeof(e->aliases));
    e->digi_dupe_time_sec = w->digi_dupe_time_sec;
    e->digi_dupe_delay_sec = w->digi_dupe_delay_sec;
    e->digi_tx_monitor = w->digi_tx_monitor;

    /* Filters */
    e->filter_unique_only = w->filter_unique_only;
    e->filter_tcpip = w->filter_tcpip;
    e->filter_invalid_ok = w->filter_invalid_ok;
    memcpy(e->filter_expr, w->filter_expr, 64);
    e->announce_filter = w->announce_filter;
    e->filter_include_mask = w->filter_include_mask;
    e->filter_exclude_mask = w->filter_exclude_mask;

    /* TRX */
    e->trx.tx_delay = w->trx_tx_delay;
    e->trx.tx_header = w->trx_tx_header;
    e->trx.bit_delay = w->trx_bit_delay;
    e->trx.rx_tune = w->trx_rx_tune;
    e->trx.squelch = w->trx_squelch;

    /* Sounds */
    for (int i = 0; i < 7; i++) {
        e->sounds[i].level = w->sound_levels[i];
    }

    /* Predefined messages */
    memcpy(e->predefined_msgs, w->predefined_msgs, sizeof(e->predefined_msgs));
}

/* Save configuration to binary file */
int cp_save(aprs_engine_t* e, const char* filepath) {
    if (!e || !filepath) return 0;

    wire_config_t w;
    cp_engine_to_wire(e, &w);

    /* Compute checksum */
    uint32_t checksum = cp_crc32((const uint8_t*)&w, sizeof(w));

    /* Write file */
    FILE* fp = fopen(filepath, "wb");
    if (!fp) return 0;

    uint32_t magic = CONFIG_MAGIC;
    uint32_t version = CONFIG_VERSION;
    uint32_t data_size = sizeof(w);

    fwrite(&magic, sizeof(magic), 1, fp);
    fwrite(&version, sizeof(version), 1, fp);
    fwrite(&data_size, sizeof(data_size), 1, fp);
    fwrite(&checksum, sizeof(checksum), 1, fp);
    fwrite(&w, sizeof(w), 1, fp);

    fclose(fp);
    return 1;
}

/* Load configuration from binary file */
int cp_load(aprs_engine_t* e, const char* filepath) {
    if (!e || !filepath) return 0;

    FILE* fp = fopen(filepath, "rb");
    if (!fp) return 0;

    uint32_t magic, version, data_size, checksum;
    if (fread(&magic, sizeof(magic), 1, fp) != 1) { fclose(fp); return 0; }
    if (fread(&version, sizeof(version), 1, fp) != 1) { fclose(fp); return 0; }
    if (fread(&data_size, sizeof(data_size), 1, fp) != 1) { fclose(fp); return 0; }
    if (fread(&checksum, sizeof(checksum), 1, fp) != 1) { fclose(fp); return 0; }

    /* Validate */
    if (magic != CONFIG_MAGIC) { fclose(fp); return 0; }
    if (version != CONFIG_VERSION) { fclose(fp); return 0; }
    if (data_size != sizeof(wire_config_t)) { fclose(fp); return 0; }

    wire_config_t w;
    if (fread(&w, sizeof(w), 1, fp) != 1) { fclose(fp); return 0; }
    fclose(fp);

    /* Verify checksum */
    uint32_t actual = cp_crc32((const uint8_t*)&w, sizeof(w));
    if (actual != checksum) return 0;

    /* Apply to engine */
    cp_wire_to_engine(e, &w);
    return 1;
}

/* Reset all configuration to factory defaults */
void cp_reset_factory(aprs_engine_t* e) {
    if (!e) return;

    /* Callsign defaults */
    strncpy(e->config.callsign, "NOCALL", sizeof(e->config.callsign) - 1);
    e->config.ssid[0] = '\0';
    e->config.symbol_table = '/';
    e->config.symbol_code = '>';
    e->config.tx_enabled = 1;
    e->config.digi_enabled = 1;
    e->config.beacon_enabled = 1;
    e->config.unique_frames_only = 1;
    e->config.tcpip_enabled = 0;
    e->config.invalid_frames_enabled = 0;
    e->config.gps_baudrate = 4800;
    e->config.use_position_mode = APRS_POSITION_AUTO;
    e->config.report_position_mode = APRS_POSITION_AUTO;

    /* Core */
    strncpy(e->udest, "APRS", sizeof(e->udest) - 1);
    e->path_use = 1;  /* path 0 enabled */
    e->msg_path_use = 1;
    e->status_use = 1;  /* status 0 enabled */
    e->nav_mark_enabled = 0;

    /* Position — default to somewhere in Poland (approx Poznan) */
    for (int i = 0; i < 3; i++) {
        e->pos_slots[i].lat = 52.4064;
        e->pos_slots[i].lon = 16.9252;
        e->pos_slots[i].alt = 92.0;
    }
    e->active_pos_slot = 0;
    e->use_mode = APRS_POSITION_AUTO;
    e->report_mode = APRS_POSITION_AUTO;
    e->my_lat = 52.4064;
    e->my_lon = 16.9252;
    e->my_alt = 92.0;

    /* Beacon */
    memset(e->status_texts, 0, sizeof(e->status_texts));
    strncpy(e->status_texts[0].text, "APRS Engine v2", sizeof(e->status_texts[0].text) - 1);
    e->status_texts[0].enabled = 1;

    memset(e->paths, 0, sizeof(e->paths));
    strncpy(e->paths[0].path, "WIDE1-1,WIDE2-1", sizeof(e->paths[0].path) - 1);
    e->paths[0].enabled = 1;
    e->paths[0].msg_enabled = 0;

    e->beacon_delay_sec = 600;  /* 10 minutes */
    e->beacon_auto_enabled = 1;
    e->next_beacon_time_ms = e->current_time_ms + 600000;

    /* Messages */
    e->receive_all_msgs = 0;
    e->msg_retries = 4;
    e->msg_retry_delay_sec = 35;

    /* Digi */
    memset(e->aliases, 0, sizeof(e->aliases));
    strncpy(e->aliases[0].alias, "WIDE1", sizeof(e->aliases[0].alias) - 1);
    e->aliases[0].type = APRS_DIGI_FLOOD;
    e->aliases[0].max_hops = 1;
    e->aliases[0].enabled = 1;
    e->digi_dupe_time_sec = 30;
    e->digi_dupe_delay_sec = 2;
    e->digi_tx_monitor = 1;

    /* Filters */
    e->filter_unique_only = 1;
    e->filter_tcpip = 1;
    e->filter_invalid_ok = 0;
    e->filter_include_mask = 0xFF;
    e->filter_exclude_mask = 0x00;
    e->announce_filter = 0;

    /* TRX defaults */
    e->trx.tx_delay = 5;
    e->trx.tx_header = 5;
    e->trx.bit_delay = 5;
    e->trx.rx_tune = 32;
    e->trx.squelch = 8;

    /* Sounds */
    for (int i = 0; i < 7; i++) {
        e->sounds[i].level = 5;
    }

    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}
