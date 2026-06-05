#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * MESSAGE MANAGER — Inbox/outbox, ACK/REJ, retry logic
 *
 * Extracted from:
 *   - sta_manager.c (add_message, transmit_message)
 *   - config.c (config_messages)
 * ======================================================================== */

/* Generate a unique message ID */
static void mm_gen_msg_id(aprs_engine_t* e, char* out, size_t out_size) {
    e->msg_next_id++;
    snprintf(out, out_size, "MSG-%04X", (unsigned)(e->msg_next_id & 0xFFFF));
}

/* Find a message in RX or TX by ID */
int mm_find_message(aprs_engine_t* e, const char* id) {
    if (!e || !id) return -1;

    for (int i = 0; i < MAX_RX_MESSAGES; i++) {
        if (e->rx_messages[i].state != APRS_MSG_NEW && e->rx_messages[i].state != APRS_MSG_READ) {
            if (e->rx_messages[i].id[0] && strcmp(e->rx_messages[i].id, id) == 0) {
                return i; /* found in RX, offset 0 */
            }
        }
        if (e->rx_messages[i].state == APRS_MSG_NEW) continue;
        if (e->rx_messages[i].id[0] && strcmp(e->rx_messages[i].id, id) == 0) {
            return i;
        }
    }

    for (int i = 0; i < MAX_TX_MESSAGES; i++) {
        if (e->tx_messages[i].id[0] && strcmp(e->tx_messages[i].id, id) == 0) {
            return MAX_RX_MESSAGES + i; /* found in TX, offset by RX count */
        }
    }

    return -1;
}

/* ========================================================================
 * RECEIVING MESSAGES
 * ======================================================================== */

/* Process an incoming APRS message.
 * Format: :DEST-SSID......:TEXT{msgID}
 *
 * Returns 1 if message was for us (stored or processed),
 * 0 if for someone else (ignored unless receive_all mode).
 */
int mm_receive_message(aprs_engine_t* e, const char* src, const char* dst,
                        const char* text, const char* msg_id) {
    if (!e || !src || !dst || !text) return 0;

    /* Build our full callsign for comparison */
    char our_callsign[24];
    if (e->config.ssid[0]) {
        snprintf(our_callsign, sizeof(our_callsign), "%s-%s",
                 e->config.callsign, e->config.ssid);
    } else {
        snprintf(our_callsign, sizeof(our_callsign), "%s", e->config.callsign);
    }

    /* Check if message is addressed to us */
    int for_us = (strcmp(dst, our_callsign) == 0) ||
                 (strncmp(dst, e->config.callsign, strlen(e->config.callsign)) == 0);

    if (!for_us && !e->receive_all_msgs) {
        return 0; /* not for us and not in "receive all" mode */
    }

    /* Check if this is an ACK for one of our sent messages */
    if (msg_id && msg_id[0] && text && strncmp(text, "ack", 3) == 0) {
        mm_process_ack(e, msg_id);
        return 1;
    }

    /* Check if this is a REJ */
    if (msg_id && msg_id[0] && text && strncmp(text, "rej", 3) == 0) {
        mm_process_rej(e, msg_id);
        return 1;
    }

    /* Check if this is a query response */
    if (text && strncmp(text, "?APRS?", 6) == 0) {
        /* APRS query — respond with station info */
        /* This triggers beacon-like response */
        return 1;
    }

    /* Store the message */
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_RX_MESSAGES; i++) {
        if (e->rx_messages[i].state == APRS_MSG_NEW && e->rx_messages[i].id[0] == '\0') {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        /* Overwrite oldest read message */
        uint64_t oldest = UINT64_MAX;
        for (int i = 0; i < MAX_RX_MESSAGES; i++) {
            if (e->rx_messages[i].state == APRS_MSG_READ &&
                e->rx_messages[i].ts_ms < oldest) {
                oldest = e->rx_messages[i].ts_ms;
                slot = i;
            }
        }
    }
    if (slot < 0) return 0; /* all slots full with unread messages */

    /* Fill message */
    memset(&e->rx_messages[slot], 0, sizeof(aprs_message_t));
    strncpy(e->rx_messages[slot].src, src, sizeof(e->rx_messages[slot].src) - 1);
    strncpy(e->rx_messages[slot].dst, dst, sizeof(e->rx_messages[slot].dst) - 1);
    strncpy(e->rx_messages[slot].text, text, sizeof(e->rx_messages[slot].text) - 1);
    if (msg_id && msg_id[0]) {
        strncpy(e->rx_messages[slot].id, msg_id, sizeof(e->rx_messages[slot].id) - 1);
    } else {
        mm_gen_msg_id(e, e->rx_messages[slot].id, sizeof(e->rx_messages[slot].id));
    }
    e->rx_messages[slot].state = APRS_MSG_NEW;
    e->rx_messages[slot].ts_ms = e->current_time_ms;
    e->rx_msg_count++;

    /* Record stats */
    se_record_msg_rx(e);

    /* Sound notification */
    nm_trigger(e, SND_MESSAGE_RX);

    /* Publish event */
    eb_publish(e, APRS_EVT_MESSAGE_RECEIVED, "message.received",
               &e->rx_messages[slot], sizeof(aprs_message_t));

    return 1;
}

