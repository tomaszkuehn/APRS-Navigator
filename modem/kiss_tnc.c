#include "kiss_tnc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * Platform abstraction — sockets and non-blocking I/O
 * ======================================================================== */

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define INVALID_SOCK (-1)
    #define sock_close(s)    closesocket(s)
    #define sock_valid(s)    ((s) != INVALID_SOCK)
    #define sock_errno       WSAGetLastError()
    #define sock_wouldblock  (sock_errno == WSAEWOULDBLOCK)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    typedef int socket_t;
    #define INVALID_SOCK (-1)
    #define sock_close(s)    close(s)
    #define sock_valid(s)    ((s) >= 0)
    #define sock_errno       errno
    #define sock_wouldblock  (errno == EWOULDBLOCK || errno == EAGAIN)
#endif

/* ========================================================================
 * TNC internal state
 * ======================================================================== */

struct kiss_tnc {
    socket_t     fd;
    int          connected;
    char         host[64];
    uint16_t     port;
    /* RX buffer — accumulates partial KISS frames from TCP */
    uint8_t      rx_buf[8192];
    size_t       rx_len;
    /* Callback for complete frames */
    kiss_frame_cb rx_cb;
    void*         rx_user;
    /* Last error message */
    char         last_error[256];
};

/* ========================================================================
 * KISS encode
 * ======================================================================== */

size_t kiss_encode(const uint8_t* in, size_t in_len,
                   uint8_t* out, size_t out_max) {
    /* Need at minimum: FEND + CMD + (worst-case: in_len*2 for all escapes) + FEND */
    if (out_max < in_len * 2 + 4) return 0;

    size_t pos = 0;
    out[pos++] = KISS_FEND;
    out[pos++] = KISS_CMD_DATA;

    for (size_t i = 0; i < in_len; i++) {
        uint8_t byte = in[i];
        if (byte == KISS_FEND) {
            if (pos + 2 > out_max) return 0;
            out[pos++] = KISS_FESC;
            out[pos++] = KISS_TFEND;
        } else if (byte == KISS_FESC) {
            if (pos + 2 > out_max) return 0;
            out[pos++] = KISS_FESC;
            out[pos++] = KISS_TFESC;
        } else {
            if (pos + 1 > out_max) return 0;
            out[pos++] = byte;
        }
    }

    if (pos + 1 > out_max) return 0;
    out[pos++] = KISS_FEND;
    return pos;
}

/* ========================================================================
 * KISS decode
 * ======================================================================== */

size_t kiss_decode(const uint8_t* in, size_t in_len,
                   uint8_t* out, size_t out_max, uint8_t* cmd) {
    if (in_len < 3) return 0;        /* FEND + CMD + FEND = minimum */
    if (in[0] != KISS_FEND) return 0; /* must start with FEND */

    size_t pos = 1;
    if (cmd) *cmd = in[pos];          /* command byte */
    pos++;

    size_t out_pos = 0;
    int    escape = 0;

    while (pos < in_len && out_pos < out_max) {
        uint8_t byte = in[pos++];

        if (escape) {
            if (byte == KISS_TFEND) {
                out[out_pos++] = KISS_FEND;
            } else if (byte == KISS_TFESC) {
                out[out_pos++] = KISS_FESC;
            }
            /* Unknown escaped value — drop */
            escape = 0;
        } else if (byte == KISS_FESC) {
            escape = 1;
        } else if (byte == KISS_FEND) {
            break; /* closing FEND — frame complete */
        } else {
            out[out_pos++] = byte;
        }
    }

    return out_pos;
}

/* ========================================================================
 * TNC lifecycle
 * ======================================================================== */

kiss_tnc_t* kiss_tnc_create(const char* host, uint16_t port) {
    /* Winsock init on Windows */
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return NULL;
    }
#endif

    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock_valid(fd)) {
#ifdef _WIN32
        WSACleanup();
#endif
        return NULL;
    }

    /* Resolve host */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        /* Not a valid IP string — try hostname resolution */
        struct hostent* he = gethostbyname(host);
        if (!he) {
            sock_close(fd);
#ifdef _WIN32
            WSACleanup();
#endif
            return NULL;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    }

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        sock_close(fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return NULL;
    }

    /* Set non-blocking for select() compatibility */
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

    kiss_tnc_t* tnc = (kiss_tnc_t*)calloc(1, sizeof(kiss_tnc_t));
    if (!tnc) {
        sock_close(fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return NULL;
    }

    tnc->fd        = fd;
    tnc->connected = 1;
    strncpy(tnc->host, host, sizeof(tnc->host) - 1);
    tnc->port      = port;

    return tnc;
}

void kiss_tnc_set_rx_callback(kiss_tnc_t* tnc, kiss_frame_cb cb, void* user) {
    if (!tnc) return;
    tnc->rx_cb   = cb;
    tnc->rx_user = user;
}

/* ========================================================================
 * Poll — read available data, extract complete KISS frames
 * ======================================================================== */

