# APRS Engine Architecture and API

## Purpose
This document defines the target architecture for extracting the APRS Navigator logic into a reusable engine with no GUI or keyboard dependencies. The engine exposes a stable C API for control, query, and event delivery, and it can serve multiple simultaneous clients through a publish/subscribe event bus.

The design is based on the existing APRS Navigator capabilities: station tracking, message handling, GPS integration, beaconing, digipeater logic, TX lock, remote control, filters, and statistics.

## Architectural goals

- Separate domain logic from presentation.
- Make the engine usable as a library or daemon.
- Support multiple concurrent clients.
- Deliver state changes as events.
- Keep transport concerns outside the core logic.
- Preserve existing APRS features.

## Recommended split

### 1. Core engine
Owns all APRS state and rules.

Responsibilities:

- Frame parsing and validation.
- Station database maintenance.
- Message inbox/outbox.
- Beacon scheduling and composition.
- Digipeater decisions.
- GPS state handling.
- Statistics and counters.
- Remote command processing.
- Configuration persistence.

### 2. Event bus
An internal publish/subscribe mechanism.

Responsibilities:

- Fan-out events to multiple subscribers.
- Filter by topic.
- Queue events for slow clients.
- Preserve ordering per stream if required.

### 3. Transport adapters
Expose the engine outside the process.

Recommended transports:

- HTTP/REST for commands and snapshots.
- WebSocket for event streaming.
- Optional gRPC streaming later if strong typing is desired.

### 4. UI clients
Any GUI, terminal, or web frontend becomes a separate client and does not access internal engine data directly.

## Implementation plan

1. Extract pure logic from UI code.
2. Define domain objects and state ownership.
3. Replace direct screen refreshes with events.
4. Add an internal event bus.
5. Expose a C API around the core.
6. Add HTTP endpoints for commands and snapshots.
7. Add WebSocket subscription for live events.
8. Convert the old UI into a client of the new API.
9. Add tests for frame processing, digi, beaconing, and message flow.
10. Stabilize the API and document versioning rules.

## C API overview

The C API is split into lifecycle, configuration, command, query, and events.

### Lifecycle

- `aprs_engine_create()`
- `aprs_engine_start()`
- `aprs_engine_stop()`
- `aprs_engine_destroy()`

### Queries

- `aprs_engine_get_snapshot()`
- `aprs_engine_get_station()`
- `aprs_engine_get_message()`
- `aprs_engine_get_stats()`
- `aprs_engine_get_gps()`

### Commands

- `aprs_engine_set_callsign()`
- `aprs_engine_set_position()`
- `aprs_engine_set_position_mode()`
- `aprs_engine_send_beacon()`
- `aprs_engine_send_message()`
- `aprs_engine_lock_tx()`
- `aprs_engine_set_filter()`
- `aprs_engine_factory_reset()`

### Subscriptions

- `aprs_engine_subscribe()`
- `aprs_engine_unsubscribe()`

## Public header

```c
#ifndef APRS_ENGINE_H
#define APRS_ENGINE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aprs_engine aprs_engine_t;

typedef enum {
    APRS_OK = 0,
    APRS_ERR_INVALID_ARG,
    APRS_ERR_NOT_FOUND,
    APRS_ERR_BUSY,
    APRS_ERR_DENIED,
    APRS_ERR_NO_MEMORY,
    APRS_ERR_TIMEOUT,
    APRS_ERR_INTERNAL
} aprs_status_t;

typedef enum {
    APRS_POSITION_AUTO = 0,
    APRS_POSITION_STATIC,
    APRS_POSITION_GPS
} aprs_position_mode_t;

typedef enum {
    APRS_MSG_NEW = 0,
    APRS_MSG_SENT,
    APRS_MSG_ACKED,
    APRS_MSG_REJECTED,
    APRS_MSG_READ
} aprs_message_state_t;

typedef enum {
    APRS_EVT_FRAME_RX = 0,
    APRS_EVT_FRAME_TX,
    APRS_EVT_STATION_UPDATED,
    APRS_EVT_MESSAGE_RECEIVED,
    APRS_EVT_MESSAGE_SENT,
    APRS_EVT_GPS_UPDATED,
    APRS_EVT_POSITION_CHANGED,
    APRS_EVT_BEACON_SENT,
    APRS_EVT_DIGI_REPEATED,
    APRS_EVT_TX_STATE_CHANGED,
    APRS_EVT_CONFIG_CHANGED,
    APRS_EVT_STATS_UPDATED,
    APRS_EVT_ERROR
} aprs_event_type_t;

typedef struct {
    char callsign;[1]
    char ssid;[2]
    char symbol_table;
    char symbol_code;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
    uint8_t unique_frames_only;
    uint16_t gps_baudrate;
    aprs_position_mode_t use_position_mode;
    aprs_position_mode_t report_position_mode;
} aprs_engine_config_t;

typedef struct {
    uint64_t uptime_ms;
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t digi_repeats;
    uint32_t msg_received;
    uint32_t msg_sent;
    uint32_t invalid_frames;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
} aprs_stats_t;

typedef struct {
    char callsign;[1]
    char last_path;
    char symbol_table;
    char symbol_code;
    double lat;
    double lon;
    double distance_km;
    double bearing_deg;
    uint32_t packet_count;
    uint64_t last_heard_ms;
    char info;
    char comment;
    uint8_t has_position;
    uint8_t has_weather;
} aprs_station_t;

typedef struct {
    char id;[1]
    char src;[1]
    char dst;[1]
    char text;
    aprs_message_state_t state;
    uint64_t ts_ms;
} aprs_message_t;

typedef struct {
    char raw;
    size_t raw_len;
    char src;[1]
    char dst;[1]
    char path;
    uint8_t is_valid;
} aprs_frame_t;

typedef struct {
    uint8_t fix;
    uint8_t valid;
    double lat;
    double lon;
    double speed_kmh;
    double course_deg;
    double altitude_m;
    uint64_t ts_ms;
} aprs_gps_t;

typedef struct {
    aprs_event_type_t type;
    uint64_t ts_ms;
    const char* topic;
    const void* payload;
    size_t payload_size;
} aprs_event_t;

typedef void (*aprs_event_cb)(const aprs_event_t* ev, void* user);
typedef uint64_t aprs_subscription_id_t;

aprs_status_t aprs_engine_create(const aprs_engine_config_t* cfg, aprs_engine_t** out);
aprs_status_t aprs_engine_start(aprs_engine_t* e);
aprs_status_t aprs_engine_stop(aprs_engine_t* e);
void aprs_engine_destroy(aprs_engine_t* e);

aprs_status_t aprs_engine_get_snapshot(aprs_engine_t* e, void* out_json, size_t out_len);
aprs_status_t aprs_engine_get_stats(aprs_engine_t* e, aprs_stats_t* out);
aprs_status_t aprs_engine_get_station(aprs_engine_t* e, const char* callsign, aprs_station_t* out);
aprs_status_t aprs_engine_get_message(aprs_engine_t* e, const char* id, aprs_message_t* out);
aprs_status_t aprs_engine_get_gps(aprs_engine_t* e, aprs_gps_t* out);

aprs_status_t aprs_engine_set_callsign(aprs_engine_t* e, const char* callsign);
aprs_status_t aprs_engine_set_position(aprs_engine_t* e, double lat, double lon, double alt);
aprs_status_t aprs_engine_set_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_send_beacon(aprs_engine_t* e);
aprs_status_t aprs_engine_send_message(aprs_engine_t* e, const char* dest, const char* text);
aprs_status_t aprs_engine_lock_tx(aprs_engine_t* e, uint8_t disable_tx, uint8_t disable_digi, uint8_t disable_query);
aprs_status_t aprs_engine_set_filter(aprs_engine_t* e, const char* filter_expr);
aprs_status_t aprs_engine_factory_reset(aprs_engine_t* e);

aprs_subscription_id_t aprs_engine_subscribe(aprs_engine_t* e, const char* topic_filter, aprs_event_cb cb, void* user);
aprs_status_t aprs_engine_unsubscribe(aprs_engine_t* e, aprs_subscription_id_t id);

#ifdef __cplusplus
}
#endif

#endif
```

