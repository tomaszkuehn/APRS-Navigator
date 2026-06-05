#include "modem.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========================================================================
 * AFSK Modulator — 1200 baud Bell 202
 *
 * Mark  = 1200 Hz (logical 1)
 * Space = 2200 Hz (logical 0)
 * Sample rate: 44100 Hz → ~36.75 samples per bit
 * NRZI encoding: bit change = 0, no change = 1
 * HDLC bit stuffing: after 5 consecutive 1s, insert a 0
 * ======================================================================== */

void afsk_mod_init(afsk_mod_t* m) {
    m->mark_phase = 0.0;
    m->space_phase = 0.0;
    m->last_phase = 0.0;
    m->last_bit = 1; /* idle = mark tone */
    m->bit_count = 0;
}

/* Modulate a single NRZI bit into audio samples */
int afsk_mod_bit(afsk_mod_t* m, int bit, int16_t* out, int max_samples) {
    double freq = bit ? AFSK_MARK_HZ : AFSK_SPACE_HZ;
    double phase_step = 2.0 * M_PI * freq / AFSK_SAMPLE_RATE;
    int n = 0;

    for (int i = 0; i < AFSK_SAMPLES_PER_BIT && n < max_samples; i++) {
        double phase = (bit ? m->mark_phase : m->space_phase);
        double sample = sin(phase);
        /* Scale to 16-bit: use 75% amplitude to avoid clipping */
        out[n++] = (int16_t)(sample * 24576.0);
        if (bit)
            m->mark_phase = fmod(m->mark_phase + phase_step, 2.0 * M_PI);
        else
            m->space_phase = fmod(m->space_phase + phase_step, 2.0 * M_PI);
    }

    /* Reset the OTHER frequency's phase for phase-continuous switching */
    if (bit)
        m->space_phase = m->mark_phase;
    else
        m->mark_phase = m->space_phase;

    return n;
}

/* Apply HDLC bit stuffing: returns stuffed bit count */
static size_t stuff_bits(const uint8_t* data, size_t len, uint8_t* out, size_t out_max) {
    size_t out_bit = 0;
    int ones = 0;

    for (size_t i = 0; i < len; i++) {
        for (int b = 0; b < 8; b++) {
            int bit = (data[i] >> b) & 1;
            if (out_bit / 8 >= out_max) return 0;
            if (bit) {
                out[out_bit / 8] |= (uint8_t)(1 << (out_bit % 8));
                ones++;
            } else {
                out[out_bit / 8] &= (uint8_t)(~(1 << (out_bit % 8)));
                ones = 0;
            }
            out_bit++;
            if (ones == 5) {
                /* Insert stuff bit (0) */
                if (out_bit / 8 >= out_max) return 0;
                out[out_bit / 8] &= (uint8_t)(~(1 << (out_bit % 8)));
                out_bit++;
                ones = 0;
            }
        }
    }
    return out_bit;
}

/* Modulate complete AX.25 frame (with HDLC flags, bit stuffing, NRZI, FCS) */
size_t afsk_mod_frame(const uint8_t* ax25_data, size_t ax25_len,
                      int16_t* out, size_t out_max) {
    /* Step 1: Compute FCS and append */
    uint8_t framed[512];
    memcpy(framed, ax25_data, ax25_len);
    uint16_t fcs = ax25_crc(ax25_data, ax25_len);
    framed[ax25_len]     = (uint8_t)(fcs & 0xFF);
    framed[ax25_len + 1] = (uint8_t)(fcs >> 8);
    size_t total = ax25_len + 2;

    /* Step 2: Bit stuff */
    uint8_t stuffed[1024];
    memset(stuffed, 0, sizeof(stuffed));
    size_t stuffed_bits = stuff_bits(framed, total, stuffed, sizeof(stuffed));
    if (stuffed_bits == 0) return 0;
    size_t stuffed_bytes = (stuffed_bits + 7) / 8;

    /* Step 3: Build HDLC frame with flags (0x7E = 01111110) */
    /* Start flag */
    uint8_t hdlc[2048];
    size_t hdlc_pos = 0;
    hdlc[hdlc_pos++] = 0x7E;

    /* Stuffed data */
    memcpy(hdlc + hdlc_pos, stuffed, stuffed_bytes);
    hdlc_pos += stuffed_bytes;

    /* End flag */
    hdlc[hdlc_pos++] = 0x7E;

    /* Step 4: NRZI encode and modulate */
    afsk_mod_t mod;
    afsk_mod_init(&mod);
    int nrzi_bit = 1; /* idle = mark = 1, must match demodulator init */

    size_t sample_count = 0;
    for (size_t i = 0; i < hdlc_pos && sample_count < out_max; i++) {
        for (int b = 0; b < 8; b++) {
            int bit = (hdlc[i] >> b) & 1;
            /* NRZI: 0 = transition, 1 = no change */
            if (bit == 0) nrzi_bit = !nrzi_bit;
            int n = afsk_mod_bit(&mod, nrzi_bit,
                                  out + sample_count,
                                  (int)(out_max - sample_count));
            sample_count += (size_t)n;
        }
    }

    return sample_count;
}

/* ========================================================================
 * AFSK Demodulator
 *
 * Uses quadrature correlator: multiply incoming signal with sin/cos
 * at mark and space frequencies, compute magnitude, compare.
 * ======================================================================== */

void afsk_demod_init(afsk_demod_t* d) {
    memset(d, 0, sizeof(*d));
}

/* Feed one audio sample (stub — real work in afsk_demod_buffer) */
int afsk_demod_sample(afsk_demod_t* d, int16_t sample, uint8_t* byte_out) {
    (void)d; (void)sample; (void)byte_out;
    return 0;
}
/* afsk_demod_buffer is in afsk_demod.c */
