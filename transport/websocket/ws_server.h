#ifndef APRS_WS_SERVER_H
#define APRS_WS_SERVER_H

#include <stdint.h>
#include <stddef.h>
#include "../../engine/include/aprs_engine_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aprs_ws_server aprs_ws_server_t;

/* Create WebSocket server on given port, using the specified engine. */
aprs_ws_server_t* aprs_ws_server_create(int port, aprs_engine_t* engine);

/* Set optional authentication token (Bearer token). Empty string disables auth. */
void aprs_ws_server_set_auth_token(aprs_ws_server_t* srv, const char* token);

/* Start the server (blocking). */
int aprs_ws_server_run(aprs_ws_server_t* srv);

/* Stop the server. */
void aprs_ws_server_stop(aprs_ws_server_t* srv);

/* Destroy the server. */
void aprs_ws_server_destroy(aprs_ws_server_t* srv);

#ifdef __cplusplus
}
#endif

#endif /* APRS_WS_SERVER_H */
