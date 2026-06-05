#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "../../engine/include/aprs_engine_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    static void sleep_ms(int ms) {
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }
#endif

/* ========================================================================
 * GUI CLIENT — Terminal-based APRS monitor
 *
 * Demonstrates the pattern:
 *   1. Create engine instance
 *   2. Subscribe to events
 *   3. Feed frames and commands
 *   4. Render events to terminal
 *
 * This is intentionally minimal — it shows the client/engine separation
 * without replicating the original ~5000 lines of LCD rendering code.
 * ======================================================================== */

static int g_event_count = 0;
static int g_station_count = 0;

/* Event callback — called when engine publishes events */
static void on_event(const aprs_event_t* ev, void* user) {
    (void)user;
    g_event_count++;

    switch (ev->type) {
        case APRS_EVT_FRAME_RX: {
            const aprs_frame_t* f = (const aprs_frame_t*)ev->payload;
            if (f) {
                printf("  [RX] %s -> %s  [%s]\n",
                       f->src, f->dst, f->is_valid ? "OK" : "INVALID");
            }
            break;
        }
        case APRS_EVT_STATION_UPDATED: {
            const aprs_station_t* st = (const aprs_station_t*)ev->payload;
            if (st) {
                g_station_count++;
                printf("  [STA] %-12s  %.4f/%.4f  %.1fkm @ %.0f°  pkts:%u\n",
                       st->callsign, st->lat, st->lon,
                       st->distance_km, st->bearing_deg, st->packet_count);
            }
            break;
        }
        case APRS_EVT_GPS_UPDATED: {
            const aprs_gps_t* g = (const aprs_gps_t*)ev->payload;
            if (g) {
                printf("  [GPS] Fix:%s  %.6f/%.6f  %.1fkm/h @ %.0f°  alt:%.0fm\n",
                       g->fix ? "YES" : "NO", g->lat, g->lon,
                       g->speed_kmh, g->course_deg, g->altitude_m);
            }
            break;
        }
        case APRS_EVT_MESSAGE_RECEIVED: {
            const aprs_message_t* m = (const aprs_message_t*)ev->payload;
            if (m) {
                printf("  [MSG] From:%s  To:%s  \"%.40s\"  [%s]\n",
                       m->src, m->dst, m->text,
                       m->state == APRS_MSG_NEW ? "NEW" : "READ");
            }
            break;
        }
        case APRS_EVT_BEACON_SENT:
            printf("  [BCN] Beacon sent\n");
            break;
        case APRS_EVT_DIGI_REPEATED:
            printf("  [DIGI] Frame repeated\n");
            break;
        case APRS_EVT_STATS_UPDATED:
            printf("  [STATS] Updated\n");
            break;
        case APRS_EVT_SOUND: {
            const uint8_t* level = (const uint8_t*)ev->payload;
            if (level) printf("  [SND] Sound event, level=%u\n", *level);
            break;
        }
        case APRS_EVT_ERROR:
            printf("  [ERR] %s\n", ev->payload ? (const char*)ev->payload : "unknown");
            break;
        default:
            printf("  [EVT] %s (type=%d)\n", ev->topic, (int)ev->type);
            break;
    }

    fflush(stdout);
}

/* ========================================================================
 * SIMULATED FRAME INJECTION
 * ======================================================================== */

