#include "../../engine/include/aprs_engine_v2.h"
#include <stdio.h>
#include <string.h>

/* ========================================================================
 * RENDERER — Platform-neutral rendering abstraction
 *
 * This is a stub. In a real client, this would:
 *   - Draw station list (sorted by distance/callsign)
 *   - Draw radar view (polar plot of stations)
 *   - Draw station detail (info, path, frames)
 *   - Draw message list
 *   - Draw GPS compass
 *   - Draw statistics
 *   - Draw menus and configuration dialogs
 *
 * The original device had ~5000 lines of LCD drawing code for this.
 * This module defines the interface so any rendering backend
 * (SDL2, OpenGL, ncurses, HTML canvas) can be plugged in.
 * ======================================================================== */

void renderer_init(void) {
    printf("[RENDERER] Initialized\n");
}

void renderer_shutdown(void) {
    printf("[RENDERER] Shutdown\n");
}

void renderer_draw_station_list(const aprs_station_t* stations, size_t count) {
    printf("\n=== STATION LIST ===\n");
    for (size_t i = 0; i < count; i++) {
        printf("  %-12s  %.1fkm @ %.0f°  %u pkts  %s\n",
               stations[i].callsign,
               stations[i].distance_km,
               stations[i].bearing_deg,
               stations[i].packet_count,
               stations[i].marked ? "[MARKED]" : "");
    }
}

void renderer_draw_radar(const aprs_station_t* stations, size_t count,
                          double center_lat, double center_lon, double range_km) {
    printf("\n=== RADAR (center %.4f/%.4f, range %.1f km) ===\n",
           center_lat, center_lon, range_km);
    for (size_t i = 0; i < count; i++) {
        if (stations[i].has_position && stations[i].distance_km <= range_km) {
            printf("  %-12s  %.1fkm @ %.0f°\n",
                   stations[i].callsign,
                   stations[i].distance_km,
                   stations[i].bearing_deg);
        }
    }
}

void renderer_draw_gps_compass(const aprs_gps_t* gps) {
    printf("\n=== GPS COMPASS ===\n");
    printf("  Fix: %s  Valid: %s\n", gps->fix ? "YES" : "NO", gps->valid ? "YES" : "NO");
    printf("  Position: %.6f, %.6f\n", gps->lat, gps->lon);
    printf("  Speed: %.1f km/h  Course: %.0f°  Alt: %.0f m\n",
           gps->speed_kmh, gps->course_deg, gps->altitude_m);
}

void renderer_draw_message_list(const aprs_message_t* messages, size_t count) {
    printf("\n=== MESSAGES ===\n");
    for (size_t i = 0; i < count; i++) {
        const char* state_str = "?";
        switch (messages[i].state) {
            case APRS_MSG_NEW:      state_str = "NEW"; break;
            case APRS_MSG_SENT:     state_str = "SENT"; break;
            case APRS_MSG_ACKED:    state_str = "ACKED"; break;
            case APRS_MSG_REJECTED: state_str = "REJECTED"; break;
            case APRS_MSG_READ:     state_str = "READ"; break;
        }
        printf("  [%s] %s -> %s: %.50s\n",
               state_str, messages[i].src, messages[i].dst, messages[i].text);
    }
}

void renderer_draw_stats(const aprs_stats_t* stats) {
    printf("\n=== STATISTICS ===\n");
    printf("  RX: %u  TX: %u  Digi: %u\n",
           stats->rx_frames, stats->tx_frames, stats->digi_repeats);
    printf("  Messages RX: %u  TX: %u  Invalid: %u  Unique: %u\n",
           stats->msg_received, stats->msg_sent,
           stats->invalid_frames, stats->unique_frames);
    printf("  TX:%s  Digi:%s  Beacon:%s  Uptime: %.1f h\n",
           stats->tx_enabled ? "ON" : "OFF",
           stats->digi_enabled ? "ON" : "OFF",
           stats->beacon_enabled ? "ON" : "OFF",
           stats->uptime_ms / 3600000.0);
}
