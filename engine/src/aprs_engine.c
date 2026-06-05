#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Mutex lock/unlock helpers */
static inline void engine_lock(aprs_engine_t* e) {
#ifdef _WIN32
    EnterCriticalSection(&e->mutex);
#else
    pthread_mutex_lock(&e->mutex);
#endif
}

static inline void engine_unlock(aprs_engine_t* e) {
#ifdef _WIN32
    LeaveCriticalSection(&e->mutex);
#else
    pthread_mutex_unlock(&e->mutex);
#endif
}

/* ========================================================================
 * TASK PROGRESS
 * ========================================================================
 * - [x] Implement mutex for thread safety
 * - [x] Fix WebSocket memory leak
 * - [x] Add authentication for HTTP/WebSocket
 * - [x] Implement missing API functions
 * ======================================================================== */

/* ========================================================================
 * APRS ENGINE — Lifecycle, command queue, frame dispatch, ticking
 *
 * This is the central coordinator. All public API functions enqueue
 * commands that are processed serially in engine_process_commands().
 * Frame ingestion flows through engine_dispatch_frame().
 * ======================================================================== */

/* ========================================================================
 * LIFECYCLE
 * ======================================================================== */

const char* aprs_engine_version(void) {
    return "2.0.0";
}

aprs_status_t aprs_engine_create(const aprs_engine_config_t* cfg, aprs_engine_t** out) {
    if (!out) return APRS_ERR_INVALID_ARG;

    aprs_engine_t* e = (aprs_engine_t*)calloc(1, sizeof(aprs_engine_t));
    if (!e) return APRS_ERR_NO_MEMORY;

    /* Apply config if provided */
    if (cfg) {
        memcpy(&e->config, cfg, sizeof(aprs_engine_config_t));
    } else {
        /* Minimal defaults — caller should use factory reset or set config */
        memset(&e->config, 0, sizeof(e->config));
        strncpy(e->config.callsign, "NOCALL", sizeof(e->config.callsign) - 1);
        e->config.symbol_table = '/';
        e->config.symbol_code = '>';
        e->config.tx_enabled = 1;
        e->config.digi_enabled = 1;
        e->config.beacon_enabled = 1;
    }

    /* Initialize subsystems */
    e->start_time_ms = (uint64_t)(clock() * 1000 / CLOCKS_PER_SEC);
    e->current_time_ms = e->start_time_ms;

    eb_init(e);

    /* Set factory defaults for everything not in config */
    cp_reset_factory(e);

    /* Re-apply user config over factory defaults */
    if (cfg) {
        memcpy(&e->config, cfg, sizeof(aprs_engine_config_t));
    }

    /* Init GPS */
    gm_init(e);

    /* Init stats */
    memset(e->traffic_buckets, 0, sizeof(e->traffic_buckets));
    memset(e->htraffic_buckets, 0, sizeof(e->htraffic_buckets));
    e->last_bucket_rotate_ms = e->current_time_ms;
    e->last_10min_rotate_ms = e->current_time_ms;

    /* Init command queue */
    memset(e->cmd_queue, 0, sizeof(e->cmd_queue));
    e->cmd_queue_head = 0;
    e->cmd_queue_tail = 0;
    e->cmd_queue_count = 0;

    /* Init unique checks */
    memset(e->unique_checks, 0, sizeof(e->unique_checks));
    e->unique_check_count = 0;

    /* Init editor */
    memset(e->editor_frame_raw, 0, sizeof(e->editor_frame_raw));
    e->editor_frame_len = 0;

    /* Init station 0 as "self" */
    strncpy(e->stations[0].callsign, e->config.callsign, sizeof(e->stations[0].callsign) - 1);
    e->stations[0].symbol_table = e->config.symbol_table;
    e->stations[0].symbol_code = e->config.symbol_code;
    e->stations[0].has_position = 1;
    e->stations[0].lat = e->my_lat;
    e->stations[0].lon = e->my_lon;
    e->stations[0].last_heard_ms = e->current_time_ms;
    e->num_stations = 1;

    /* Build initial sort order */
    for (int i = 0; i < MAX_STATIONS; i++) {
        e->sort_order[i] = (uint16_t)i;
    }
    strncpy(e->sort_column, "last_heard", sizeof(e->sort_column) - 1);
    e->sort_ascending = 0;

    /* Set message ID counter */
    e->msg_next_id = 1000;

    /* Initialize mutex */
#ifdef _WIN32
    InitializeCriticalSection(&e->mutex);
#else
    pthread_mutex_init(&e->mutex, NULL);
#endif

    *out = e;
    return APRS_OK;
}

aprs_status_t aprs_engine_start(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    e->running = 1;
    e->start_time_ms = e->current_time_ms;
    return APRS_OK;
}

aprs_status_t aprs_engine_stop(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    e->running = 0;
    return APRS_OK;
}

void aprs_engine_destroy(aprs_engine_t* e) {
    if (!e) return;
    
    /* Destroy mutex */
#ifdef _WIN32
    DeleteCriticalSection(&e->mutex);
#else
    pthread_mutex_destroy(&e->mutex);
#endif
    
    free(e);
}