/* Extended APRS frames for testing — covering all major frame types */
static const char* test_frames[] = {
    /* ════════ Position frames (compressed & uncompressed) ════════ */
    /* Stationary position with symbol */
    "SP3XYZ-9>APRS,WIDE1-1,WIDE2-1:!5210.12N/01655.22E-Test station",
    "SP9AAA-1>APRS,RELAY:!5130.50N/01610.30E&Home QTH",
    /* Compressed position with PHG & comment */
    "SQ2BBB-2>APRS,WIDE2-2:=5200.00N/01650.00E#PHG5130 W2, Poznan",
    /* Position with timestamp (hours/minutes) — mobile */
    "SP3CCC-3>APRS,WIDE1-1:/092345h5211.50N/01654.80EvTest/APRS mobile",
    /* Position with timestamp (zulu) — weather */
    "SP5DDD-5>APRS,TCPIP:@092345z5220.00N/01705.00E_Test weather station",

    /* More mobile stations around Poznan */
    "SP3MOB-7>APRS,WIDE1-1,WIDE2-1:/152310h5225.10N/01657.30Er090/050/A=000078 APRS mobile en-route",
    "SQ3CAR-9>APRS,WIDE2-2:/152315h5222.80N/01651.90Ek049/035/A=000095 Car mobile",
    "SP3BIKE>APRS,WIDE1-1:`(Ml s>/]\"4q}Bike tracker|3",
    "HF3PEN-1>APRS,WIDE2-1:=5230.15N/01640.20E-PHG2300Club station QTH",
    "SP3WX-13>APRS,TCPIP:@152330z5228.75N/01702.40E_000/000g003t045r000p000P000h50b10120 WX3in1",
    "SR3DPN>APRS,WIDE2-2:!5224.50N/01655.70E# Digi/repeater Poznan",
    "SP3APR-2>APRS,WIDE1-1:/152340h5223.12N/01648.90Ev/A=000112 iGate RX-only",
    "SQ3TSM-7>APRS,RELAY:/152345h5221.60N/01659.15E[120/000/A=000067 handheld",
    "SP3KAK>APRS,WIDE1-1,WIDE2-1:=5215.30N/01702.80E-Antenna 12m AGL",
    "HF3P-10>APRS,TCPIP:!5224.80N/01655.30E#WX station v2",

    /* ════════ Status frames ════════ */
    "SP3XYZ-9>APRS:>Monitoring 144.800 MHz",
    "SQ3TSM-7>APRS:>QRV 145.550 FM / 430.950 DMR",
    "SR3DPN>APRS:>Digipeater WIDE1-1 WIDE2-2 144.800",
    "SP3MOB-7>APRS:>En route to QTH / 73!",

    /* ════════ Messages (with ACK/reply patterns) ════════ */
    "SP3XYZ-9>APRS::SP3ABC-7 :Hello from engine test{MSG-1001}",
    "SP3ABC-7>APRS::SP3XYZ-9 :ack MSG-1001",
    "SP3MOB-7>APRS::SQ3CAR-9 :What is your ETA?{MSG-1002}",
    "SQ3CAR-9>APRS::SP3MOB-7 :ETA 10 minutes, almost there{MSG-1003}",
    "SP3MOB-7>APRS::SQ3CAR-9 :ack MSG-1003",
    "HF3PEN-1>APRS::SP3XYZ-9 :Club meeting Thursday 19:00{MSG-1004}",
    "SP3XYZ-9>APRS::HF3PEN-1 :ack MSG-1004",
    "SP3KAK>APRS::SP3ABC-7 :Can you relay to net?{MSG-1005}",

    /* ════════ Query frames ════════ */
    "SP3XYZ-9>APRS:?APRS?",
    "SQ3CAR-9>APRS:?APRSP?",
    "SP3MOB-7>APRS:?IGATE?",

    /* ════════ Object frames ════════ */
    "SP3XYZ-9>APRS:;MEETING *152350z5224.00N/01654.00E-Club Meeting Point",
    "SR3DPN>APRS:;REPEATR*152355z5225.50N/01653.90Er145.550MHz -060 R50k",
    "SP3APR-2>APRS:;APRSIG *152400z5223.80N/01655.20EIiGate Fill-in",

    /* ════════ Item frames ════════ */
    "SP3XYZ-9>APRS:)BEACON! 152405z5224.10N/01655.40E30m APRS beacon on tower",

    /* ════════ Mic-E frames ════════ */
    "SP3XYZ-9>S4T2V5,WIDE1-1:`(Fl o>/]\"4j}Test Mic-E",
    "SP3MOB-7>S3U4S6,WIDE1-1,WIDE2-1:`(Gl!r>/]\"5s}En Route|3",
    "SQ3CAR-9>S2Q2P8,WIDE2-1:`(El#p>/]\"4l}In Service|3",

    /* ════════ Telemetry frames ════════ */
    "SP3WX-13>APRS:T#001,120,045,030,128,095,00000000",

    /* ════════ Duplicate frames (test unique filter) ════════ */
    "SP3XYZ-9>APRS,WIDE1-1,WIDE2-1:!5210.12N/01655.22E-Test station",     /* duplicate */
    "SP3MOB-7>APRS,WIDE1-1,WIDE2-1:/152310h5225.10N/01657.30Er090/050/A=000078 APRS mobile en-route", /* duplicate */

    /* ════════ Third-party traffic ════════ */
    "SP3APR-2>APRS:}SP3XYZ-9>APRS,WIDE1-1:!5210.12N/01655.22E-3rd party relay",

    /* ════════ More stations (expanding coverage) ════════ */
    "SP2OFR-1>APRS,WIDE2-2:=5310.50N/01750.30E-QTH Bydgoszcz area",
    "SQ1FYB-9>APRS,WIDE1-1:/152420h5330.20N/01615.40E[180/060/A=000145 Mobile north",
    "SP4WRZ>APRS,TCPIP:@152425z5250.00N/01730.00E_/000g006t032r000p000P000h45b10150 WX Gniezno",
    "SR5LEO>APRS,WIDE2-2:!5220.50N/02100.30E# Digi Warszawa",
    "SP7LUK>APRS,WIDE1-1:=5140.00N/01920.00E-PHG1200 Lodz base",
    "SQ6ELQ-7>APRS,RELAY:/152430h5050.30N/01615.90Ev/A=000234 Mobile SW",
    "SP8EBC-1>APRS,TCPIP:!5030.00N/01755.00E#Opole iGate",
    "SR9NZE>APRS,WIDE2-2:!5010.50N/01950.00E# Digi Krakow",
    "SP3PGH-9>APRS,WIDE1-1:/152435h5218.90N/01638.20Er220/085/A=000112 Mobile W",
    "SQ3RX-2>APRS,WIDE1-1:=5222.00N/01710.50E-WX station v1.5",

    /* ════════ More messages ════════ */
    "SP3APR-2>APRS::SP3ABC-7 :New iGate firmware available v2.1{MSG-1006}",
    "SP3ABC-7>APRS::SP3APR-2 :ack MSG-1006",
    "SQ3TSM-7>APRS::SP3XYZ-9 :QSO on 145.550?{MSG-1007}",
    "SP3XYZ-9>APRS::SQ3TSM-7 :ack MSG-1007",

    /* ════════ Bulletins & announcements ════════ */
    "SP3XYZ-9>APRS::BLN0     :APRS Net every Tuesday 20:00 local",
    "SR3DPN>APRS::BLN1     :New digi in Poznan area on 144.800",
};