## aprs_engine_ws_protocol.md

```md
# APRS Engine WebSocket JSON Protocol

## Purpose
This document defines the JSON protocol for real-time communication between clients and the APRS engine over WebSocket. The protocol is designed for multiple simultaneous clients, topic-based subscriptions, command execution, acknowledgements, and server-side push events.

## Connection model

- One WebSocket connection represents one client session.
- A client may subscribe to zero or more topics.
- The server may broadcast the same event to many clients.
- The client can send commands over the same connection.
- The server should send an initial snapshot immediately after authentication or subscription, depending on the chosen mode.

## Message envelope

All protocol messages use a common envelope.

```json
{
  "type": "command|event|snapshot|ack|error|hello|subscribe|unsubscribe|ping|pong",
  "id": "optional-message-id",
  "tsMs": 1717050000123,
  "topic": "optional.topic.name",
  "payload": {}
}
```

### Fields

- `type`: Message category.
- `id`: Correlation ID for commands and acknowledgements.
- `tsMs`: Server timestamp in milliseconds.
- `topic`: Event topic or subscription topic.
- `payload`: Type-specific content.

## Session flow

1. Client connects.
2. Server sends `hello`.
3. Client optionally authenticates.
4. Client subscribes to topics.
5. Server sends snapshot.
6. Server pushes events.
7. Client sends commands when needed.
8. Server replies with `ack` or `error`.

## Handshake

### Server hello

```json
{
  "type": "hello",
  "tsMs": 1717050000000,
  "payload": {
    "engineVersion": "1.0",
    "protocolVersion": "1.0",
    "serverName": "aprs-engine",
    "capabilities": ["snapshot", "events", "commands", "filters"]
  }
}
```

### Client subscribe

```json
{
  "type": "subscribe",
  "id": "c-1001",
  "payload": {
    "topics": ["station.*", "message.*", "gps.*", "stats.*"],
    "filter": "callsign=SP3ABC-*"
  }
}
```

### Server ack

```json
{
  "type": "ack",
  "id": "c-1001",
  "tsMs": 1717050000100,
  "payload": {
    "status": "ok"
  }
}
```

## Commands

Commands are sent by the client and should be processed by the engine in a serialized command queue.

### Send beacon

```json
{
  "type": "command",
  "id": "cmd-2001",
  "topic": "beacon.send",
  "payload": {
    "pathIndex": 0,
    "statusIndex": 1
  }
}
```

### Set position

```json
{
  "type": "command",
  "id": "cmd-2002",
  "topic": "position.set",
  "payload": {
    "mode": "static",
    "lat": 52.4064,
    "lon": 16.9252,
    "alt": 92.0
  }
}
```

### Lock TX

```json
{
  "type": "command",
  "id": "cmd-2003",
  "topic": "tx.lock",
  "payload": {
    "disableTx": true,
    "disableDigi": false,
    "disableQuery": true
  }
}
```

### Send message

```json
{
  "type": "command",
  "id": "cmd-2004",
  "topic": "message.send",
  "payload": {
    "dest": "SP9AAA-1",
    "text": "Hello from engine"
  }
}
```

## Events

Events are server-pushed notifications and are read-only for clients.

### Frame received

```json
{
  "type": "event",
  "tsMs": 1717050000123,
  "topic": "frame.rx",
  "payload": {
    "src": "SP3XYZ-9",
    "dst": "APRS",
    "path": "WIDE1-1,WIDE2-1",
    "raw": "SP3XYZ-9>APRS,WIDE1-1,WIDE2-1:!5210.12N/01655.22E-Test"
  }
}
```

### Station updated

```json
{
  "type": "event",
  "tsMs": 1717050000200,
  "topic": "station.updated",
  "payload": {
    "callsign": "SP3XYZ-9",
    "symbolTable": "/",
    "symbolCode": ">",
    "lat": 52.102,
    "lon": 16.921,
    "distanceKm": 12.4,
    "bearingDeg": 78.0,
    "packetCount": 31,
    "lastHeardMs": 1717050000200,
    "hasPosition": true,
    "hasWeather": false
  }
}
```

### Message received

```json
{
  "type": "event",
  "tsMs": 1717050000456,
  "topic": "message.received",
  "payload": {
    "id": "m-1024",
    "src": "SP9AAA-1",
    "dst": "SP3ABC-7",
    "text": "Hello",
    "state": "new"
  }
}
```

### GPS updated

```json
{
  "type": "event",
  "tsMs": 1717050000500,
  "topic": "gps.updated",
  "payload": {
    "fix": true,
    "valid": true,
    "lat": 52.4064,
    "lon": 16.9252,
    "speedKmh": 48.2,
    "courseDeg": 127.0,
    "altitudeM": 92.0
  }
}
```

### Beacon sent

```json
{
  "type": "event",
  "tsMs": 1717050000600,
  "topic": "beacon.sent",
  "payload": {
    "callsign": "SP3ABC-7",
    "path": "WIDE1-1,WIDE2-1",
    "statusIndex": 1
  }
}
```

### Digi repeated

```json
{
  "type": "event",
  "tsMs": 1717050000650,
  "topic": "digi.repeated",
  "payload": {
    "src": "SP3XYZ-9",
    "alias": "WIDE1-1",
    "duplicateSuppressed": false
  }
}
```

### TX state changed

```json
{
  "type": "event",
  "tsMs": 1717050000700,
  "topic": "tx.state",
  "payload": {
    "txEnabled": false,
    "digiEnabled": true,
    "beaconEnabled": true,
    "reason": "manual_lock"
  }
}
```

### Stats updated

```json
{
  "type": "event",
  "tsMs": 1717050000800,
  "topic": "stats.updated",
  "payload": {
    "rxFrames": 1842,
    "txFrames": 112,
    "digiRepeats": 29,
    "msgReceived": 18,
    "msgSent": 9,
    "invalidFrames": 4
  }
}
```

## Snapshot

The server may send a snapshot after subscription or on explicit request.

```json
{
  "type": "snapshot",
  "tsMs": 1717050000900,
  "payload": {
    "engine": {
      "version": "1.0",
      "callsign": "SP3ABC-7",
      "txEnabled": true,
      "digiEnabled": true,
      "beaconEnabled": true,
      "uptimeMs": 8642000
    },
    "gps": {
      "fix": true,
      "lat": 52.4064,
      "lon": 16.9252,
      "speedKmh": 48.2,
      "courseDeg": 127.0
    },
    "stats": {
      "rxFrames": 1842,
      "txFrames": 112,
      "digiRepeats": 29,
      "msgReceived": 18,
      "msgSent": 9
    }
  }
}
```

## Error format

```json
{
  "type": "error",
  "id": "cmd-2004",
  "tsMs": 1717050000950,
  "payload": {
    "code": "invalid_argument",
    "message": "Destination callsign is empty"
  }
}
```

### Recommended error codes

- `invalid_argument`
- `unauthorized`
- `forbidden`
- `not_found`
- `busy`
- `timeout`
- `unsupported`
- `internal_error`

## Filters

The server may allow subscription filters to reduce traffic.

Examples:

- `callsign=SP3ABC-*`
- `topic=message.*`
- `station=SP3XYZ-9`
- `rx_only=true`
- `include_raw=false`

Filter semantics should be additive and conservative: if a filter cannot be parsed, the server should reject it rather than guess.

## Ordering and delivery

- Events should be delivered in the order they are produced per connection.
- Commands should be acknowledged with the same `id`.
- A client should treat `ack` as command acceptance, not necessarily radio success.
- Final command completion may be reported via a follow-up event.

## Client examples

### Monitor-only client

- Subscribe to `frame.rx`, `station.*`, `stats.*`.
- Do not send commands.
- Render UI based on events only.

### Operator client

- Subscribe to all topics.
- Send beacon, message, and TX lock commands.
- Keep local command history using correlation IDs.

### Diagnostic client

- Subscribe to `frame.*`, `digi.*`, `error`, `stats.*`.
- Request snapshots periodically.

## Notes for implementation

- Use UTF-8 strings everywhere.
- Avoid binary blobs in JSON unless absolutely necessary.
- Keep message sizes bounded.
- Add protocol versioning at the hello stage.
- Support replay or snapshot resend after reconnect.
- Do not expose internal pointers or mutable engine state over the wire.
```

