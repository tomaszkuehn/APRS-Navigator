#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================================================
 * DIGI ENGINE — Digipeater alias matching, simple/flood/trace, duplicate
 *               prevention, timed repeat queue, recent digi list
 *
 * Extracted from:
 *   - sta_manager.c (digi_path, do_repeat, analyze_path, check_digis, digimeter)
 *   - config.c (config_aliases, config_digi_delay)
 * ======================================================================== */

void de_set_enabled(aprs_engine_t* e, uint8_t enabled) {
    if (!e) return;
    e->config.digi_enabled = enabled;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void de_set_alias(aprs_engine_t* e, int slot, const char* alias,
                  aprs_digi_type_t type, int max_hops) {
    if (!e || !alias) return;
    if (slot < 0 || slot >= MAX_ALIASES) return;

    strncpy(e->aliases[slot].alias, alias, sizeof(e->aliases[slot].alias) - 1);
    e->aliases[slot].alias[sizeof(e->aliases[slot].alias) - 1] = '\0';
    e->aliases[slot].type = type;
    e->aliases[slot].max_hops = (uint8_t)max_hops;
    e->aliases[slot].enabled = 1;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void de_set_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec) {
    if (!e) return;
    e->digi_dupe_time_sec = (uint32_t)dupetime_sec;
    e->digi_dupe_delay_sec = (uint32_t)dupedelay_sec;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void de_set_tx_monitor(aprs_engine_t* e, uint8_t enabled) {
    if (!e) return;
    e->digi_tx_monitor = enabled;
}

int de_get_recent(aprs_engine_t* e, char* out, size_t out_len) {
    if (!e || !out || out_len == 0) return 0;

    size_t pos = 0;
    for (int i = 0; i < MAX_RECENT_DIGI && e->recent_digi[i][0]; i++) {
        int n = snprintf(out + pos, out_len - pos, "%s%s",
                         (i > 0 ? "," : ""), e->recent_digi[i]);
        if (n < 0 || (size_t)n >= out_len - pos) break;
        pos += (size_t)n;
    }
    return 1;
}

/* Get digisens list — stations that digipeated MY beacon */
int de_get_self_digis(aprs_engine_t* e, char* out, size_t out_len) {
    if (!e || !out || out_len == 0) return 0;
    size_t pos = 0;
    for (int i = 0; i < MAX_RECENT_DIGI && e->self_digis[i][0]; i++) {
        int n = snprintf(out + pos, out_len - pos, "%s%s",
                         (i > 0 ? "," : ""), e->self_digis[i]);
        if (n < 0 || (size_t)n >= out_len - pos) break;
        pos += (size_t)n;
    }
    return 1;
}

/* Check if a received frame is a digipeated version of our own beacon.
 * Called during frame dispatch when the source matches our callsign. */
void de_check_own_digi(aprs_engine_t* e, const char* src, const char* path) {
    if (!e || !src || !path) return;

    /* Does this frame's source match our callsign? */
    if (strncmp(src, e->config.callsign, strlen(e->config.callsign)) != 0)
        return;

    /* Extract the last digipeater from the path (the one that just repeated us) */
    /* Path format: WIDE1-1*,WIDE2-1  or  CALLSIGN*,WIDE1-1
     * The digipeater that just handled it has its callsign marked with * */
    char path_copy[128];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    /* Find first callsign in path that has been "used" (marked with *) */
    char* token = strtok(path_copy, ",");
    while (token) {
        while (*token == ' ') token++;
        /* Check for used flag — in text form, a used digipeater has * appended */
        char* star = strchr(token, '*');
        if (star) {
            *star = '\0'; /* strip the * */
            /* Add to self_digis list */
            /* Shift down */
            for (int i = MAX_RECENT_DIGI - 1; i > 0; i--) {
                strncpy(e->self_digis[i], e->self_digis[i - 1], 9);
                e->self_digis[i][9] = '\0';
            }
            strncpy(e->self_digis[0], token, 9);
            e->self_digis[0][9] = '\0';
            if (e->self_digis_count < MAX_RECENT_DIGI) e->self_digis_count++;

            /* Cancel pending digisens retry — someone repeated us */
            e->digisens_retry_pending = 0;

            /* Publish digisens event */
            eb_publish(e, APRS_EVT_DIGI_RECENT, "digi.selfdigi", token, strlen(token) + 1);
            break;
        }
        token = strtok(NULL, ",");
    }
}

/* Add a callsign to the recent digipeated list */
static void de_add_recent(aprs_engine_t* e, const char* callsign) {
    if (!e || !callsign) return;

    /* Shift list down */
    for (int i = MAX_RECENT_DIGI - 1; i > 0; i--) {
        strncpy(e->recent_digi[i], e->recent_digi[i - 1], 9);
        e->recent_digi[i][9] = '\0';
    }
    strncpy(e->recent_digi[0], callsign, 9);
    e->recent_digi[0][9] = '\0';
    if (e->recent_digi_count < MAX_RECENT_DIGI) e->recent_digi_count++;
}

/* Analyze a frame's path to find matching digi aliases.
 * Returns the position in the path where the first matching alias is found,
 * or -1 if no match.
 */
static int de_analyze_path_for_alias(aprs_engine_t* e, const char* path) {
    if (!e || !path || !path[0]) return -1;

    /* Split path into comma-separated fields */
    char path_copy[128];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* token = strtok(path_copy, ",");
    int pos = 0;
    while (token) {
        /* Trim leading spaces */
        while (*token == ' ') token++;

        /* Check each alias */
        for (int a = 0; a < MAX_ALIASES; a++) {
            if (!e->aliases[a].enabled) continue;

            /* Alias matching: check if token starts with our alias */
            /* Example: alias "WIDE" matches "WIDE1-1", "WIDE2-2" */
            size_t alias_len = strlen(e->aliases[a].alias);
            if (strncmp(token, e->aliases[a].alias, alias_len) == 0) {
                /* Check if already digipeated (often marked with * after substitution) */
                /* In AX.25, a used digipeater has bit 7 of SSID set,
                 * which in text form appears as the callsign having been used */
                /* We check if there is a hop count still available */
                if (e->aliases[a].type == APRS_DIGI_FLOOD ||
                    e->aliases[a].type == APRS_DIGI_TRACE) {
                    /* For WIDEn-N style: parse the hop count */
                    const char* hyphen = strchr(token + alias_len, '-');
                    if (hyphen) {
                        int hop = atoi(hyphen + 1);
                        if (hop > 0 && hop <= e->aliases[a].max_hops) {
                            return pos;
                        }
                    }
                } else {
                    /* SIMPLE: match exact alias, no hop check */
                    if (strlen(token) == alias_len) {
                        return pos;
                    }
                    /* Also match if it's the alias followed by digits */
                    if (isdigit((unsigned char)token[alias_len])) {
                        return pos;
                    }
                }
            }
        }

        token = strtok(NULL, ",");
        pos++;
    }

    return -1;
}

/* Build the modified path for a digipeated frame based on alias type.
 * SIMPLE: replace alias with own callsign
 * FLOOD:  decrement hop count (WIDEn-N -> WIDEn-(N-1))
 * TRACE:  insert own callsign before the matched alias
 */
static int de_modify_path(char* path, size_t path_size, int alias_pos,
                           const char* alias, aprs_digi_type_t type,
                           const char* own_callsign) {
    if (!path || !alias || !own_callsign) return 0;

    /* Split and rebuild path */
    char parts[8][16];
    int part_count = 0;
    char path_copy[128];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* token = strtok(path_copy, ",");
    while (token && part_count < 8) {
        while (*token == ' ') token++;
        strncpy(parts[part_count], token, 15);
        parts[part_count][15] = '\0';
        part_count++;
        token = strtok(NULL, ",");
    }

    if (alias_pos >= part_count) return 0;

    switch (type) {
        case APRS_DIGI_SIMPLE: {
            /* Replace alias with own callsign */
            strncpy(parts[alias_pos], own_callsign, 15);
            parts[alias_pos][15] = '\0';
            break;
        }
        case APRS_DIGI_FLOOD: {
            /* Decrement hop count: WIDEn-N -> WIDEn-(N-1), then mark used */
            char* target = parts[alias_pos];
            /* Find the hyphen and decrement number after it */
            char* hyphen = strchr(target, '-');
            if (hyphen) {
                int hop = atoi(hyphen + 1);
                if (hop > 1) {
                    snprintf(hyphen + 1, (size_t)(15 - (hyphen + 1 - target)), "%d", hop - 1);
                    /* Don't replace with *, keep the decremented value */
                } else {
                    /* Hop count becomes 0 — replace with own callsign (consumed) */
                    strncpy(target, own_callsign, 15);
                    target[15] = '\0';
                }
            }
            break;
        }
        case APRS_DIGI_TRACE: {
            /* Insert own callsign before matched alias (shift right) */
            for (int i = part_count; i > alias_pos; i--) {
                if (i < 8) strncpy(parts[i], parts[i - 1], 15);
            }
            strncpy(parts[alias_pos], own_callsign, 15);
            parts[alias_pos][15] = '\0';
            part_count++;
            if (part_count > 8) part_count = 8;
            break;
        }
    }

    /* Rebuild path */
    path[0] = '\0';
    for (int i = 0; i < part_count; i++) {
        if (i > 0) strncat(path, ",", path_size - strlen(path) - 1);
        strncat(path, parts[i], path_size - strlen(path) - 1);
    }

    return 1;
}

/* Check if a frame is a duplicate in the digi queue */
static int de_is_duplicate(aprs_engine_t* e, const char* src, uint8_t crc) {
    if (!e) return 0;

    uint64_t now = e->current_time_ms;
    for (int i = 0; i < MAX_DIGI_QUEUE; i++) {
        if (e->digi_queue[i].status != 0) { /* free */
            if (e->digi_queue[i].crc == crc &&
                e->digi_queue[i].station_id == crc) {
                uint64_t elapsed = now - e->digi_queue[i].repeat_time_ms;
                /* If recently queued, it's a duplicate */
                if (elapsed < (uint64_t)e->digi_dupe_time_sec * 1000) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* ========================================================================
 * PUBLIC API
 * ======================================================================== */

void de_check_and_queue(aprs_engine_t* e, const aprs_frame_t* f) {
    if (!e || !f) return;
    if (!e->config.digi_enabled) return;

    /* Analyze path for matching aliases */
    int alias_pos = de_analyze_path_for_alias(e, f->path);
    if (alias_pos < 0) return;

    /* Find which alias matched */
    digi_alias_t* matched_alias = NULL;
    char path_copy[128];
    strncpy(path_copy, f->path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* token = strtok(path_copy, ",");
    for (int p = 0; p <= alias_pos && token; p++) {
        if (p == alias_pos) {
            for (int a = 0; a < MAX_ALIASES; a++) {
                if (!e->aliases[a].enabled) continue;
                size_t alen = strlen(e->aliases[a].alias);
                if (strncmp(token, e->aliases[a].alias, alen) == 0) {
                    matched_alias = &e->aliases[a];
                    break;
                }
            }
        }
        token = strtok(NULL, ",");
    }

    if (!matched_alias) return;

    /* Compute CRC for duplicate detection */
    uint8_t crc = fi_compute_crc(f->raw, f->raw_len);

    /* Check for duplicate */
    if (de_is_duplicate(e, f->src, crc)) {
        return; /* duplicate suppressed */
    }

    /* Find free queue slot */
    int slot = -1;
    for (int i = 0; i < MAX_DIGI_QUEUE; i++) {
        if (e->digi_queue[i].status == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        /* Queue full — find oldest waiting entry */
        uint64_t oldest = UINT64_MAX;
        for (int i = 0; i < MAX_DIGI_QUEUE; i++) {
            if (e->digi_queue[i].status == 1 &&
                e->digi_queue[i].repeat_time_ms < oldest) {
                oldest = e->digi_queue[i].repeat_time_ms;
                slot = i;
            }
        }
        if (slot < 0) return; /* all slots actively repeating */
    }

    /* Queue the frame */
    memset(&e->digi_queue[slot], 0, sizeof(digi_queue_entry_t));
    memcpy(e->digi_queue[slot].frame_data, f->raw, f->raw_len);
    e->digi_queue[slot].frame_data[f->raw_len] = '\0';
    e->digi_queue[slot].status = 1; /* waiting */
    e->digi_queue[slot].repeat_time_ms = e->current_time_ms +
        (uint64_t)e->digi_dupe_delay_sec * 1000;
    strncpy(e->digi_queue[slot].alias, matched_alias->alias, 7);
    e->digi_queue[slot].alias[7] = '\0';
    e->digi_queue[slot].alias_pos = (uint8_t)alias_pos;
    e->digi_queue[slot].crc = crc;
    e->digi_queue_count++;

    /* Publish digi recent event */
    eb_publish(e, APRS_EVT_DIGI_RECENT, "digi.recent", f, sizeof(aprs_frame_t));
}

void de_tick(aprs_engine_t* e) {
    if (!e) return;
    if (!e->config.digi_enabled) return;

    for (int i = 0; i < MAX_DIGI_QUEUE; i++) {
        if (e->digi_queue[i].status != 1) continue; /* not waiting */

        if (e->current_time_ms >= e->digi_queue[i].repeat_time_ms) {
            /* Time to repeat! */
            e->digi_queue[i].status = 2; /* repeated */

            /* Build the digipeated frame */
            char digi_frame[512];
            strncpy(digi_frame, e->digi_queue[i].frame_data, sizeof(digi_frame) - 1);
            digi_frame[sizeof(digi_frame) - 1] = '\0';

            /* Parse and modify the path */
            aprs_frame_t parsed;
            if (fi_parse_frame(digi_frame, strlen(digi_frame), &parsed)) {
                int alias_pos_int = e->digi_queue[i].alias_pos;

                /* Find alias type */
                aprs_digi_type_t atype = APRS_DIGI_SIMPLE;
                for (int a = 0; a < MAX_ALIASES; a++) {
                    if (strcmp(e->aliases[a].alias, e->digi_queue[i].alias) == 0) {
                        atype = e->aliases[a].type;
                        break;
                    }
                }

                char own_callsign[24];
                snprintf(own_callsign, sizeof(own_callsign), "%s-%s",
                         e->config.callsign, e->config.ssid[0] ? e->config.ssid : "0");

                char modified_path[128];
                strncpy(modified_path, parsed.path, sizeof(modified_path) - 1);
                modified_path[sizeof(modified_path) - 1] = '\0';

                de_modify_path(modified_path, sizeof(modified_path),
                              alias_pos_int, e->digi_queue[i].alias, atype,
                              own_callsign);
            }

            /* Record stats */
            se_record_tx(e);
            se_record_digi(e);

            /* Add to recent list */
            de_add_recent(e, e->config.callsign);

            /* Publish event */
            eb_publish(e, APRS_EVT_DIGI_REPEATED, "digi.repeated",
                       e->digi_queue[i].frame_data, strlen(e->digi_queue[i].frame_data));

            /* Sound */
            nm_trigger(e, SND_DIGI_TX);

            /* Mark slot as free after a cooldown */
            e->digi_queue[i].status = 0;
            if (e->digi_queue_count > 0) e->digi_queue_count--;
        }
    }
}