#define NUM_TEST_FRAMES (sizeof(test_frames) / sizeof(test_frames[0]))

/* ========================================================================
 * MAIN
 * ======================================================================== */

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("=== APRS Engine v2 — GUI Client Demo ===\n\n");

    /* 1. Create engine */
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
    cfg.use_position_mode = APRS_POSITION_STATIC;
    cfg.report_position_mode = APRS_POSITION_STATIC;

    aprs_engine_t* engine = NULL;
    aprs_status_t status = aprs_engine_create(&cfg, &engine);
    if (status != APRS_OK) {
        fprintf(stderr, "Failed to create engine: %d\n", (int)status);
        return 1;
    }

    printf("Engine created. Version: %s\n", aprs_engine_version());

    /* 2. Configure */
    aprs_engine_set_static_position(engine, 0, 52.4064, 16.9252, 92.0);
    aprs_engine_set_use_position_mode(engine, APRS_POSITION_STATIC);
    aprs_engine_set_beacon_delay(engine, 600);
    printf("Position set to Poznan (52.4064N, 16.9252E)\n");

    /* 3. Subscribe to events */
    printf("Subscribing to all events...\n");
    uint64_t sub_id = aprs_engine_subscribe(engine, "*", on_event, NULL);
    if (!sub_id) {
        fprintf(stderr, "Subscription failed\n");
        aprs_engine_destroy(engine);
        return 1;
    }
    printf("Subscription OK (id=%llu)\n\n", (unsigned long long)sub_id);

    /* 4. Start engine */
    aprs_engine_start(engine);
    printf("Engine started. Feeding test frames...\n");
    printf("---\n");

    /* 5. Feed test frames with delays */
    for (int i = 0; i < (int)NUM_TEST_FRAMES; i++) {
        printf("\n[FEED %d] %s\n", i + 1, test_frames[i]);

        /* Inject NMEA GPS data with the first frame */
        if (i == 0) {
            aprs_engine_push_nmea(engine,
                "$GPGGA,092345,5210.12,N,01655.22,E,1,08,1.0,92.0,M,42.0,M,,*XX\r\n");
        }

        aprs_engine_feed_frame(engine, test_frames[i], strlen(test_frames[i]));

        /* Process engine ticks */
        for (int t = 0; t < 10; t++) {
            aprs_engine_tick(engine, 100); /* 100ms per tick */
        }

        sleep_ms(200);
    }

    /* 6. Show engine state */
    printf("\n---\n");
    printf("Test complete. %d events received, %d stations updated.\n\n",
           g_event_count, g_station_count);

    /* Get snapshot */
    char snapshot[4096];
    size_t snap_len = sizeof(snapshot);
    if (aprs_engine_get_snapshot(engine, snapshot, &snap_len) == APRS_OK) {
        printf("=== Engine Snapshot ===\n%s\n\n", snapshot);
    }

    /* Get stats */
    aprs_stats_t stats;
    if (aprs_engine_get_stats(engine, &stats) == APRS_OK) {
        printf("=== Statistics ===\n");
        printf("RX frames:      %u\n", stats.rx_frames);
        printf("TX frames:      %u\n", stats.tx_frames);
        printf("Digi repeats:   %u\n", stats.digi_repeats);
        printf("Messages RX:    %u\n", stats.msg_received);
        printf("Messages TX:    %u\n", stats.msg_sent);
        printf("Invalid frames: %u\n", stats.invalid_frames);
        printf("Unique frames:  %u\n", stats.unique_frames);
    }

    /* List stations */
    aprs_station_t stations[30];
    size_t st_count = 30;
    if (aprs_engine_list_stations(engine, stations, &st_count) == APRS_OK) {
        printf("\n=== Station List (%zu stations) ===\n", st_count);
        printf("%-15s %12s %12s %8s %8s %6s\n",
               "Callsign", "Lat", "Lon", "Dist(km)", "Bearing", "Pkts");
        printf("----------------------------------------------------------------\n");
        for (size_t i = 0; i < st_count; i++) {
            printf("%-15s %11.4f %11.4f %8.1f %8.0f %6u\n",
                   stations[i].callsign,
                   stations[i].lat, stations[i].lon,
                   stations[i].distance_km, stations[i].bearing_deg,
                   stations[i].packet_count);
        }
    }

    /* 7. Cleanup */
    aprs_engine_unsubscribe(engine, sub_id);
    aprs_engine_stop(engine);
    aprs_engine_destroy(engine);

    printf("\nEngine destroyed. Done.\n");
    return 0;
}