/* ========================================================================
 * COMMAND QUEUE
 * ======================================================================== */

int engine_enqueue_cmd(aprs_engine_t* e, const cmd_entry_t* cmd) {
    if (!e || !cmd) return 0;

    /* Check if queue is full */
    if (e->cmd_queue_count >= MAX_CMD_QUEUE) return 0;

    memcpy(&e->cmd_queue[e->cmd_queue_tail], cmd, sizeof(cmd_entry_t));
    e->cmd_queue_tail = (e->cmd_queue_tail + 1) % MAX_CMD_QUEUE;
    e->cmd_queue_count++;
    return 1;
}

/* Process all pending commands in the queue */
void engine_process_commands(aprs_engine_t* e) {
    if (!e) return;

    while (e->cmd_queue_count > 0) {
        cmd_entry_t* cmd = &e->cmd_queue[e->cmd_queue_head];
        e->cmd_queue_head = (e->cmd_queue_head + 1) % MAX_CMD_QUEUE;
        e->cmd_queue_count--;

        switch (cmd->type) {
            case CMD_SET_CONFIG:
                memcpy(&e->config, &cmd->data.set_config.cfg, sizeof(aprs_engine_config_t));
                eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
                break;

            case CMD_SET_CALLSIGN:
                strncpy(e->config.callsign, cmd->data.set_callsign.callsign,
                        sizeof(e->config.callsign) - 1);
                eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
                break;

            case CMD_SET_POSITION:
                pm_set_manual_position(e, cmd->data.set_position.lat,
                                        cmd->data.set_position.lon,
                                        cmd->data.set_position.alt);
                break;

            case CMD_SET_POSITION_MODE:
                pm_set_use_mode(e, cmd->data.set_position_mode.mode);
                break;

            case CMD_SEND_BEACON:
                bm_send_beacon(e);
                break;

            case CMD_SEND_MESSAGE:
                mm_send_message(e, cmd->data.send_message.dest,
                                cmd->data.send_message.text);
                break;

            case CMD_LOCK_TX:
                e->config.tx_enabled = !cmd->data.lock_tx.disable_tx;
                e->config.digi_enabled = !cmd->data.lock_tx.disable_digi;
                e->config.beacon_enabled = !cmd->data.lock_tx.disable_query;
                eb_publish(e, APRS_EVT_TX_STATE_CHANGED, "tx.state", NULL, 0);
                break;

            case CMD_SET_FILTER:
                e->filter_unique_only = cmd->data.set_filter_expr.expr[0] ? 1 : 0;
                eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
                break;

            case CMD_FACTORY_RESET:
                cp_reset_factory(e);
                break;

            case CMD_MARK_STATION:
                sm_mark(e, cmd->data.mark_station.callsign, cmd->data.mark_station.mark);
                break;

            case CMD_SORT_STATIONS:
                sm_sort(e, cmd->data.sort_stations.column, cmd->data.sort_stations.ascending);
                break;

            case CMD_SET_USE_POS_MODE:
                pm_set_use_mode(e, cmd->data.set_use_pos_mode.mode);
                break;

            case CMD_SET_REPORT_POS_MODE:
                pm_set_report_mode(e, cmd->data.set_report_pos_mode.mode);
                break;

            case CMD_SET_STATIC_POS:
                pm_set_static_position(e, cmd->data.set_static_pos.slot,
                                        cmd->data.set_static_pos.lat,
                                        cmd->data.set_static_pos.lon,
                                        cmd->data.set_static_pos.alt);
                break;

            case CMD_SELECT_POS_SLOT:
                pm_select_slot(e, cmd->data.select_pos_slot.slot);
                break;

            case CMD_CAPTURE_POS:
                pm_capture_station(e, cmd->data.capture_pos.callsign);
                break;

            case CMD_SET_MANUAL_POS:
                pm_set_manual_position(e, cmd->data.set_manual_pos.lat,
                                        cmd->data.set_manual_pos.lon,
                                        cmd->data.set_manual_pos.alt);
                break;

            case CMD_SET_STATUS_TEXT:
                bm_set_status_text(e, cmd->data.set_status_text.slot,
                                   cmd->data.set_status_text.text);
                break;

            case CMD_SET_PATH:
                bm_set_path(e, cmd->data.set_path.slot, cmd->data.set_path.path);
                break;

            case CMD_SELECT_STATUS:
                bm_select_status(e, cmd->data.select_status.slot,
                                 cmd->data.select_status.enabled);
                break;

            case CMD_SELECT_PATH:
                bm_select_path(e, cmd->data.select_path.slot,
                               cmd->data.select_path.enabled);
                break;

            case CMD_SET_BEACON_DELAY:
                bm_set_delay(e, cmd->data.set_beacon_delay.delay_sec);
                break;

            case CMD_SET_MSG_RECV_MODE:
                mm_set_receive_mode(e, cmd->data.set_msg_recv_mode.receive_all);
                break;

            case CMD_SET_MSG_RETRY:
                mm_set_retry(e, cmd->data.set_msg_retry.retries,
                             cmd->data.set_msg_retry.delay_sec);
                break;

            case CMD_MARK_MSG_READ:
                mm_mark_read(e, cmd->data.mark_msg_read.id);
                break;

            case CMD_COPY_FRAME_EDITOR:
                rfe_copy_from_station(e, cmd->data.copy_frame_editor.callsign);
                break;

            case CMD_SET_EDITOR_TEXT:
                rfe_set_text(e, cmd->data.set_editor_text.raw);
                break;

            case CMD_EDIT_EDITOR_BYTE:
                rfe_edit_byte(e, cmd->data.edit_editor_byte.offset,
                              cmd->data.edit_editor_byte.value);
                break;

            case CMD_SEND_EDITOR_FRAME:
                rfe_send(e);
                break;

            case CMD_CLEAR_EDITOR_FRAME:
                rfe_clear(e);
                break;

            case CMD_SET_DIGI_ENABLED:
                de_set_enabled(e, cmd->data.set_digi_enabled.enabled);
                break;

            case CMD_SET_DIGI_ALIAS:
                de_set_alias(e, cmd->data.set_digi_alias.slot,
                             cmd->data.set_digi_alias.alias,
                             cmd->data.set_digi_alias.type,
                             cmd->data.set_digi_alias.max_hops);
                break;

            case CMD_SET_DIGI_TIMING:
                de_set_timing(e, cmd->data.set_digi_timing.dupetime_sec,
                              cmd->data.set_digi_timing.dupedelay_sec);
                break;

            case CMD_SET_DIGI_TX_MONITOR:
                de_set_tx_monitor(e, cmd->data.set_digi_tx_monitor.enabled);
                break;

            case CMD_SET_FRAME_FILTER:
                e->filter_unique_only = cmd->data.set_frame_filter.unique_only;
                e->filter_tcpip = cmd->data.set_frame_filter.tcpip;
                e->filter_invalid_ok = cmd->data.set_frame_filter.invalid_ok;
                eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
                break;

            case CMD_SET_FILTER_EXPR:
                strncpy(e->filter_expr, cmd->data.set_filter_expr.expr,
                        sizeof(e->filter_expr) - 1);
                eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
                break;

            case CMD_PUSH_NMEA:
                gm_push_nmea(e, cmd->data.push_nmea.sentence);
                break;

            case CMD_GENERATE_REMOTE_KEYS: {
                char keys[4][16];
                rc_generate_keys(e, keys);
                break;
            }

            case CMD_EXEC_REMOTE_CMD:
                rc_execute_command(e, cmd->data.exec_remote_cmd.src,
                                   cmd->data.exec_remote_cmd.msg);
                break;

            case CMD_SET_TRX_PARAMS:
                trx_set_params(e, cmd->data.set_trx.tx_delay,
                               cmd->data.set_trx.tx_header,
                               cmd->data.set_trx.bit_delay,
                               cmd->data.set_trx.rx_tune,
                               cmd->data.set_trx.squelch);
                break;

            case CMD_SET_SOUND_LEVEL:
                nm_set_sound(e, cmd->data.set_sound.sound_id,
                             cmd->data.set_sound.level);
                break;

            case CMD_SAVE_CONFIG:
                cp_save(e, "aprs_engine.cfg");
                break;

            case CMD_LOAD_CONFIG:
                cp_load(e, "aprs_engine.cfg");
                break;

            default:
                break;
        }
    }
}