## aprs_engine_spec_v2.md

```md
# APRS Navigator Engine v2 - Developer Specification

## 1. Scope

This document defines a complete implementation plan for extracting the APRS Navigator logic from the original device and turning it into a reusable engine. The goal is functional parity with the user manual, including station tracking, position handling, beaconing, digi, messages, GPS, filters, stats, remote control, raw frame editing, and diagnostics.

The engine must be transport-agnostic and free of GUI dependencies. UI, terminal, and web clients are separate consumers of the engine state and event stream.

## 2. Functional requirements from the manual

The implementation must cover:

- Station list with sorting, marking, and detail view.
- Radar view data model.
- Message inbox, outbox, send, retry, ACK/REJ handling.
- GPS input and navigation state.
- User position and report position with AUTO, STATIC, GPS.
- Three stored positions with manual entry and capture.
- Beacon composition, rotation of paths and status texts.
- Digipeater with four aliases, timing, and monitoring.
- Station announce notifications.
- Frame filtering: unique only, TCP/IP, invalid frames.
- Raw frame viewing and editing.
- Remote control via one-shot keys.
- TRX tuning parameters.
- Statistics: hourly and 20-hour history.
- Sound events as abstract notifications.
- TX lock modes.

## 3. Recommended architecture

### 3.1 Core engine

Owns all mutable APRS state and all domain rules.

### 3.2 Event bus

Internal pub/sub system for fan-out to many subscribers.

### 3.3 Transport adapters

- HTTP for commands, snapshots, and admin operations.
- WebSocket for live events.
- Optional gRPC streaming later.

### 3.4 Client adapters

- GUI client.
- Console client.
- Web client.
- Diagnostics client.

## 4. Runtime model

The engine should process all state mutations in a single serialized command loop. Multiple clients may connect concurrently, but writes must be serialized through one command queue to preserve deterministic behavior.

Read requests may be served from immutable snapshots or lock-protected copies.

## 5. State domains

### 5.1 Global engine state

- Callsign and SSID.
- TX/digi/beacon lock state.
- Active filters.
- Default symbol.
- Version and runtime flags.

### 5.2 Station database

Stores all heard stations and per-station history.

### 5.3 Messages

Stores received and sent messages, retries, and state transitions.

### 5.4 GPS state

Stores latest valid GPS data and last known fix.

### 5.5 Position manager state

Stores user positions, report positions, and memory slots.

### 5.6 Digi state

Stores alias definitions, duplication prevention, recent repeat list, and DIGITX flag.

### 5.7 Statistics state

Stores counters and time buckets for analysis.

### 5.8 Raw frame buffer

Stores one editable frame buffer for copy/edit/replay workflows.

## 6. Implementation modules

### 6.1 Frame ingest

Responsibilities:

- Decode APRS frames.
- Validate headers and payload.
- Classify frame types.
- Apply filters.
- Emit raw and parsed events.

### 6.2 Station manager

Responsibilities:

- Add/update stations.
- Track last heard time.
- Maintain packet counts.
- Store up to five raw frames per station if desired.
- Sort and mark stations.

### 6.3 Position manager

Responsibilities:

- Maintain `USE` and `REPORT` positions.
- Support `AUTO`, `STATIC`, `GPS` modes.
- Store three fixed locations.
- Accept manual coordinate entry.
- Capture position from selected station.
- Recalculate distance/bearing on changes.

### 6.4 Beacon manager

Responsibilities:

- Compose beacon frames.
- Rotate status texts.
- Rotate paths.
- Insert NAV tag if enabled.
- Honor TX lock and beacon enable state.
- Schedule periodic sends.

### 6.5 Message manager

Responsibilities:

- Receive messages.
- Filter by own callsign or all messages.
- Queue outgoing messages.
- Repeat until ACK/REJ or retry limit.
- Track sent and received history.

### 6.6 Digi engine

Responsibilities:

- Match aliases.
- Apply simple/flood/trace behavior.
- Prevent duplicates.
- Honor hop limits.
- Maintain timing windows.
- Track recent digi list.

### 6.7 Stats engine

Responsibilities:

- Count received, transmitted, invalid, unique, digi frames.
- Maintain 1-hour minute buckets.
- Maintain 20-hour 10-minute buckets.
- Expose snapshots and histories.

### 6.8 Raw frame editor

Responsibilities:

- Copy any received frame into the editable buffer.
- Allow direct modification.
- Allow hex editing where needed.
- Replay edited frame.
- Create a frame from scratch.

### 6.9 Remote control

Responsibilities:

- Generate one-shot authentication keys.
- Validate APRS remote command messages.
- Execute supported remote actions.

### 6.10 TRX tuning

Responsibilities:

- Store and expose tuning parameters.
- Provide safe defaults.
- Allow advanced diagnostics mode.

### 6.11 Notification manager

Responsibilities:

- Map events to sound IDs.
- Emit abstract notifications for clients.

## 7. Public C API

### 7.1 Lifecycle

```c
typedef struct aprs_engine aprs_engine_t;

