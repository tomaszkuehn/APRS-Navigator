#include "modem.h"
#include "kiss_tnc.h"
#include "../engine/include/aprs_engine_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #define sock_errno WSAGetLastError()
    #define sock_eintr  WSAEINTR
#else
    #include <signal.h>
    #include <errno.h>
    #include <sys/select.h>
    #define sock_errno errno
    #define sock_eintr  EINTR
#endif

/* Signal flag for clean shutdown */
static volatile int g_running = 1;

#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        g_running = 0;
        return TRUE;
    }
    return FALSE;
}
#else
static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}
#endif

/* ========================================================================
 * APRS Software Modem CLI
 *
 * TX: aprs-modem tx "APRS text" output.wav
 *     Converts APRS text frame to AFSK audio, saves as WAV
 *
 * RX: aprs-modem rx input.wav
 *     Reads WAV file, demodulates AFSK, prints decoded frames
 *
 * Engine mode:
 *   aprs-modem engine input.wav
 *   Reads WAV, demodulates, feeds frames directly into engine
 * ======================================================================== */

static void print_usage(const char* prog) {
    printf("APRS Software Modem (1200 baud AFSK Bell 202)\n\n");
    printf("Usage:\n");
    printf("  %s tx \"APRS-TEXT\" output.wav    - Encode frame to audio\n", prog);
    printf("  %s rx input.wav [input2.wav...]  - Decode audio to frames\n", prog);
    printf("  %s engine input.wav              - Feed decoded frames to engine\n", prog);
    printf("  %s kiss [host:port]              - KISS TNC mode (Direwolf)\n", prog);
    printf("\nExamples:\n");
    printf("  %s tx \"SP3ABC-7>APRS,WIDE1-1:!5210N/01655E-Test\" beacon.wav\n", prog);
    printf("  %s rx recording.wav\n", prog);
    printf("  %s engine recording.wav\n", prog);
    printf("  %s kiss                          - Connect to local Direwolf\n", prog);
    printf("  %s kiss 192.168.1.5:8100         - Connect to remote TNC\n", prog);
}

/* Callback for demodulator — called for each valid frame */
static int frame_count = 0;

static void on_frame(const uint8_t* data, size_t len, void* user) {
    aprs_engine_t* engine = (aprs_engine_t*)user;
    frame_count++;

    /* Decode AX.25 to text */
    char text[512];
    size_t text_len = ax25_decode(data, len, text, sizeof(text));
    if (text_len > 0 && text_len < sizeof(text)) {
        text[text_len] = '\0';
        printf("[FRAME %d] len=%zu  %s\n", frame_count, len, text);

        /* Feed to engine if provided */
        if (engine) {
            aprs_engine_feed_frame(engine, text, text_len);
            for (int t = 0; t < 5; t++) aprs_engine_tick(engine, 100);
        }
    } else {
        printf("[FRAME %d] len=%zu  (decode failed, raw hex)\n", frame_count, len);
    }
}

/* ==================================================================
 * KISS mode callbacks
 * ================================================================== */

/* Context passed to the engine TX subscription callback */
typedef struct {
    kiss_tnc_t* tnc;
    int         count;
} kiss_tx_ctx_t;

/* KISS RX callback — called for each complete frame from the TNC */
static void on_kiss_frame(const uint8_t* data, size_t len, void* user) {
    aprs_engine_t* engine = (aprs_engine_t*)user;

    char text[512];
    size_t text_len = ax25_decode(data, len, text, sizeof(text));
    if (text_len > 0 && text_len < sizeof(text)) {
        text[text_len] = '\0';
        printf("[RX] %s\n", text);
        aprs_engine_feed_frame(engine, text, text_len);
    } else {
        printf("[RX] AX.25 decode failed, %zu bytes\n", len);
    }
}

/* Engine TX callback — sends outgoing frames to the TNC via KISS */
static void on_engine_tx(const aprs_event_t* ev, void* user) {
    kiss_tx_ctx_t* ctx = (kiss_tx_ctx_t*)user;
    const aprs_frame_t* frame = (const aprs_frame_t*)ev->payload;

    /* Encode APRS text to AX.25 wire format (includes CRC-16) */
    uint8_t ax25[512];
    size_t ax25_len = ax25_encode(frame->raw, ax25, sizeof(ax25));
    if (ax25_len < 3) {
        fprintf(stderr, "[TX] Failed to encode: %s\n", frame->raw);
        return;
    }

    /* Strip CRC (last 2 bytes) — Direwolf adds its own */
    size_t kiss_len = ax25_len - 2;

    ctx->count++;
    printf("[TX %d] %s\n", ctx->count, frame->raw);

    kiss_status_t st = kiss_tnc_send(ctx->tnc, ax25, kiss_len);
    if (st != KISS_OK) {
        fprintf(stderr, "[TX] KISS send failed: %s\n",
                kiss_tnc_last_error(ctx->tnc));
    }
}

