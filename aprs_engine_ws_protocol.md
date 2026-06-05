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