#ifndef KISS_TNC_H
#define KISS_TNC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * KISS TNC Client — connects to Direwolf (or any KISS TNC) over TCP
 *
 * KISS protocol: SLIP-style framing with FEND (0xC0) delimiters and
 * byte escaping for 0xC0 and 0xDB.
 * ======================================================================== */

/* --- KISS protocol constants --- */
#define KISS_FEND          0xC0   /* Frame end/start marker */
#define KISS_FESC          0xDB   /* Escape character */
#define KISS_TFEND         0xDC   /* Escaped FEND value */
#define KISS_TFESC         0xDD   /* Escaped FESC value */

/* --- KISS command codes (low nibble of command byte) --- */
#define KISS_CMD_DATA      0x00   /* Data frame */
#define KISS_CMD_TXDELAY   0x01   /* TX key-up delay (10ms units) */
#define KISS_CMD_PERSIST   0x02   /* Persistence parameter */
#define KISS_CMD_SLOTTIME  0x03   /* Slot time (10ms units) */
#define KISS_CMD_TXTAIL    0x04   /* TX tail time (10ms units) */
#define KISS_CMD_FULLDUP   0x05   /* Full duplex flag */
#define KISS_CMD_SETHW     0x06   /* Set hardware */
#define KISS_CMD_RETURN    0xFF   /* Exit KISS mode */

/* --- Opaque handle --- */
typedef struct kiss_tnc kiss_tnc_t;

/* --- Callback for received frames --- */
typedef void (*kiss_frame_cb)(const uint8_t* data, size_t len, void* user);

/* --- Status codes --- */
typedef enum {
    KISS_OK = 0,
    KISS_ERR_CONNECT = -1,
    KISS_ERR_SOCKET  = -2,
    KISS_ERR_SEND    = -3,
    KISS_ERR_RECV    = -4,
    KISS_ERR_BUF     = -5
} kiss_status_t;

/* ========================================================================
 * TNC Client Lifecycle
 * ======================================================================== */

/* Connect to a KISS TNC at host:port. Returns NULL on failure. */
kiss_tnc_t* kiss_tnc_create(const char* host, uint16_t port);

/* Set the callback invoked for each complete received frame. */
void kiss_tnc_set_rx_callback(kiss_tnc_t* tnc, kiss_frame_cb cb, void* user);

/* Poll the socket for incoming data. Call periodically (non-blocking).
 * Internally calls recv(), buffers partial frames, and invokes the
 * RX callback for each complete frame found. Returns KISS_OK or error. */
kiss_status_t kiss_tnc_poll(kiss_tnc_t* tnc);

/* Send a raw AX.25 frame (WITHOUT CRC — the TNC adds it).
 * Wraps in KISS framing and transmits on the TCP socket. */
kiss_status_t kiss_tnc_send(kiss_tnc_t* tnc, const uint8_t* ax25_data, size_t len);

/* Get the underlying socket file descriptor for use with select()/poll(). */
int kiss_tnc_get_fd(const kiss_tnc_t* tnc);

/* Check whether the TNC is still connected. */
int kiss_tnc_is_connected(const kiss_tnc_t* tnc);

/* Get the last error string (static buffer, not thread-safe). */
const char* kiss_tnc_last_error(const kiss_tnc_t* tnc);

/* Close the connection and free resources. */
void kiss_tnc_destroy(kiss_tnc_t* tnc);

/* ========================================================================
 * Standalone KISS encode/decode utilities (no TNC handle needed)
 * ======================================================================== */

/* Wrap raw bytes in a KISS frame with byte escaping.
 * out[0] = FEND, out[1] = cmd, out[2..] = escaped data, out[N-1] = FEND.
 * Returns encoded length, or 0 if buffer too small. */
size_t kiss_encode(const uint8_t* in, size_t in_len,
                   uint8_t* out, size_t out_max);

/* Decode a single KISS frame (must start and end with FEND).
 * Returns length of decoded AX.25 data, or 0 on error.
 * If cmd is non-NULL, stores the command byte there. */
size_t kiss_decode(const uint8_t* in, size_t in_len,
                   uint8_t* out, size_t out_max, uint8_t* cmd);

#ifdef __cplusplus
}
#endif

#endif /* KISS_TNC_H */