/* ==================================================================
 * KISS TNC mode — connects to Direwolf, runs the engine loop
 * ================================================================== */

static int run_kiss_mode(const char* host, int port) {
    /* --- Create engine --- */
    aprs_engine_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.callsign, "NOCALL", sizeof(cfg.callsign) - 1);
    cfg.tx_enabled    = 1;
    cfg.digi_enabled  = 1;
    cfg.beacon_enabled = 1;

    aprs_engine_t* engine = NULL;
    if (aprs_engine_create(&cfg, &engine) != APRS_OK) {
        fprintf(stderr, "ERROR: Engine creation failed\n");
        return 1;
    }
    aprs_engine_start(engine);
    printf("APRS Engine v%s ready\n", aprs_engine_version());

    /* --- Connect to Direwolf --- */
    kiss_tnc_t* tnc = kiss_tnc_create(host, (uint16_t)port);
    if (!tnc) {
        fprintf(stderr, "ERROR: Cannot connect to Direwolf at %s:%d\n"
                        "       Is Direwolf running? (direwolf -c direwolf.conf)\n",
                host, port);
        aprs_engine_destroy(engine);
        return 1;
    }
    printf("Connected to Direwolf at %s:%d\n", host, port);

    /* --- Wire up callbacks --- */
    kiss_tnc_set_rx_callback(tnc, on_kiss_frame, engine);

    kiss_tx_ctx_t tx_ctx;
    tx_ctx.tnc   = tnc;
    tx_ctx.count = 0;
    aprs_engine_subscribe(engine, "frame.tx", on_engine_tx, &tx_ctx);

    /* --- Install signal handler --- */