typedef enum {
    APRS_OK = 0,
    APRS_ERR_INVALID_ARG,
    APRS_ERR_NOT_FOUND,
    APRS_ERR_BUSY,
    APRS_ERR_DENIED,
    APRS_ERR_NO_MEMORY,
    APRS_ERR_TIMEOUT,
    APRS_ERR_INTERNAL
} aprs_status_t;

aprs_status_t aprs_engine_create(const aprs_engine_config_t* cfg, aprs_engine_t** out);
aprs_status_t aprs_engine_start(aprs_engine_t* e);
aprs_status_t aprs_engine_stop(aprs_engine_t* e);
void aprs_engine_destroy(aprs_engine_t* e);
```

### 7.2 Configuration

```c
aprs_status_t aprs_engine_set_config(aprs_engine_t* e, const aprs_engine_config_t* cfg);
aprs_status_t aprs_engine_get_config(aprs_engine_t* e, aprs_engine_config_t* out);
aprs_status_t aprs_engine_save_config(aprs_engine_t* e);
```

### 7.3 Station API

```c
aprs_status_t aprs_engine_list_stations(aprs_engine_t* e, aprs_station_t* out, size_t* inout_count);
aprs_status_t aprs_engine_get_station(aprs_engine_t* e, const char* callsign, aprs_station_t* out);
aprs_status_t aprs_engine_mark_station(aprs_engine_t* e, const char* callsign, uint8_t mark);
aprs_status_t aprs_engine_sort_stations(aprs_engine_t* e, const char* column, uint8_t ascending);
```

### 7.4 Position API

```c
typedef enum {
    APRS_POSITION_AUTO = 0,
    APRS_POSITION_STATIC,
    APRS_POSITION_GPS
} aprs_position_mode_t;