/* ========================================================================
 * SENDING MESSAGES
 * ======================================================================== */

int mm_send_message(aprs_engine_t* e, const char* dest, const char* text) {
    if (!e || !dest || !text) return 0;

    /* Find free TX slot */
    int slot = -1;
    for (int i = 0; i < MAX_TX_MESSAGES; i++) {
        if (e->tx_messages[i].state == APRS_MSG_ACKED ||
            e->tx_messages[i].state == APRS_MSG_REJECTED ||
            e->tx_messages[i].id[0] == '\0') {
            slot = i;
            break;
        }
    }
    if (slot < 0) return 0;

    /* Generate message ID */
    char msg_id[16];
    mm_gen_msg_id(e, msg_id, sizeof(msg_id));

    /* Store in TX messages */
    memset(&e->tx_messages[slot], 0, sizeof(aprs_message_t));
    strncpy(e->tx_messages[slot].src, e->config.callsign, sizeof(e->tx_messages[slot].src) - 1);
    strncpy(e->tx_messages[slot].dst, dest, sizeof(e->tx_messages[slot].dst) - 1);
    strncpy(e->tx_messages[slot].text, text, sizeof(e->tx_messages[slot].text) - 1);
    strncpy(e->tx_messages[slot].id, msg_id, sizeof(e->tx_messages[slot].id) - 1);
    e->tx_messages[slot].state = APRS_MSG_SENT;
    e->tx_messages[slot].ts_ms = e->current_time_ms;
    e->tx_messages[slot].retry_count = 0;
    e->tx_msg_count++;

    /* Record stats */
    se_record_msg_tx(e);

    /* Publish event */
    eb_publish(e, APRS_EVT_MESSAGE_SENT, "message.sent",
               &e->tx_messages[slot], sizeof(aprs_message_t));

    return 1;
}

/* ========================================================================
 * ACK / REJ PROCESSING
 * ======================================================================== */

void mm_process_ack(aprs_engine_t* e, const char* msg_id) {
    if (!e || !msg_id) return;

    for (int i = 0; i < MAX_TX_MESSAGES; i++) {
        if (strcmp(e->tx_messages[i].id, msg_id) == 0) {
            e->tx_messages[i].state = APRS_MSG_ACKED;
            eb_publish(e, APRS_EVT_MESSAGE_STATE, "message.state",
                       &e->tx_messages[i], sizeof(aprs_message_t));
            return;
        }
    }
}

void mm_process_rej(aprs_engine_t* e, const char* msg_id) {
    if (!e || !msg_id) return;

    for (int i = 0; i < MAX_TX_MESSAGES; i++) {
        if (strcmp(e->tx_messages[i].id, msg_id) == 0) {
            e->tx_messages[i].state = APRS_MSG_REJECTED;
            eb_publish(e, APRS_EVT_MESSAGE_STATE, "message.state",
                       &e->tx_messages[i], sizeof(aprs_message_t));
            return;
        }
    }
}

/* ========================================================================
 * RETRY
 * ======================================================================== */

void mm_retry_tick(aprs_engine_t* e) {
    if (!e) return;

    for (int i = 0; i < MAX_TX_MESSAGES; i++) {
        if (e->tx_messages[i].state == APRS_MSG_SENT) {
            uint64_t elapsed = e->current_time_ms - e->tx_messages[i].ts_ms;
            uint64_t retry_interval = (uint64_t)e->msg_retry_delay_sec * 1000;

            if (elapsed >= retry_interval && e->tx_messages[i].retry_count < e->msg_retries) {
                /* Re-queue for transmission */
                e->tx_messages[i].retry_count++;
                e->tx_messages[i].ts_ms = e->current_time_ms;

                eb_publish(e, APRS_EVT_MESSAGE_SENT, "message.sent",
                           &e->tx_messages[i], sizeof(aprs_message_t));
                break; /* one retry per tick */
            }
        }
    }
}

/* ========================================================================
 * CONFIGURATION
 * ======================================================================== */

void mm_set_receive_mode(aprs_engine_t* e, uint8_t receive_all) {
    if (!e) return;
    e->receive_all_msgs = receive_all;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void mm_set_retry(aprs_engine_t* e, int retries, int delay_sec) {
    if (!e) return;
    e->msg_retries = (uint8_t)retries;
    e->msg_retry_delay_sec = (uint8_t)delay_sec;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

int mm_mark_read(aprs_engine_t* e, const char* id) {
    if (!e || !id) return 0;

    for (int i = 0; i < MAX_RX_MESSAGES; i++) {
        if (e->rx_messages[i].id[0] && strcmp(e->rx_messages[i].id, id) == 0) {
            e->rx_messages[i].state = APRS_MSG_READ;
            eb_publish(e, APRS_EVT_MESSAGE_STATE, "message.state",
                       &e->rx_messages[i], sizeof(aprs_message_t));
            return 1;
        }
    }
    return 0;
}