#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
#endif

    /* --- Main loop --- */
    printf("KISS TNC mode running. Press Ctrl+C to exit.\n\n");

    while (g_running) {
        int kiss_fd = kiss_tnc_get_fd(tnc);

        fd_set read_fds;
        FD_ZERO(&read_fds);
        if (kiss_fd >= 0) {
            FD_SET(kiss_fd, &read_fds);
        }

        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 50000; /* 50 ms */

        int activity = select(kiss_fd + 1, &read_fds, NULL, NULL, &tv);

        if (activity < 0) {
            if (sock_errno != sock_eintr) {
                perror("select");
                break;
            }
            continue;
        }

        if (activity > 0 && kiss_fd >= 0 && FD_ISSET(kiss_fd, &read_fds)) {
            kiss_status_t st = kiss_tnc_poll(tnc);
            if (st != KISS_OK) {
                fprintf(stderr, "KISS: %s\n", kiss_tnc_last_error(tnc));
                break;
            }
        }

        /* Tick the engine (this may trigger TX callbacks synchronously) */
        aprs_engine_tick(engine, 50);
    }

    /* --- Cleanup --- */
    printf("\nShutting down...\n");
    aprs_engine_stop(engine);
    aprs_engine_destroy(engine);
    kiss_tnc_destroy(tnc);
    printf("KISS TNC mode ended.\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(argv[0]); return 1; }

    const char* mode = argv[1];

    /* ==================================================================
     * KISS MODE: Direwolf TNC (handled first — needs only mode arg)
     * ================================================================== */
    if (strcmp(mode, "kiss") == 0) {
        const char* host = "127.0.0.1";
        int port = 8001;

        if (argc >= 3) {
            char* addr = argv[2];
            char* colon = strchr(addr, ':');
            if (colon) {
                *colon = '\0';
                host = addr;
                port = atoi(colon + 1);
                if (port <= 0) port = 8001;
            } else {
                /* Could be hostname or bare port number */
                int p = atoi(addr);
                if (p > 0 && strspn(addr, "0123456789") == strlen(addr)) {
                    port = p;
                } else {
                    host = addr;
                }
            }
        }

        return run_kiss_mode(host, port);
    }

    /* All other modes need at least one more argument */
    if (argc < 3) { print_usage(argv[0]); return 1; }

    /* ==================================================================
     * TX MODE: text → WAV
     * ================================================================== */
    if (strcmp(mode, "tx") == 0) {
        if (argc < 4) { print_usage(argv[0]); return 1; }
        const char* text = argv[2];
        const char* out_file = argv[3];

        printf("TX: Encoding APRS frame...\n");
        printf("    Text: %s\n", text);

        /* Step 1: Encode text to AX.25 wire format */
        uint8_t ax25[512];
        size_t ax25_len = ax25_encode(text, ax25, sizeof(ax25));
        if (ax25_len == 0) {
            fprintf(stderr, "ERROR: Failed to encode AX.25 frame\n");
            return 1;
        }
        printf("    AX.25: %zu bytes\n", ax25_len);

        /* Step 2: Modulate to audio */
        int16_t* audio = (int16_t*)malloc(AFSK_SAMPLE_RATE * 60); /* up to 60 seconds */
        if (!audio) { fprintf(stderr, "ERROR: Out of memory\n"); return 1; }
        size_t samples = afsk_mod_frame(ax25, ax25_len, audio, AFSK_SAMPLE_RATE * 60);
        if (samples == 0) { fprintf(stderr, "ERROR: Modulation failed\n"); free(audio); return 1; }

        double duration = (double)samples / AFSK_SAMPLE_RATE;
        printf("    Audio: %zu samples (%.2f seconds)\n", samples, duration);

        /* Step 3: Write WAV */
        printf("    Writing %s...\n", out_file);
        if (wav_write(out_file, audio, samples, AFSK_SAMPLE_RATE) != 0) {
            fprintf(stderr, "ERROR: Failed to write WAV file\n");
            free(audio); return 1;
        }
        printf("OK: %s written (%.1f sec, 44100 Hz mono 16-bit PCM)\n", out_file, duration);
        free(audio);
    }

    /* ==================================================================
     * RX MODE: WAV → text
     * ================================================================== */
    else if (strcmp(mode, "rx") == 0) {
        int total = 0;
        for (int i = 2; i < argc; i++) {
            printf("RX: Processing %s...\n", argv[i]);
            size_t count = 0; int rate = 0;
            int16_t* samples = wav_read(argv[i], &count, &rate);
            if (!samples) { fprintf(stderr, "ERROR: Cannot read %s\n", argv[i]); continue; }
            printf("    %zu samples, %d Hz, %.1f seconds\n", count, rate, (double)count / rate);

            frame_count = 0;
            afsk_demod_buffer(samples, count, on_frame, NULL);
            printf("    Found %d frames\n", frame_count);
            total += frame_count;
            free(samples);
        }
        printf("Total: %d frames decoded\n", total);
    }

    /* ==================================================================
     * ENGINE MODE: WAV → engine
     * ================================================================== */
    else if (strcmp(mode, "engine") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 1; }

        /* Create engine */
        aprs_engine_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        strncpy(cfg.callsign, "NOCALL", sizeof(cfg.callsign) - 1);
        cfg.tx_enabled = 1; cfg.digi_enabled = 1; cfg.beacon_enabled = 1;
        aprs_engine_t* engine = NULL;
        if (aprs_engine_create(&cfg, &engine) != APRS_OK) {
            fprintf(stderr, "ERROR: Engine creation failed\n"); return 1;
        }
        aprs_engine_start(engine);
        printf("Engine v%s ready\n", aprs_engine_version());

        int total = 0;
        for (int i = 2; i < argc; i++) {
            printf("RX: Processing %s...\n", argv[i]);
            size_t count = 0; int rate = 0;
            int16_t* samples = wav_read(argv[i], &count, &rate);
            if (!samples) { fprintf(stderr, "ERROR: Cannot read %s\n", argv[i]); continue; }

            frame_count = 0;
            afsk_demod_buffer(samples, count, on_frame, engine);
            printf("    Fed %d frames to engine\n", frame_count);
            total += frame_count;
            free(samples);
        }
        printf("Total: %d frames fed to engine\n", total);

        /* Show engine state */
        aprs_stats_t st;
        aprs_engine_get_stats(engine, &st);
        printf("Engine stats: rx=%u tx=%u st=%u\n", st.rx_frames, st.tx_frames, st.msg_received);
        aprs_engine_destroy(engine);
    }
    else {
        print_usage(argv[0]); return 1;
    }

    return 0;
}