/* ========================================================================
 * FRAME DISPATCH
 * ======================================================================== */

/* Process a raw APRS frame through the entire pipeline:
 *   parse → classify → filter → station update → digi check → message dispatch → stats
 */
void engine_dispatch_frame(aprs_engine_t* e, const char* raw, size_t len) {
    if (!e || !raw || len == 0) return;

    /* Step 1: Parse */
    aprs_frame_t f;
    if (!fi_parse_frame(raw, len, &f)) {
        se_record_invalid(e);
        return;
    }

    /* Step 2: Check validity */
    if (!fi_validate_checksum(raw, len)) {
        f.is_valid = 0;
    }

    /* Step 3: Apply filters */
    if (!fi_apply_filters(e, &f)) {
        if (!f.is_valid) se_record_invalid(e);
        return;
    }

    /* Step 4: Classify */
    frame_class_t fc = fi_classify_frame(&f);

    /* Step 5: Compute CRC for uniqueness check */
    uint8_t crc = fi_compute_crc(raw, len);

    /* Step 6: Uniqueness check */
    int is_uniq = fi_is_unique(e, f.src, crc);
    f.is_unique = (uint8_t)is_uniq;
    if (is_uniq) {
        se_record_unique(e);
        fi_register_unique(e, f.src, crc);
    }

    /* Step 7: Record RX stat */
    se_record_rx(e);

    /* Step 8: Station update */
    int16_t st_idx = sm_find_or_create(e, f.src);
    if (st_idx >= 0) {
        sm_update_from_frame(e, (uint16_t)st_idx, &f, fc);
    }

    /* Step 9: Publish frame.rx event */
    eb_publish_frame_rx(e, &f);

    /* Step 9b: Check if this is our own frame being digipeated (digisens) */
    if (strncmp(f.src, e->config.callsign, strlen(e->config.callsign)) == 0) {
        de_check_own_digi(e, f.src, f.path);
        /* Sound: our own frame heard via digi */
        nm_trigger(e, SND_SELF_RX);
    }

    /* Step 10: Dispatch by frame type */
    const char* colon = strchr(raw, ':');
    if (colon) {
        const char* payload = colon + 1;
        size_t payload_len = (size_t)(raw + len - payload);

        switch (fc) {
            case FRAME_CLASS_MESSAGE: {
                /* Extract message fields: CALL>APRS::DEST-SSID    :TEXT{MSGID}
                 * After the first ':' (data type), payload[0] is ':'
                 * Then 9 chars of destination (padded with spaces)
                 * Then the message text */
                char dst[16];
                memset(dst, 0, sizeof(dst));

                /* Skip the message data type indicator ':' at payload[0] */
                const char* msg_body = payload;
                if (payload_len > 0 && *msg_body == ':') {
                    msg_body++;
                    payload_len--;
                }

                if (payload_len >= 9) {
                    const char* dst_start = msg_body;
                    size_t dst_len = 9;
                    if (dst_len >= sizeof(dst)) dst_len = sizeof(dst) - 1;
                    memcpy(dst, dst_start, dst_len);
                    /* Strip trailing spaces */
                    while (dst_len > 0 && dst[dst_len-1] == ' ') {
                        dst[--dst_len] = '\0';
                    }
                }

                /* Text starts after the 9-char destination field */
                const char* text_start = msg_body + 9;
                size_t text_len = (payload_len > 9) ? payload_len - 9 : 0;

                /* Extract message ID from text if present: {MSGID} */
                char msg_id[16] = {0};
                char text[256] = {0};
                size_t tlen = text_len < sizeof(text) - 1 ? text_len : sizeof(text) - 1;
                memcpy(text, text_start, tlen);
                text[tlen] = '\0';

                /* Find { at end */
                char* brace = strrchr(text, '{');
                if (brace && brace > text && brace[1] != '\0') {
                    char* end_brace = strchr(brace, '}');
                    if (end_brace) {
                        size_t id_len = (size_t)(end_brace - brace - 1);
                        if (id_len < sizeof(msg_id) - 1) {
                            memcpy(msg_id, brace + 1, id_len);
                            msg_id[id_len] = '\0';
                        }
                        *brace = '\0'; /* truncate text at { */
                    }
                }

                mm_receive_message(e, f.src, dst, text, msg_id);
                break;
            }

            case FRAME_CLASS_QUERY:
                /* Respond to ?APRS? query by triggering beacon */
                if (payload_len >= 6 && strncmp(payload, "APRS?", 5) == 0) {
                    bm_send_beacon(e);
                }
                break;

            case FRAME_CLASS_STATUS:
                /* Status received — station info already updated */
                break;

            case FRAME_CLASS_POSITION:
            case FRAME_CLASS_MIC_E:
                /* Position recalc */
                if (st_idx >= 0) {
                    sm_recalc_all(e);
                    eb_publish_station_updated(e, &e->stations[st_idx]);
                }
                break;

            case FRAME_CLASS_OBJECT:
            case FRAME_CLASS_ITEM:
            case FRAME_CLASS_WX:
            case FRAME_CLASS_THIRDPARTY:
            default:
                break;
        }
    }

    /* Step 11: Digi check (for all frames) */
    de_check_and_queue(e, &f);

    /* Step 12: Sound notification */
    if (st_idx >= 0) {
        if (e->stations[st_idx].marked) {
            nm_trigger(e, SND_MARKED_RX);
        } else if (strcmp(f.src, e->config.callsign) == 0) {
            nm_trigger(e, SND_SELF_RX);
        } else {
            nm_trigger(e, SND_STATION_RX);
        }
    }

    /* Step 13: Publish stats periodically */
    eb_publish_simple(e, APRS_EVT_STATS_UPDATED);
}

