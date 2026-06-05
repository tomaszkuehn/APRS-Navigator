#ifndef APRS_MODEM_H
#define APRS_MODEM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * AFSK 1200 Modem for APRS
 *
 * Bell 202 tones: 1200 Hz = mark (1), 2200 Hz = space (0)
 * 1200 baud, NRZI encoded, AX.25 HDLC framing
 * ======================================================================== */

/* --- Configuration --- */
#define AFSK_SAMPLE_RATE   48000  /* divides evenly: 48000/1200 = 40 samples/bit */
#define AFSK_BAUD_RATE      1200
#define AFSK_MARK_HZ        1200
#define AFSK_SPACE_HZ       2200
#define AFSK_SAMPLES_PER_BIT (AFSK_SAMPLE_RATE / AFSK_BAUD_RATE)  /* ~36.75 */

/* --- WAV header --- */
typedef struct {
    char     riff[4];       /* "RIFF" */
    uint32_t file_size;
    char     wave[4];       /* "WAVE" */
    char     fmt[4];        /* "fmt " */
    uint32_t fmt_size;      /* 16 */
    uint16_t audio_format;  /* 1 = PCM */
    uint16_t num_channels;  /* 1 = mono */
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_tag[4];   /* "data" */
    uint32_t data_size;
} wav_header_t;

/* --- Modem result codes --- */
typedef enum {
    MODEM_OK = 0,
    MODEM_ERR_FILE,
    MODEM_ERR_MEMORY,
    MODEM_ERR_FORMAT,
    MODEM_ERR_CRC,
    MODEM_ERR_SYNC
} modem_status_t;

/* --- AFSK modulator state --- */
typedef struct {
    double mark_phase;
    double space_phase;
    double last_phase;
    int    last_bit;        /* previous bit for NRZI */
    int    bit_count;       /* consecutive 1s for bit stuffing */
} afsk_mod_t;

/* --- AFSK demodulator state --- */
typedef struct {
    double mark_i, mark_q;   /* mark correlator */
    double space_i, space_q;  /* space correlator */
    double sample_clock;
    int    last_raw_bit;
    int    bit_stuff_count;
    int    sync;              /* found HDLC flag */
    int    byte_bits;
    uint8_t current_byte;
} afsk_demod_t;

/* --- AX.25 frame --- */
#define AX25_MAX_DATA   256
#define AX25_MAX_PATH     8
#define AX25_MAX_ADDR_LEN 7

typedef struct {
    char     dest_call[7];
    uint8_t  dest_ssid;
    char     src_call[7];
    uint8_t  src_ssid;
    char     path[AX25_MAX_PATH][7];
    uint8_t  path_ssid[AX25_MAX_PATH];
    int      path_count;
    uint8_t  control;
    uint8_t  pid;
    uint8_t  data[AX25_MAX_DATA];
    size_t   data_len;
} ax25_frame_t;

/* ========================================================================
 * WAV File I/O
 * ======================================================================== */

/* Write mono 16-bit PCM WAV file. Returns 0 on success. */
int wav_write(const char* filepath, const int16_t* samples, size_t count,
              int sample_rate);

/* Read mono 16-bit PCM WAV file. Returns NULL on failure. Caller must free. */
int16_t* wav_read(const char* filepath, size_t* out_count, int* out_rate);

/* ========================================================================
 * AFSK Modulator
 * ======================================================================== */

/* Initialize modulator state */
void afsk_mod_init(afsk_mod_t* m);

/* Modulate a single NRZI bit into audio samples.
 * Returns number of samples written (typically ~36 at 44100 Hz). */
int afsk_mod_bit(afsk_mod_t* m, int bit, int16_t* out, int max_samples);

/* Modulate a complete AX.25 frame (with flags, bit stuffing, NRZI, CRC).
 * Returns number of audio samples written. */
size_t afsk_mod_frame(const uint8_t* ax25_data, size_t ax25_len,
                      int16_t* out, size_t out_max);

/* ========================================================================
 * AFSK Demodulator
 * ======================================================================== */

/* Initialize demodulator state */
void afsk_demod_init(afsk_demod_t* d);

/* Feed one audio sample to the demodulator.
 * Returns 1 if a complete byte was decoded (byte stored in *byte_out),
 * 2 if an HDLC flag was detected (sync),
 * 0 otherwise. */
int afsk_demod_sample(afsk_demod_t* d, int16_t sample, uint8_t* byte_out);

/* Demodulate a buffer of audio samples, extract AX.25 frames.
 * Calls on_frame(frame_data, frame_len, user_data) for each valid frame.
 * Returns number of frames found. */
typedef void (*afsk_frame_cb)(const uint8_t* data, size_t len, void* user);
int afsk_demod_buffer(const int16_t* samples, size_t count,
                      afsk_frame_cb callback, void* user);

/* ========================================================================
 * AX.25 Frame Encoder/Decoder
 * ======================================================================== */

/* Encode an APRS text frame into AX.25 wire format.
 * Format: CALLSIGN-SSID>DST,PATH1,PATH2:PAYLOAD
 * Returns length of encoded data, or 0 on error. */
size_t ax25_encode(const char* aprs_text, uint8_t* out, size_t out_max);

/* Build AX.25 from structured frame */
size_t ax25_build(const ax25_frame_t* f, uint8_t* out, size_t out_max);

/* Decode AX.25 wire format into a human-readable APRS text string.
 * Returns length of text, or 0 on error. */
size_t ax25_decode(const uint8_t* ax25_data, size_t ax25_len,
                   char* out_text, size_t out_max);

/* Compute AX.25 CRC-16 (FCS) */
uint16_t ax25_crc(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* APRS_MODEM_H */