aprs_status_t aprs_engine_set_use_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_report_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt);
aprs_status_t aprs_engine_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt);
aprs_status_t aprs_engine_select_position_slot(aprs_engine_t* e, int slot);
aprs_status_t aprs_engine_capture_station_position(aprs_engine_t* e, const char* callsign);
aprs_status_t aprs_engine_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt);
```

### 7.5 Beacon API

```c
aprs_status_t aprs_engine_set_status_text(aprs_engine_t* e, int slot, const char* text);
aprs_status_t aprs_engine_set_path(aprs_engine_t* e, int slot, const char* path);
aprs_status_t aprs_engine_select_status_text(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_select_path(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_set_beacon_delay(aprs_engine_t* e, int delay_sec);
aprs_status_t aprs_engine_send_beacon(aprs_engine_t* e);
aprs_status_t aprs_engine_preview_last_tx_frame(aprs_engine_t* e, aprs_frame_t* out);
```

### 7.6 Message API

```c
aprs_status_t aprs_engine_send_message(aprs_engine_t* e, const char* dest, const char* text);
aprs_status_t aprs_engine_list_messages(aprs_engine_t* e, aprs_message_t* out, size_t* inout_count);
aprs_status_t aprs_engine_set_message_receive_mode(aprs_engine_t* e, uint8_t receive_all);
aprs_status_t aprs_engine_set_message_retry(aprs_engine_t* e, int retries, int delay_sec);
aprs_status_t aprs_engine_mark_message_read(aprs_engine_t* e, const char* id);
```

### 7.7 Frame editor API

```c
aprs_status_t aprs_engine_copy_frame_to_editor(aprs_engine_t* e, const char* frame_id);
aprs_status_t aprs_engine_get_editor_frame(aprs_engine_t* e, aprs_frame_t* out);
aprs_status_t aprs_engine_set_editor_frame_text(aprs_engine_t* e, const char* raw);
aprs_status_t aprs_engine_edit_editor_frame(aprs_engine_t* e, size_t offset, uint8_t value);
aprs_status_t aprs_engine_send_editor_frame(aprs_engine_t* e);
aprs_status_t aprs_engine_clear_editor_frame(aprs_engine_t* e);
```

### 7.8 Digi API

```c
typedef enum {
    APRS_DIGI_SIMPLE = 0,
    APRS_DIGI_FLOOD,
    APRS_DIGI_TRACE
} aprs_digi_type_t;

aprs_status_t aprs_engine_set_digi_enabled(aprs_engine_t* e, uint8_t enabled);
aprs_status_t aprs_engine_set_digi_alias(aprs_engine_t* e, int slot, const char* alias, aprs_digi_type_t type, int max_hops);
aprs_status_t aprs_engine_set_digi_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec);
aprs_status_t aprs_engine_get_recent_digi(aprs_engine_t* e, char* out, size_t out_len);
aprs_status_t aprs_engine_set_digi_tx_monitor(aprs_engine_t* e, uint8_t enabled);
```

### 7.9 Filter API

```c
aprs_status_t aprs_engine_set_frame_filter(aprs_engine_t* e, uint8_t unique_only, uint8_t tcpip, uint8_t invalid_ok);
aprs_status_t aprs_engine_set_filter_expression(aprs_engine_t* e, const char* expr);
```

### 7.10 GPS API

```c
aprs_status_t aprs_engine_push_nmea(aprs_engine_t* e, const char* sentence);
aprs_status_t aprs_engine_get_gps(aprs_engine_t* e, aprs_gps_t* out);
```

### 7.11 Stats API

```c
aprs_status_t aprs_engine_get_stats(aprs_engine_t* e, aprs_stats_t* out);
aprs_status_t aprs_engine_get_hourly_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);
aprs_status_t aprs_engine_get_20h_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);
```

### 7.12 Remote API

```c
aprs_status_t aprs_engine_generate_remote_keys(aprs_engine_t* e, char keys[4][16]);
aprs_status_t aprs_engine_execute_remote_command(aprs_engine_t* e, const char* src, const char* msg);
```

### 7.13 Subscriptions

```c
typedef enum {
    APRS_EVT_FRAME_RX = 0,
    APRS_EVT_FRAME_TX,
    APRS_EVT_STATION_UPDATED,
    APRS_EVT_MESSAGE_RECEIVED,
    APRS_EVT_MESSAGE_SENT,
    APRS_EVT_GPS_UPDATED,
    APRS_EVT_POSITION_CHANGED,
    APRS_EVT_BEACON_SENT,
    APRS_EVT_DIGI_REPEATED,
    APRS_EVT_TX_STATE_CHANGED,
    APRS_EVT_CONFIG_CHANGED,
    APRS_EVT_STATS_UPDATED,
    APRS_EVT_ERROR,
    APRS_EVT_SOUND
} aprs_event_type_t;

typedef struct {
    aprs_event_type_t type;
    uint64_t ts_ms;
    const char* topic;
    const void* payload;
    size_t payload_size;
} aprs_event_t;

typedef void (*aprs_event_cb)(const aprs_event_t* ev, void* user);
typedef uint64_t aprs_subscription_id_t;