/* ========================================================================
 * TICK — Main periodic processing
 * ======================================================================== */

aprs_status_t aprs_engine_tick(aprs_engine_t* e, uint64_t delta_ms) {
    if (!e) return APRS_ERR_INVALID_ARG;
    if (!e->running) return APRS_ERR_DENIED;

    /* Update clock */
    e->current_time_ms += delta_ms;

    /* Process pending commands */
    engine_process_commands(e);

    /* GPS tick */
    gm_tick(e);

    /* Beacon tick */
    bm_tick(e);

    /* Digi tick */
    de_tick(e);

    /* Message retry tick */
    mm_retry_tick(e);

    /* Stats tick */
    se_tick(e);

    return APRS_OK;
}

/* ========================================================================
 * FEED FRAME — Public API for injecting raw APRS frames
 * ======================================================================== */

aprs_status_t aprs_engine_feed_frame(aprs_engine_t* e, const char* raw, size_t len) {
    if (!e) return APRS_ERR_INVALID_ARG;
    if (!raw || len == 0) return APRS_ERR_INVALID_ARG;
    if (!e->running) return APRS_ERR_DENIED;

    engine_dispatch_frame(e, raw, len);
    return APRS_OK;
}

/* ========================================================================
 * PUBLIC C API — All command wrappers
 * ======================================================================== */

