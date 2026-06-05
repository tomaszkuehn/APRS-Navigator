#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================================================
 * AX.25 Frame Encoder/Decoder
 *
 * Wire format: HDLC flags + addresses + control + pid + payload + FCS + flags
 * Bit-stuffing applied after every 5 consecutive 1s
 * NRZI encoding: 0 = transition, 1 = no transition
 * ======================================================================== */

/* CRC-16 table for AX.25 (polynomial 0x8408) */
static uint16_t crc_table[256];
static int crc_table_init = 0;

static void crc_init(void) {
    if (crc_table_init) return;
    for (int i = 0; i < 256; i++) {
        uint16_t crc = (uint16_t)i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0x8408;
            else crc >>= 1;
        }
        crc_table[i] = crc;
    }
    crc_table_init = 1;
}

uint16_t ax25_crc(const uint8_t* data, size_t len) {
    crc_init();
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF];
    return crc ^ 0xFFFF;
}

/* Encode a 7-byte AX.25 address field with SSID */
static size_t encode_addr(uint8_t* out, const char* call, int ssid, int last) {
    for (int i = 0; i < 6; i++) {
        char c = (i < (int)strlen(call)) ? toupper(call[i]) : ' ';
        out[i] = (uint8_t)(c << 1);  /* shift left, LSB stays 0 for ext bit */
    }
    /* Byte 7: SSID in bits 1-4, bit0=extension */
    out[6] = (uint8_t)(((ssid & 0x0F) << 1) | (last ? 0x01 : 0x00));
    return 7;
}

/* Decode a 7-byte AX.25 address field */
static void decode_addr(const uint8_t* in, char* call, int* ssid) {
    for (int i = 0; i < 6; i++) {
        char c = (char)(in[i] >> 1);  /* shift right to get 7-bit char */
        call[i] = (c >= ' ' && c <= 'z') ? c : ' ';
    }
    call[6] = '\0';
    for (int i = 5; i >= 0 && call[i] == ' '; i--) call[i] = '\0';
    *ssid = (in[6] >> 1) & 0x0F;
}

/* Build AX.25 frame from structured data */
size_t ax25_build(const ax25_frame_t* f, uint8_t* out, size_t max) {
    if (!f || !out || max < 32) return 0;
    size_t pos = 0;

    /* Destination */
    if (pos + 7 > max) return 0;
    pos += encode_addr(out + pos, f->dest_call, f->dest_ssid, f->path_count == 0);
    /* Source */
    if (pos + 7 > max) return 0;
    pos += encode_addr(out + pos, f->src_call, f->src_ssid, 1);
    /* Digipeater path */
    for (int i = 0; i < f->path_count && i < AX25_MAX_PATH; i++) {
        if (pos + 7 > max) return 0;
        pos += encode_addr(out + pos, f->path[i], f->path_ssid[i], 1);
    }
    /* Control + PID */
    out[pos++] = f->control ? f->control : 0x03; /* UI frame */
    out[pos++] = f->pid ? f->pid : 0xF0; /* No layer 3 */
    /* Payload */
    if (pos + f->data_len > max) return 0;
    memcpy(out + pos, f->data, f->data_len);
    pos += f->data_len;
    /* CRC-16 */
    uint16_t fcs = ax25_crc(out, pos);
    out[pos++] = (uint8_t)(fcs & 0xFF);
    out[pos++] = (uint8_t)(fcs >> 8);
    return pos;
}

/* Parse APRS text like "SP3ABC-7>APRS,WIDE1-1:!5210N/01655E-Test" into AX.25 wire */
size_t ax25_encode(const char* text, uint8_t* out, size_t max) {
    if (!text || !out) return 0;
    ax25_frame_t f;
    memset(&f, 0, sizeof(f));

    /* Parse header: CALL>SST,PATH1,PATH2:PAYLOAD */
    char buf[512];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* gt = strchr(buf, '>');
    if (!gt) return 0;
    *gt = '\0';
    char* colon = strchr(gt + 1, ':');
    if (!colon) return 0;
    *colon = '\0';

    /* Source callsign */
    char* src = buf;
    char* dash = strchr(src, '-');
    if (dash) { *dash = '\0'; f.src_ssid = (uint8_t)atoi(dash + 1); }
    strncpy(f.src_call, src, 6);

    /* Destination + path */
    char* addr = gt + 1;
    char* comma = strchr(addr, ',');
    if (comma) {
        *comma = '\0';
        /* Parse path */
        char* token = comma + 1;
        while (token && *token && f.path_count < AX25_MAX_PATH) {
            char* next = strchr(token, ',');
            if (next) *next = '\0';
            char* pd = strchr(token, '-');
            int ssid = 0;
            if (pd) { *pd = '\0'; ssid = atoi(pd + 1); }
            strncpy(f.path[f.path_count], token, 6);
            f.path_ssid[f.path_count] = (uint8_t)ssid;
            f.path_count++;
            token = next ? next + 1 : NULL;
        }
    }
    strncpy(f.dest_call, addr, 6);

    /* Payload */
    f.data_len = strlen(colon + 1);
    if (f.data_len > AX25_MAX_DATA) f.data_len = AX25_MAX_DATA;
    memcpy(f.data, colon + 1, f.data_len);

    return ax25_build(&f, out, max);
}

/* Decode AX.25 wire format to human-readable text */
size_t ax25_decode(const uint8_t* data, size_t len, char* out, size_t max) {
    if (!data || len < 16 || !out || max < 64) return 0;
    size_t pos = 0;

    /* Destination */
    char call[8]; int ssid, last = 0;
    decode_addr(data, call, &ssid);
    decode_addr(data + 7, call, &ssid); /* skip dest, decode source into call */
    pos = 14; /* min: dest + source */

    /* Check if there are more addresses (digipeaters) */
    while (pos + 7 <= len && !(data[pos + 6] & 0x01)) {
        pos += 7;
    }
    pos += 7; /* last address */

    /* Now at control field */
    if (pos + 2 > len) return 0;
    uint8_t ctrl = data[pos];
    uint8_t pid  = data[pos + 1];
    pos += 2;

    /* Payload (everything before FCS) */
    size_t payload_len = len - pos - 2; /* minus 2 for FCS */
    if (payload_len > max - 32) payload_len = max - 32;

    /* Build text: SRC>DST,PATH:PAYLOAD */
    /* Source is at offset 7 in the frame */
    char src_call[8], dst_call[8];
    int src_ssid, dst_ssid;
    decode_addr(data + 7, src_call, &src_ssid);
    decode_addr(data, dst_call, &dst_ssid);

    int n = snprintf(out, max, "%s-%d>%s-%d", src_call, src_ssid, dst_call, dst_ssid);

    /* Extract path from positions 14 to pos-1 */
    for (size_t i = 14; i + 7 <= pos - 1; i += 7) {
        char pc[8]; int ps;
        decode_addr(data + i, pc, &ps);
        n += snprintf(out + n, max - n, ",%s-%d", pc, ps);
    }

    n += snprintf(out + n, max - n, ":%s", data + pos);
    return (size_t)n;
}
