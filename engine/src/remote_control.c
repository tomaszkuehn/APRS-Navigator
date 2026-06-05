#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Portable case-insensitive string comparison */
static int str_casecmp(const char* a, const char* b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* ========================================================================
 * REMOTE CONTROL — One-shot authentication keys, remote command execution
 *
 * Extracted from:
 *   - config.c (remote_keys, kcrypter)
 * ======================================================================== */

/* Simple pseudo-random number generator (not crypto-secure, but matches
 * the spirit of the original one-shot key system) */
static uint32_t rc_prng(uint32_t* state) {
    *state = (*state * 1103515245 + 12345) & 0x7FFFFFFF;
    return *state;
}

/* Generate 4 one-shot remote control keys */
void rc_generate_keys(aprs_engine_t* e, char keys[4][16]) {
    if (!e || !keys) return;

    /* Use current time as seed */
    uint32_t seed = (uint32_t)(e->current_time_ms & 0xFFFFFFFF);
    uint32_t state = seed;

    for (int i = 0; i < MAX_REMOTE_KEYS; i++) {
        /* Generate a 32-bit key */
        uint32_t k = rc_prng(&state);
        k = (k << 16) | rc_prng(&state);

        /* Store the key */
        e->remote_keys[i].key = k;
        e->remote_keys[i].enabled = 1;

        /* Format as hex string for output */
        snprintf(keys[i], 16, "%08X", k);
    }

    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

/* Execute a remote command received via APRS message.
 * Format: KEY COMMAND [ARGS]
 *
 * Supported commands:
 *   BEACON           - Send beacon now
 *   POSITION lat lon - Set position
 *   TXOFF            - Disable TX
 *   TXON             - Enable TX
 *   DIGIOFF          - Disable digi
 *   DIGION           - Enable digi
 *   MSG dest text    - Send a message
 *
 * Returns 1 on success, 0 on failure.
 */
int rc_execute_command(aprs_engine_t* e, const char* src, const char* msg) {
    if (!e || !src || !msg) return 0;

    /* Parse: first token is the key, rest is command */
    char cmd_buf[256];
    strncpy(cmd_buf, msg, sizeof(cmd_buf) - 1);
    cmd_buf[sizeof(cmd_buf) - 1] = '\0';

    /* Extract key (first space-delimited token) */
    char* key_str = strtok(cmd_buf, " ");
    if (!key_str) return 0;

    uint32_t provided_key = (uint32_t)strtoul(key_str, NULL, 16);

    /* Validate key */
    int key_idx = -1;
    for (int i = 0; i < MAX_REMOTE_KEYS; i++) {
        if (e->remote_keys[i].enabled && e->remote_keys[i].key == provided_key) {
            key_idx = i;
            break;
        }
    }
    if (key_idx < 0) return 0; /* invalid key */

    /* Extract command */
    char* command = strtok(NULL, " ");
    if (!command) return 0;

    /* Execute command */
    if (str_casecmp(command, "BEACON") == 0) {
        bm_send_beacon(e);
    } else if (str_casecmp(command, "TXOFF") == 0) {
        e->config.tx_enabled = 0;
        eb_publish(e, APRS_EVT_TX_STATE_CHANGED, "tx.state", NULL, 0);
    } else if (str_casecmp(command, "TXON") == 0) {
        e->config.tx_enabled = 1;
        eb_publish(e, APRS_EVT_TX_STATE_CHANGED, "tx.state", NULL, 0);
    } else if (str_casecmp(command, "DIGIOFF") == 0) {
        e->config.digi_enabled = 0;
        eb_publish(e, APRS_EVT_TX_STATE_CHANGED, "tx.state", NULL, 0);
    } else if (str_casecmp(command, "DIGION") == 0) {
        e->config.digi_enabled = 1;
        eb_publish(e, APRS_EVT_TX_STATE_CHANGED, "tx.state", NULL, 0);
    } else if (str_casecmp(command, "STATUS") == 0) {
        /* Status query — respond with basic info */
        /* (Handled by the caller formatting a response) */
    } else {
        return 0; /* unknown command */
    }

    /* Invalidate the key after use (one-shot) */
    e->remote_keys[key_idx].enabled = 0;

    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
    return 1;
}
