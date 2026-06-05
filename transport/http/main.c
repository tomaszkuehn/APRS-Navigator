#include "http_server.h"
#include "../../engine/include/aprs_engine_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static aprs_http_server_t* g_srv = NULL;

static void on_signal(int sig) {
    (void)sig;
    printf("\nShutting down...\n");
    if (g_srv) aprs_http_server_stop(g_srv);
}

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    /* Create engine with default config */
    aprs_engine_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.callsign, "SP3ABC-7", sizeof(cfg.callsign) - 1);
    strncpy(cfg.ssid, "7", sizeof(cfg.ssid) - 1);
    cfg.symbol_table = '/';
    cfg.symbol_code = '>';
    cfg.tx_enabled = 1;
    cfg.digi_enabled = 1;
    cfg.beacon_enabled = 1;
    cfg.unique_frames_only = 1;

    aprs_engine_t* engine = NULL;
    if (aprs_engine_create(&cfg, &engine) != APRS_OK) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }
    aprs_engine_start(engine);
    printf("Engine v%s started\n", aprs_engine_version());

    /* Feed a few test frames so there's data to query */
    const char* test_frames[] = {
        "SP3XYZ-9>APRS,WIDE1-1:!5210.12N/01655.22E-Test station",
        "SP9AAA-1>APRS,RELAY:!5130.50N/01610.30E&Home QTH",
        "SQ2BBB-2>APRS,WIDE2-2:=5200.00N/01650.00E#PHG5130 Poznan",
    };
    for (int i = 0; i < 3; i++) {
        aprs_engine_feed_frame(engine, test_frames[i], strlen(test_frames[i]));
        for (int t = 0; t < 5; t++) aprs_engine_tick(engine, 100);
    }

    /* Create and run HTTP server */
    g_srv = aprs_http_server_create(port, engine);
    if (!g_srv) {
        fprintf(stderr, "Failed to create HTTP server on port %d\n", port);
        aprs_engine_destroy(engine);
        return 1;
    }

    printf("HTTP server listening on http://localhost:%d\n", port);
    printf("Endpoints:\n");
    printf("  GET  http://localhost:%d/v2/state\n", port);
    printf("  GET  http://localhost:%d/v2/stations\n", port);
    printf("  GET  http://localhost:%d/v2/stations/SP3XYZ-9\n", port);
    printf("  GET  http://localhost:%d/v2/stats\n", port);
    printf("  GET  http://localhost:%d/v2/gps\n", port);
    printf("  GET  http://localhost:%d/v2/messages\n", port);
    printf("  POST http://localhost:%d/v2/beacon/send\n", port);
    printf("  POST http://localhost:%d/v2/messages/send\n", port);
    printf("\nPress Ctrl+C to stop\n\n");

    int result = aprs_http_server_run(g_srv);

    aprs_http_server_destroy(g_srv);
    aprs_engine_destroy(engine);
    return result;
}