aprs_subscription_id_t aprs_engine_subscribe(aprs_engine_t* e, const char* topic_filter, aprs_event_cb cb, void* user);
aprs_status_t aprs_engine_unsubscribe(aprs_engine_t* e, aprs_subscription_id_t id);
```

## 8. Data structures

### 8.1 Station

```c
typedef struct {
    char callsign[16];
    char last_path[64];
    char symbol_table;
    char symbol_code;
    double lat;
    double lon;
    double distance_km;
    double bearing_deg;
    uint32_t packet_count;
    uint64_t last_heard_ms;
    char info[128];
    char comment[128];
    uint8_t has_position;
    uint8_t has_weather;
    uint8_t marked;
} aprs_station_t;
```

### 8.2 Message

```c
typedef enum {
    APRS_MSG_NEW = 0,
    APRS_MSG_SENT,
    APRS_MSG_ACKED,
    APRS_MSG_REJECTED,
    APRS_MSG_READ
} aprs_message_state_t;

typedef struct {
    char id[16];
    char src[16];
    char dst[16];
    char text[256];
    aprs_message_state_t state;
    uint64_t ts_ms;
    uint8_t retry_count;
} aprs_message_t;
```

### 8.3 Frame

```c
typedef struct {
    char raw[512];
    size_t raw_len;
    char src[16];
    char dst[16];
    char path[128];
    uint8_t is_valid;
    uint8_t is_unique;
} aprs_frame_t;
```

### 8.4 GPS

```c
typedef struct {
    uint8_t fix;
    uint8_t valid;
    double lat;
    double lon;
    double speed_kmh;
    double course_deg;
    double altitude_m;
    uint64_t ts_ms;
} aprs_gps_t;
```

### 8.5 Stats

```c
typedef struct {
    uint64_t uptime_ms;
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t digi_repeats;
    uint32_t msg_received;
    uint32_t msg_sent;
    uint32_t invalid_frames;
    uint32_t unique_frames;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
} aprs_stats_t;
```

## 9. Event topics

Required topics:

- `frame.rx`
- `frame.tx`
- `frame.raw`
- `station.updated`
- `station.marked`
- `message.received`
- `message.sent`
- `message.state`
- `gps.updated`
- `position.changed`
- `position.captured`
- `beacon.sent`
- `digi.repeated`
- `digi.recent`
- `tx.state`
- `config.changed`
- `stats.updated`
- `stats.hourly`
- `stats.20h`
- `sound.event`
- `error`

## 10. WebSocket protocol

### 10.1 Envelope

```json
{
  "type": "command|event|snapshot|ack|error|hello|subscribe|unsubscribe|ping|pong",
  "id": "optional-message-id",
  "tsMs": 1717050000123,
  "topic": "optional.topic.name",
  "payload": {}
}
```

### 10.2 Handshake

Server hello:

```json
{
  "type": "hello",
  "tsMs": 1717050000000,
  "payload": {
    "engineVersion": "2.0",
    "protocolVersion": "1.0",
    "serverName": "aprs-engine",
    "capabilities": ["snapshot", "events", "commands", "filters", "editor", "digi", "stats-history"]
  }
}
```

Client subscribe:

```json
{
  "type": "subscribe",
  "id": "c-1001",
  "payload": {
    "topics": ["station.*", "message.*", "gps.*", "stats.*"],
    "filter": "callsign=SP3ABC-*"
  }
}
```

Ack:

```json
{
  "type": "ack",
  "id": "c-1001",
  "tsMs": 1717050000100,
  "payload": {
    "status": "ok"
  }
}
```

## 11. JSON command examples

### Capture station position

```json
{
  "type": "command",
  "id": "cmd-3001",
  "topic": "position.captured",
  "payload": {
    "callsign": "SP3XYZ-9"
  }
}
```

### Select position slot

```json
{
  "type": "command",
  "id": "cmd-3002",
  "topic": "position.select",
  "payload": {
    "slot": 2
  }
}
```

### Edit raw frame

```json
{
  "type": "command",
  "id": "cmd-3003",
  "topic": "frame.edit",
  "payload": {
    "raw": "SP3ABC-7>APRS:>Hello world"
  }
}
```

### Send edited frame

```json
{
  "type": "command",
  "id": "cmd-3004",
  "topic": "frame.send_editor",
  "payload": {}
}
```

### Set digi alias

```json
{
  "type": "command",
  "id": "cmd-3005",
  "topic": "digi.alias.set",
  "payload": {
    "slot": 1,
    "alias": "WIDE3",
    "mode": "trace",
    "maxHops": 3
  }
}
```

### Remote keys

```json
{
  "type": "command",
  "id": "cmd-3006",
  "topic": "remote.keys.generate",
  "payload": {}
}
```

## 12. Event examples

### Position captured

```json
{
  "type": "event",
  "tsMs": 1717050001200,
  "topic": "position.captured",
  "payload": {
    "callsign": "SP3XYZ-9",
    "slot": 2,
    "lat": 52.102,
    "lon": 16.921
  }
}
```

### Frame edited

```json
{
  "type": "event",
  "tsMs": 1717050001300,
  "topic": "frame.raw",
  "payload": {
    "buffer": "SP3ABC-7>APRS:>Hello world"
  }
}
```

### Station marked

```json
{
  "type": "event",
  "tsMs": 1717050001400,
  "topic": "station.marked",
  "payload": {
    "callsign": "SP3XYZ-9",
    "marked": true
  }
}
```

## 13. HTTP endpoints

Suggested endpoints:

- `GET /v2/state`
- `GET /v2/stations`
- `GET /v2/stations/{callsign}`
- `GET /v2/messages`
- `GET /v2/messages/{id}`
- `GET /v2/stats`
- `GET /v2/stats/hourly`
- `GET /v2/stats/20h`
- `GET /v2/digi/recent`
- `POST /v2/beacon/send`
- `POST /v2/messages/send`
- `POST /v2/position/capture`
- `POST /v2/position/select`
- `POST /v2/frame/edit`
- `POST /v2/frame/send`
- `POST /v2/tx/lock`
- `POST /v2/config/reset`

## 14. Implementation order

1. Extract pure data model.
2. Implement frame ingest and station manager.
3. Implement position manager.
4. Implement message manager.
5. Implement beacon scheduler.
6. Implement digi engine.
7. Implement stats history.
8. Implement raw frame editor.
9. Implement remote control.
10. Implement WebSocket and HTTP adapters.
11. Convert GUI to a client.
12. Add tests and compatibility checks.

## 15. Verification matrix

Every manual feature should map to a module and an API entry point.

- Station view -> Station manager.
- Radar -> Station manager + position manager.
- Messages -> Message manager.
- GPS view -> GPS manager.
- Position select -> Position manager.
- Beacon path/status rotation -> Beacon manager.
- TX lock -> Core config.
- Frame filtering -> Frame ingest.
- Raw view -> Raw frame editor.
- Station announce -> Event bus + UI client.
- Digi recent list -> Digi engine.
- Remote command -> Remote control.
- Statistics -> Stats engine.

## 16. Notes

The engine should preserve the original APRS behavior: unique frame handling, message retries, digipeater alias flexibility, manual and GPS positions, and diagnostic workflows. The old device UI can be rebuilt on top of this engine without compromising feature coverage.
```

