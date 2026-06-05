#ifndef APRS_HTTP_SERVER_H
#define APRS_HTTP_SERVER_H

#include <stdint.h>
#include <stddef.h>
#include "../../engine/include/aprs_engine_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aprs_http_server aprs_http_server_t;

/* Create HTTP server bound to given port, using the specified engine.
 * Returns NULL on failure. */
aprs_http_server_t* aprs_http_server_create(int port, aprs_engine_t* engine);

/* Set optional authentication token (Bearer token). Empty string disables auth. */
void aprs_http_server_set_auth_token(aprs_http_server_t* srv, const char* token);

/* Start the server (blocking). Returns 0 on clean shutdown, -1 on error. */
int aprs_http_server_run(aprs_http_server_t* srv);

/* Stop the server (call from signal handler or another thread). */
void aprs_http_server_stop(aprs_http_server_t* srv);

/* Destroy the server and free resources. */
void aprs_http_server_destroy(aprs_http_server_t* srv);

#ifdef __cplusplus
}
#endif

#endif /* APRS_HTTP_SERVER_H */