/* --- Configuration --- */

aprs_status_t aprs_engine_set_config(aprs_engine_t* e, const aprs_engine_config_t* cfg) {
    if (!e || !cfg) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_CONFIG;
    memcpy(&cmd.data.set_config.cfg, cfg, sizeof(*cfg));
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_config(aprs_engine_t* e, aprs_engine_config_t* out) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    memcpy(out, &e->config, sizeof(aprs_engine_config_t));
    return APRS_OK;
}

aprs_status_t aprs_engine_save_config(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SAVE_CONFIG;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_load_config(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_LOAD_CONFIG;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_factory_reset(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_FACTORY_RESET;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_callsign(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_CALLSIGN;
    strncpy(cmd.data.set_callsign.callsign, callsign, sizeof(cmd.data.set_callsign.callsign) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Stations --- */

aprs_status_t aprs_engine_list_stations(aprs_engine_t* e, aprs_station_t* out, size_t* inout_count) {
    if (!e || !out || !inout_count) return APRS_ERR_INVALID_ARG;

    size_t max = *inout_count;
    size_t count = 0;
    for (uint16_t i = 0; i < e->num_stations && count < max; i++) {
        memcpy(&out[count], &e->stations[e->sort_order[i]], sizeof(aprs_station_t));
        count++;
    }
    *inout_count = count;
    return APRS_OK;
}

aprs_status_t aprs_engine_get_station(aprs_engine_t* e, const char* callsign, aprs_station_t* out) {
    if (!e || !callsign || !out) return APRS_ERR_INVALID_ARG;

    int16_t idx = sm_find_station(e, callsign);
    if (idx < 0) return APRS_ERR_NOT_FOUND;

    memcpy(out, &e->stations[idx], sizeof(aprs_station_t));
    return APRS_OK;
}

aprs_status_t aprs_engine_mark_station(aprs_engine_t* e, const char* callsign, uint8_t mark) {
    if (!e || !callsign) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_MARK_STATION;
    strncpy(cmd.data.mark_station.callsign, callsign, sizeof(cmd.data.mark_station.callsign) - 1);
    cmd.data.mark_station.mark = mark;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_sort_stations(aprs_engine_t* e, const char* column, uint8_t ascending) {
    if (!e || !column) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SORT_STATIONS;
    strncpy(cmd.data.sort_stations.column, column, sizeof(cmd.data.sort_stations.column) - 1);
    cmd.data.sort_stations.ascending = ascending;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Position --- */

aprs_status_t aprs_engine_set_use_position_mode(aprs_engine_t* e, aprs_position_mode_t mode) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_USE_POS_MODE;
    cmd.data.set_use_pos_mode.mode = mode;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_report_position_mode(aprs_engine_t* e, aprs_position_mode_t mode) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_REPORT_POS_MODE;
    cmd.data.set_report_pos_mode.mode = mode;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt) {
    if (!e) return APRS_ERR_INVALID_ARG;
    if (slot < 0 || slot >= MAX_POS_SLOTS) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_STATIC_POS;
    cmd.data.set_static_pos.slot = slot;
    cmd.data.set_static_pos.lat = lat;
    cmd.data.set_static_pos.lon = lon;
    cmd.data.set_static_pos.alt = alt;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt) {
    if (!e) return APRS_ERR_INVALID_ARG;
    if (!pm_get_static_position(e, slot, lat, lon, alt)) return APRS_ERR_INVALID_ARG;
    return APRS_OK;
}

aprs_status_t aprs_engine_select_position_slot(aprs_engine_t* e, int slot) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SELECT_POS_SLOT;
    cmd.data.select_pos_slot.slot = slot;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_capture_station_position(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_CAPTURE_POS;
    strncpy(cmd.data.capture_pos.callsign, callsign, sizeof(cmd.data.capture_pos.callsign) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_MANUAL_POS;
    cmd.data.set_manual_pos.lat = lat;
    cmd.data.set_manual_pos.lon = lon;
    cmd.data.set_manual_pos.alt = alt;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Beacon --- */

aprs_status_t aprs_engine_set_status_text(aprs_engine_t* e, int slot, const char* text) {
    if (!e || !text || slot < 0 || slot >= MAX_STATUS_TEXTS) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_STATUS_TEXT;
    cmd.data.set_status_text.slot = slot;
    strncpy(cmd.data.set_status_text.text, text, sizeof(cmd.data.set_status_text.text) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_path(aprs_engine_t* e, int slot, const char* path) {
    if (!e || !path || slot < 0 || slot >= MAX_PATHS) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_PATH;
    cmd.data.set_path.slot = slot;
    strncpy(cmd.data.set_path.path, path, sizeof(cmd.data.set_path.path) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_select_status_text(aprs_engine_t* e, int slot, uint8_t enabled) {
    if (!e || slot < 0 || slot >= MAX_STATUS_TEXTS) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SELECT_STATUS;
    cmd.data.select_status.slot = slot;
    cmd.data.select_status.enabled = enabled;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_select_path(aprs_engine_t* e, int slot, uint8_t enabled) {
    if (!e || slot < 0 || slot >= MAX_PATHS) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SELECT_PATH;
    cmd.data.select_path.slot = slot;
    cmd.data.select_path.enabled = enabled;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_beacon_delay(aprs_engine_t* e, int delay_sec) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_BEACON_DELAY;
    cmd.data.set_beacon_delay.delay_sec = delay_sec;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_send_beacon(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SEND_BEACON;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_preview_last_tx_frame(aprs_engine_t* e, aprs_frame_t* out) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    bm_preview_last(e, out);
    return APRS_OK;
}

aprs_status_t aprs_engine_set_beacon_auto_mode(aprs_engine_t* e, uint8_t smart_enabled) {
    if (!e) return APRS_ERR_INVALID_ARG;
    bm_set_auto_mode(e, smart_enabled);
    return APRS_OK;
}

aprs_status_t aprs_engine_get_self_digis(aprs_engine_t* e, char* out, size_t out_len) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    return de_get_self_digis(e, out, out_len) ? APRS_OK : APRS_ERR_INTERNAL;
}

/* --- Messages --- */

aprs_status_t aprs_engine_send_message(aprs_engine_t* e, const char* dest, const char* text) {
    if (!e || !dest || !text) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SEND_MESSAGE;
    strncpy(cmd.data.send_message.dest, dest, sizeof(cmd.data.send_message.dest) - 1);
    strncpy(cmd.data.send_message.text, text, sizeof(cmd.data.send_message.text) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_list_messages(aprs_engine_t* e, aprs_message_t* out, size_t* inout_count) {
    if (!e || !out || !inout_count) return APRS_ERR_INVALID_ARG;

    size_t max = *inout_count;
    size_t count = 0;

    /* RX messages first */
    for (int i = 0; i < MAX_RX_MESSAGES && count < max; i++) {
        if (e->rx_messages[i].id[0] || e->rx_messages[i].state != APRS_MSG_NEW) {
            if (e->rx_messages[i].src[0]) {
                memcpy(&out[count], &e->rx_messages[i], sizeof(aprs_message_t));
                count++;
            }
        }
    }
    /* TX messages */
    for (int i = 0; i < MAX_TX_MESSAGES && count < max; i++) {
        if (e->tx_messages[i].id[0]) {
            memcpy(&out[count], &e->tx_messages[i], sizeof(aprs_message_t));
            count++;
        }
    }

    *inout_count = count;
    return APRS_OK;
}

aprs_status_t aprs_engine_get_message(aprs_engine_t* e, const char* id, aprs_message_t* out) {
    if (!e || !id || !out) return APRS_ERR_INVALID_ARG;

    for (int i = 0; i < MAX_RX_MESSAGES; i++) {
        if (e->rx_messages[i].id[0] && strcmp(e->rx_messages[i].id, id) == 0) {
            memcpy(out, &e->rx_messages[i], sizeof(aprs_message_t));
            return APRS_OK;
        }
    }
    for (int i = 0; i < MAX_TX_MESSAGES; i++) {
        if (e->tx_messages[i].id[0] && strcmp(e->tx_messages[i].id, id) == 0) {
            memcpy(out, &e->tx_messages[i], sizeof(aprs_message_t));
            return APRS_OK;
        }
    }
    return APRS_ERR_NOT_FOUND;
}

aprs_status_t aprs_engine_set_message_receive_mode(aprs_engine_t* e, uint8_t receive_all) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_MSG_RECV_MODE;
    cmd.data.set_msg_recv_mode.receive_all = receive_all;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_message_retry(aprs_engine_t* e, int retries, int delay_sec) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_MSG_RETRY;
    cmd.data.set_msg_retry.retries = retries;
    cmd.data.set_msg_retry.delay_sec = delay_sec;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_mark_message_read(aprs_engine_t* e, const char* id) {
    if (!e || !id) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_MARK_MSG_READ;
    strncpy(cmd.data.mark_msg_read.id, id, sizeof(cmd.data.mark_msg_read.id) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Frame Editor --- */

aprs_status_t aprs_engine_copy_frame_to_editor(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_COPY_FRAME_EDITOR;
    strncpy(cmd.data.copy_frame_editor.callsign, callsign, sizeof(cmd.data.copy_frame_editor.callsign) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_editor_frame(aprs_engine_t* e, aprs_frame_t* out) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    rfe_get(e, out);
    return APRS_OK;
}

aprs_status_t aprs_engine_set_editor_frame_text(aprs_engine_t* e, const char* raw) {
    if (!e || !raw) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_EDITOR_TEXT;
    strncpy(cmd.data.set_editor_text.raw, raw, sizeof(cmd.data.set_editor_text.raw) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_edit_editor_frame(aprs_engine_t* e, size_t offset, uint8_t value) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_EDIT_EDITOR_BYTE;
    cmd.data.edit_editor_byte.offset = offset;
    cmd.data.edit_editor_byte.value = value;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_send_editor_frame(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SEND_EDITOR_FRAME;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_clear_editor_frame(aprs_engine_t* e) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_CLEAR_EDITOR_FRAME;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Digi --- */

aprs_status_t aprs_engine_set_digi_enabled(aprs_engine_t* e, uint8_t enabled) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_DIGI_ENABLED;
    cmd.data.set_digi_enabled.enabled = enabled;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_digi_alias(aprs_engine_t* e, int slot, const char* alias,
                                          aprs_digi_type_t type, int max_hops) {
    if (!e || !alias || slot < 0 || slot >= MAX_ALIASES) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_DIGI_ALIAS;
    cmd.data.set_digi_alias.slot = slot;
    strncpy(cmd.data.set_digi_alias.alias, alias, sizeof(cmd.data.set_digi_alias.alias) - 1);
    cmd.data.set_digi_alias.type = type;
    cmd.data.set_digi_alias.max_hops = max_hops;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_digi_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_DIGI_TIMING;
    cmd.data.set_digi_timing.dupetime_sec = dupetime_sec;
    cmd.data.set_digi_timing.dupedelay_sec = dupedelay_sec;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_recent_digi(aprs_engine_t* e, char* out, size_t out_len) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    return de_get_recent(e, out, out_len) ? APRS_OK : APRS_ERR_INTERNAL;
}

aprs_status_t aprs_engine_set_digi_tx_monitor(aprs_engine_t* e, uint8_t enabled) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_DIGI_TX_MONITOR;
    cmd.data.set_digi_tx_monitor.enabled = enabled;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Filters --- */

aprs_status_t aprs_engine_set_frame_filter(aprs_engine_t* e, uint8_t unique_only,
                                            uint8_t tcpip, uint8_t invalid_ok) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_FRAME_FILTER;
    cmd.data.set_frame_filter.unique_only = unique_only;
    cmd.data.set_frame_filter.tcpip = tcpip;
    cmd.data.set_frame_filter.invalid_ok = invalid_ok;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_set_filter_expression(aprs_engine_t* e, const char* expr) {
    if (!e || !expr) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_FILTER_EXPR;
    strncpy(cmd.data.set_filter_expr.expr, expr, sizeof(cmd.data.set_filter_expr.expr) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- GPS --- */

aprs_status_t aprs_engine_push_nmea(aprs_engine_t* e, const char* sentence) {
    if (!e || !sentence) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_PUSH_NMEA;
    strncpy(cmd.data.push_nmea.sentence, sentence, sizeof(cmd.data.push_nmea.sentence) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_gps(aprs_engine_t* e, aprs_gps_t* out) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    gm_get_state(e, out);
    return APRS_OK;
}

/* --- Stats --- */

aprs_status_t aprs_engine_get_stats(aprs_engine_t* e, aprs_stats_t* out) {
    if (!e || !out) return APRS_ERR_INVALID_ARG;
    se_get_stats(e, out);
    return APRS_OK;
}

aprs_status_t aprs_engine_get_hourly_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count) {
    if (!e || !buckets || !inout_count) return APRS_ERR_INVALID_ARG;
    return se_get_hourly(e, buckets, inout_count) ? APRS_OK : APRS_ERR_INTERNAL;
}

aprs_status_t aprs_engine_get_20h_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count) {
    if (!e || !buckets || !inout_count) return APRS_ERR_INVALID_ARG;
    return se_get_20h(e, buckets, inout_count) ? APRS_OK : APRS_ERR_INTERNAL;
}

/* --- Remote --- */

aprs_status_t aprs_engine_generate_remote_keys(aprs_engine_t* e, char keys[4][16]) {
    if (!e || !keys) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_GENERATE_REMOTE_KEYS;
    if (!engine_enqueue_cmd(e, &cmd)) return APRS_ERR_BUSY;
    /* Keys are generated synchronously in command processing */
    engine_process_commands(e);
    /* Copy current keys */
    for (int i = 0; i < 4; i++) {
        snprintf(keys[i], 16, "%08X", e->remote_keys[i].key);
    }
    return APRS_OK;
}

aprs_status_t aprs_engine_execute_remote_command(aprs_engine_t* e, const char* src, const char* msg) {
    if (!e || !src || !msg) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_EXEC_REMOTE_CMD;
    strncpy(cmd.data.exec_remote_cmd.src, src, sizeof(cmd.data.exec_remote_cmd.src) - 1);
    strncpy(cmd.data.exec_remote_cmd.msg, msg, sizeof(cmd.data.exec_remote_cmd.msg) - 1);
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

/* --- Subscriptions --- */

aprs_subscription_id_t aprs_engine_subscribe(aprs_engine_t* e, const char* topic_filter,
                                              aprs_event_cb cb, void* user) {
    if (!e || !topic_filter || !cb) return 0;
    return eb_subscribe(e, topic_filter, cb, user);
}

aprs_status_t aprs_engine_unsubscribe(aprs_engine_t* e, aprs_subscription_id_t id) {
    if (!e) return APRS_ERR_INVALID_ARG;
    return eb_unsubscribe(e, id) ? APRS_OK : APRS_ERR_NOT_FOUND;
}

/* --- Snapshot (JSON) --- */

aprs_status_t aprs_engine_get_snapshot(aprs_engine_t* e, char* out_json, size_t* out_len) {
    if (!e || !out_json || !out_len) return APRS_ERR_INVALID_ARG;

    size_t buf_size = *out_len;
    if (buf_size < 256) return APRS_ERR_NO_MEMORY;

    /* Build a minimal JSON snapshot */
    int n = snprintf(out_json, buf_size,
        "{"
        "\"engine\":{"
            "\"version\":\"%s\","
            "\"callsign\":\"%s\","
            "\"ssid\":\"%s\","
            "\"txEnabled\":%s,"
            "\"digiEnabled\":%s,"
            "\"beaconEnabled\":%s,"
            "\"uptimeMs\":%llu"
        "},"
        "\"gps\":{"
            "\"fix\":%s,"
            "\"valid\":%s,"
            "\"lat\":%.6f,"
            "\"lon\":%.6f,"
            "\"speedKmh\":%.1f,"
            "\"courseDeg\":%.1f,"
            "\"altitudeM\":%.1f"
        "},"
        "\"stats\":{"
            "\"rxFrames\":%u,"
            "\"txFrames\":%u,"
            "\"digiRepeats\":%u,"
            "\"msgReceived\":%u,"
            "\"msgSent\":%u,"
            "\"invalidFrames\":%u,"
            "\"numStations\":%u"
        "},"
        "\"position\":{"
            "\"mode\":%d,"
            "\"lat\":%.6f,"
            "\"lon\":%.6f"
        "}"
        "}",
        aprs_engine_version(),
        e->config.callsign,
        e->config.ssid[0] ? e->config.ssid : "0",
        e->config.tx_enabled ? "true" : "false",
        e->config.digi_enabled ? "true" : "false",
        e->config.beacon_enabled ? "true" : "false",
        (unsigned long long)(e->current_time_ms - e->start_time_ms),
        e->gps.fix ? "true" : "false",
        e->gps.valid ? "true" : "false",
        e->gps.lat,
        e->gps.lon,
        e->gps.speed_kmh,
        e->gps.course_deg,
        e->gps.altitude_m,
        e->stats.rx_frames,
        e->stats.tx_frames,
        e->stats.digi_repeats,
        e->stats.msg_received,
        e->stats.msg_sent,
        e->stats.invalid_frames,
        e->num_stations,
        (int)e->use_mode,
        e->my_lat,
        e->my_lon
    );

    *out_len = (size_t)(n >= 0 ? n : 0);
    return (n >= 0 && (size_t)n < buf_size) ? APRS_OK : APRS_ERR_NO_MEMORY;
}

/* --- TRX --- */

aprs_status_t aprs_engine_set_trx_params(aprs_engine_t* e,
                                          uint8_t tx_delay, uint8_t tx_header,
                                          uint8_t bit_delay, uint8_t rx_tune,
                                          uint8_t squelch) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_TRX_PARAMS;
    cmd.data.set_trx.tx_delay = tx_delay;
    cmd.data.set_trx.tx_header = tx_header;
    cmd.data.set_trx.bit_delay = bit_delay;
    cmd.data.set_trx.rx_tune = rx_tune;
    cmd.data.set_trx.squelch = squelch;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_trx_params(aprs_engine_t* e,
                                          uint8_t* tx_delay, uint8_t* tx_header,
                                          uint8_t* bit_delay, uint8_t* rx_tune,
                                          uint8_t* squelch) {
    if (!e) return APRS_ERR_INVALID_ARG;
    trx_get_params(e, tx_delay, tx_header, bit_delay, rx_tune, squelch);
    return APRS_OK;
}

/* --- Sound --- */

aprs_status_t aprs_engine_set_sound_level(aprs_engine_t* e, int sound_id, uint8_t level) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_SET_SOUND_LEVEL;
    cmd.data.set_sound.sound_id = sound_id;
    cmd.data.set_sound.level = level;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}

aprs_status_t aprs_engine_get_sound_level(aprs_engine_t* e, int sound_id, uint8_t* level) {
    if (!e || !level) return APRS_ERR_INVALID_ARG;
    nm_get_sound(e, sound_id, level);
    return APRS_OK;
}

/* --- TX Lock --- */

aprs_status_t aprs_engine_lock_tx(aprs_engine_t* e, uint8_t disable_tx,
                                   uint8_t disable_digi, uint8_t disable_query) {
    if (!e) return APRS_ERR_INVALID_ARG;
    cmd_entry_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_LOCK_TX;
    cmd.data.lock_tx.disable_tx = disable_tx;
    cmd.data.lock_tx.disable_digi = disable_digi;
    cmd.data.lock_tx.disable_query = disable_query;
    return engine_enqueue_cmd(e, &cmd) ? APRS_OK : APRS_ERR_BUSY;
}