## aprs_engine_v2.h

```c
#ifndef APRS_ENGINE_V2_H
#define APRS_ENGINE_V2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aprs_engine aprs_engine_t;

typedef enum {
    APRS_OK = 0,
    APRS_ERR_INVALID_ARG,
    APRS_ERR_NOT_FOUND,
    APRS_ERR_BUSY,
    APRS_ERR_DENIED,
    APRS_ERR_NO_MEMORY,
    APRS_ERR_TIMEOUT,
    APRS_ERR_INTERNAL
} aprs_status_t;

typedef enum {
    APRS_POSITION_AUTO = 0,
    APRS_POSITION_STATIC,
    APRS_POSITION_GPS
} aprs_position_mode_t;

typedef enum {
    APRS_DIGI_SIMPLE = 0,
    APRS_DIGI_FLOOD,
    APRS_DIGI_TRACE
} aprs_digi_type_t;

typedef enum {
    APRS_MSG_NEW = 0,
    APRS_MSG_SENT,
    APRS_MSG_ACKED,
    APRS_MSG_REJECTED,
    APRS_MSG_READ
} aprs_message_state_t;

typedef enum {
    APRS_EVT_FRAME_RX = 0,
    APRS_EVT_FRAME_TX,
    APRS_EVT_FRAME_RAW,
    APRS_EVT_STATION_UPDATED,
    APRS_EVT_STATION_MARKED,
    APRS_EVT_MESSAGE_RECEIVED,
    APRS_EVT_MESSAGE_SENT,
    APRS_EVT_MESSAGE_STATE,
    APRS_EVT_GPS_UPDATED,
    APRS_EVT_POSITION_CHANGED,
    APRS_EVT_POSITION_CAPTURED,
    APRS_EVT_BEACON_SENT,
    APRS_EVT_DIGI_REPEATED,
    APRS_EVT_DIGI_RECENT,
    APRS_EVT_TX_STATE_CHANGED,
    APRS_EVT_CONFIG_CHANGED,
    APRS_EVT_STATS_UPDATED,
    APRS_EVT_STATS_HOURLY,
    APRS_EVT_STATS_20H,
    APRS_EVT_SOUND,
    APRS_EVT_ERROR
} aprs_event_type_t;

typedef struct {
    char callsign;[1]
    char ssid;[2]
    char symbol_table;
    char symbol_code;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
    uint8_t unique_frames_only;
    uint8_t tcpip_enabled;
    uint8_t invalid_frames_enabled;
    uint16_t gps_baudrate;
    aprs_position_mode_t use_position_mode;
    aprs_position_mode_t report_position_mode;
} aprs_engine_config_t;

typedef struct {
    char callsign;[1]
    char last_path;
    char symbol_table;
    char symbol_code;
    double lat;
    double lon;
    double distance_km;
    double bearing_deg;
    uint32_t packet_count;
    uint64_t last_heard_ms;
    char info;
    char comment;
    uint8_t has_position;
    uint8_t has_weather;
    uint8_t marked;
} aprs_station_t;

typedef struct {
    char id;[1]
    char src;[1]
    char dst;[1]
    char text;
    aprs_message_state_t state;
    uint64_t ts_ms;
    uint8_t retry_count;
} aprs_message_t;

typedef struct {
    char raw;
    size_t raw_len;
    char src;[1]
    char dst;[1]
    char path;
    uint8_t is_valid;
    uint8_t is_unique;
} aprs_frame_t;

typedef struct {
    uint8_t fix;
    uint8_t valid;
    double lat;
    double lon;
    double speed_kmh;
    double course_deg;
    double altitude_m;
    uint64_t ts_ms;
} aprs_gps_t;

typedef struct {
    uint64_t uptime_ms;
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t digi_repeats;
    uint32_t msg_received;
    uint32_t msg_sent;
    uint32_t invalid_frames;
    uint32_t unique_frames;
    uint8_t tx_enabled;
    uint8_t digi_enabled;
    uint8_t beacon_enabled;
} aprs_stats_t;

typedef struct {
    aprs_event_type_t type;
    uint64_t ts_ms;
    const char* topic;
    const void* payload;
    size_t payload_size;
} aprs_event_t;

typedef void (*aprs_event_cb)(const aprs_event_t* ev, void* user);
typedef uint64_t aprs_subscription_id_t;

aprs_status_t aprs_engine_create(const aprs_engine_config_t* cfg, aprs_engine_t** out);
aprs_status_t aprs_engine_start(aprs_engine_t* e);
aprs_status_t aprs_engine_stop(aprs_engine_t* e);
void aprs_engine_destroy(aprs_engine_t* e);

aprs_status_t aprs_engine_set_config(aprs_engine_t* e, const aprs_engine_config_t* cfg);
aprs_status_t aprs_engine_get_config(aprs_engine_t* e, aprs_engine_config_t* out);
aprs_status_t aprs_engine_save_config(aprs_engine_t* e);

aprs_status_t aprs_engine_list_stations(aprs_engine_t* e, aprs_station_t* out, size_t* inout_count);
aprs_status_t aprs_engine_get_station(aprs_engine_t* e, const char* callsign, aprs_station_t* out);
aprs_status_t aprs_engine_mark_station(aprs_engine_t* e, const char* callsign, uint8_t mark);
aprs_status_t aprs_engine_sort_stations(aprs_engine_t* e, const char* column, uint8_t ascending);

aprs_status_t aprs_engine_set_use_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_report_position_mode(aprs_engine_t* e, aprs_position_mode_t mode);
aprs_status_t aprs_engine_set_static_position(aprs_engine_t* e, int slot, double lat, double lon, double alt);
aprs_status_t aprs_engine_get_static_position(aprs_engine_t* e, int slot, double* lat, double* lon, double* alt);
aprs_status_t aprs_engine_select_position_slot(aprs_engine_t* e, int slot);
aprs_status_t aprs_engine_capture_station_position(aprs_engine_t* e, const char* callsign);
aprs_status_t aprs_engine_set_manual_position(aprs_engine_t* e, double lat, double lon, double alt);

aprs_status_t aprs_engine_set_status_text(aprs_engine_t* e, int slot, const char* text);
aprs_status_t aprs_engine_set_path(aprs_engine_t* e, int slot, const char* path);
aprs_status_t aprs_engine_select_status_text(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_select_path(aprs_engine_t* e, int slot, uint8_t enabled);
aprs_status_t aprs_engine_set_beacon_delay(aprs_engine_t* e, int delay_sec);
aprs_status_t aprs_engine_send_beacon(aprs_engine_t* e);
aprs_status_t aprs_engine_preview_last_tx_frame(aprs_engine_t* e, aprs_frame_t* out);

aprs_status_t aprs_engine_send_message(aprs_engine_t* e, const char* dest, const char* text);
aprs_status_t aprs_engine_list_messages(aprs_engine_t* e, aprs_message_t* out, size_t* inout_count);
aprs_status_t aprs_engine_set_message_receive_mode(aprs_engine_t* e, uint8_t receive_all);
aprs_status_t aprs_engine_set_message_retry(aprs_engine_t* e, int retries, int delay_sec);
aprs_status_t aprs_engine_mark_message_read(aprs_engine_t* e, const char* id);

aprs_status_t aprs_engine_copy_frame_to_editor(aprs_engine_t* e, const char* frame_id);
aprs_status_t aprs_engine_get_editor_frame(aprs_engine_t* e, aprs_frame_t* out);
aprs_status_t aprs_engine_set_editor_frame_text(aprs_engine_t* e, const char* raw);
aprs_status_t aprs_engine_edit_editor_frame(aprs_engine_t* e, size_t offset, uint8_t value);
aprs_status_t aprs_engine_send_editor_frame(aprs_engine_t* e);
aprs_status_t aprs_engine_clear_editor_frame(aprs_engine_t* e);

aprs_status_t aprs_engine_set_digi_enabled(aprs_engine_t* e, uint8_t enabled);
aprs_status_t aprs_engine_set_digi_alias(aprs_engine_t* e, int slot, const char* alias, aprs_digi_type_t type, int max_hops);
aprs_status_t aprs_engine_set_digi_timing(aprs_engine_t* e, int dupetime_sec, int dupedelay_sec);
aprs_status_t aprs_engine_get_recent_digi(aprs_engine_t* e, char* out, size_t out_len);
aprs_status_t aprs_engine_set_digi_tx_monitor(aprs_engine_t* e, uint8_t enabled);

aprs_status_t aprs_engine_set_frame_filter(aprs_engine_t* e, uint8_t unique_only, uint8_t tcpip, uint8_t invalid_ok);
aprs_status_t aprs_engine_set_filter_expression(aprs_engine_t* e, const char* expr);

aprs_status_t aprs_engine_push_nmea(aprs_engine_t* e, const char* sentence);
aprs_status_t aprs_engine_get_gps(aprs_engine_t* e, aprs_gps_t* out);

aprs_status_t aprs_engine_get_stats(aprs_engine_t* e, aprs_stats_t* out);
aprs_status_t aprs_engine_get_hourly_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);
aprs_status_t aprs_engine_get_20h_stats(aprs_engine_t* e, uint32_t* buckets, size_t* inout_count);

aprs_status_t aprs_engine_generate_remote_keys(aprs_engine_t* e, char keys);[3][1]
aprs_status_t aprs_engine_execute_remote_command(aprs_engine_t* e, const char* src, const char* msg);

aprs_subscription_id_t aprs_engine_subscribe(aprs_engine_t* e, const char* topic_filter, aprs_event_cb cb, void* user);
aprs_status_t aprs_engine_unsubscribe(aprs_engine_t* e, aprs_subscription_id_t id);

#ifdef __cplusplus
}
#endif

#endif
