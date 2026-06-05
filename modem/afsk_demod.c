/* ========================================================================
 * AFSK Demodulator — two-pass: raw decode + post-process destuffing
 * ======================================================================== */
#include "modem.h"
#include <string.h>
#include <stdlib.h>

int afsk_demod_buffer(const int16_t* samples, size_t count,
                      afsk_frame_cb callback, void* user) {
    if (!samples || count < 100) return 0;

    size_t spb = AFSK_SAMPLE_RATE / AFSK_BAUD_RATE;

    /* ================================================================
     * PASS 1: Decode raw bits without destuffing, find HDLC frames
     * ================================================================ */
    int last_raw = 1;
    uint8_t byte = 0;
    int bc = 0;
    int in_frame = 0;
    uint8_t raw_buf[2048];
    size_t raw_len = 0;
    int found = 0;

    for (size_t i = 0; i + spb <= count; i += spb) {
        int cross = 0;
        for (size_t j = 1; j < spb; j++)
            if ((samples[i + j] ^ samples[i + j - 1]) < 0) cross++;

        int raw = (cross >= 3) ? 0 : 1;  /* 3+ crossings = space(2200Hz) = 0 */
        int dec = (raw != last_raw) ? 0 : 1;  /* NRZI decode */
        last_raw = raw;

        /* Accumulate LSB first, no destuffing yet */
        byte = (uint8_t)((byte >> 1) | (dec << 7));
        bc++;

        if (bc == 8) {
            bc = 0;
            if (byte == 0x7E) {
                if (!in_frame) {
                    in_frame = 1;
                    raw_len = 0;
                } else if (raw_len > 0) {
                    /* Frame found between flags — raw_len bytes of stuffed data */
                    /* ================================================================
                     * PASS 2: Destuff the byte stream and verify CRC
                     * ================================================================ */
                    uint8_t destuffed[1024];
                    size_t ds_len = 0;
                    int ones = 0;
                    int dbit = 0;
                    uint8_t acc = 0;

                    for (size_t k = 0; k < raw_len; k++) {
                        for (int b = 0; b < 8; b++) {
                            int bit = (raw_buf[k] >> b) & 1;
                            if (bit) {
                                ones++;
                            } else {
                                if (ones == 5) {
                                    ones = 0;
                                    continue;  /* skip stuff bit */
                                }
                                ones = 0;
                            }
                            /* Accumulate destuffed bit */
                            acc = (uint8_t)((acc >> 1) | (bit << 7));
                            dbit++;
                            if (dbit == 8) {
                                dbit = 0;
                                if (ds_len < sizeof(destuffed))
                                    destuffed[ds_len++] = acc;
                                acc = 0;
                            }
                        }
                    }

                    /* Verify CRC on destuffed data */
                    if (ds_len > 2) {
                        uint16_t calc = ax25_crc(destuffed, ds_len - 2);
                        uint16_t recv = (uint16_t)destuffed[ds_len - 2] |
                                       ((uint16_t)destuffed[ds_len - 1] << 8);
                        if (calc == recv) {
                            found++;
                            if (callback) callback(destuffed, ds_len - 2, user);
                        }
                    }
                    in_frame = 0;
                    raw_len = 0;
                }
            } else if (in_frame && raw_len < sizeof(raw_buf) - 1) {
                raw_buf[raw_len++] = byte;
            }
            byte = 0;
        }
    }
    return found;
}