kiss_status_t kiss_tnc_poll(kiss_tnc_t* tnc) {
    if (!tnc || !tnc->connected) return KISS_ERR_CONNECT;

    /* Read whatever is available (non-blocking) */
    uint8_t buf[4096];
    int n = recv(tnc->fd, (char*)buf, sizeof(buf), 0);

    if (n == 0) {
        /* Connection closed by peer */
        tnc->connected = 0;
        snprintf(tnc->last_error, sizeof(tnc->last_error),
                 "Direwolf closed the connection");
        return KISS_ERR_CONNECT;
    }

    if (n < 0) {
        if (sock_wouldblock) {
            return KISS_OK; /* no data available — normal */
        }
        tnc->connected = 0;
        snprintf(tnc->last_error, sizeof(tnc->last_error),
                 "recv error: %d", (int)sock_errno);
        return KISS_ERR_RECV;
    }

    /* Append to RX buffer */
    if (tnc->rx_len + (size_t)n > sizeof(tnc->rx_buf)) {
        /* Buffer overflow — discard oldest data */
        size_t keep = sizeof(tnc->rx_buf) / 2;
        if (keep < tnc->rx_len) {
            memmove(tnc->rx_buf, tnc->rx_buf + tnc->rx_len - keep, keep);
            tnc->rx_len = keep;
        }
    }

    memcpy(tnc->rx_buf + tnc->rx_len, buf, (size_t)n);
    tnc->rx_len += (size_t)n;

    /* Extract complete frames */
    size_t processed = 0;
    while (processed < tnc->rx_len) {
        /* Find opening FEND */
        if (tnc->rx_buf[processed] != KISS_FEND) {
            processed++;
            continue;
        }

        /* Find closing FEND */
        size_t end_pos;
        int found = 0;
        for (end_pos = processed + 1; end_pos < tnc->rx_len; end_pos++) {
            if (tnc->rx_buf[end_pos] == KISS_FEND) {
                found = 1;
                break;
            }
        }
        if (!found) {
            /* Incomplete frame — wait for more data */
            break;
        }

        /* Decode the frame */
        size_t frame_len = end_pos - processed + 1;
        uint8_t decoded[4096];
        uint8_t cmd = 0;
        size_t dlen = kiss_decode(tnc->rx_buf + processed, frame_len,
                                  decoded, sizeof(decoded), &cmd);

        if (dlen > 0 && cmd == KISS_CMD_DATA && tnc->rx_cb) {
            tnc->rx_cb(decoded, dlen, tnc->rx_user);
        }

        processed = end_pos + 1;
    }

    /* Compact buffer */
    if (processed > 0 && processed < tnc->rx_len) {
        memmove(tnc->rx_buf, tnc->rx_buf + processed,
                tnc->rx_len - processed);
        tnc->rx_len -= processed;
    } else if (processed >= tnc->rx_len) {
        tnc->rx_len = 0;
    }

    return KISS_OK;
}

/* ========================================================================
 * Send a frame to the TNC
 * ======================================================================== */

kiss_status_t kiss_tnc_send(kiss_tnc_t* tnc, const uint8_t* data, size_t len) {
    if (!tnc || !tnc->connected) return KISS_ERR_CONNECT;
    if (!data || len == 0) return KISS_ERR_BUF;

    uint8_t kiss_buf[4096];
    size_t klen = kiss_encode(data, len, kiss_buf, sizeof(kiss_buf));
    if (klen == 0) {
        snprintf(tnc->last_error, sizeof(tnc->last_error),
                 "KISS encode buffer too small for %zu bytes", len);
        return KISS_ERR_BUF;
    }

    int sent = send(tnc->fd, (const char*)kiss_buf, (int)klen, 0);
    if (sent < 0) {
        snprintf(tnc->last_error, sizeof(tnc->last_error),
                 "send error: %d", (int)sock_errno);
        return KISS_ERR_SEND;
    }

    return KISS_OK;
}

/* ========================================================================
 * Accessors
 * ======================================================================== */

int kiss_tnc_get_fd(const kiss_tnc_t* tnc) {
    if (!tnc || !tnc->connected) return -1;
    return (int)tnc->fd;
}

int kiss_tnc_is_connected(const kiss_tnc_t* tnc) {
    return tnc && tnc->connected;
}

const char* kiss_tnc_last_error(const kiss_tnc_t* tnc) {
    if (!tnc) return "NULL TNC handle";
    return tnc->last_error;
}

/* ========================================================================
 * Destroy — close socket and free memory
 * ======================================================================== */

void kiss_tnc_destroy(kiss_tnc_t* tnc) {
    if (!tnc) return;
    if (sock_valid(tnc->fd)) {
        sock_close(tnc->fd);
        tnc->fd = INVALID_SOCK;
    }
    tnc->connected = 0;
    free(tnc);
#ifdef _WIN32
    WSACleanup();
#endif
}
