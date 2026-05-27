# APRS Navigator — Architecture & API Reference

## Overview

The APRS Navigator has been refactored into four independent subsystems, each with a
pluggable backend interface. The core APRS processing engine remains unchanged.

```
                          ┌────────────────────────────┐
                          │     Application Layer       │
                          │  app.c (main loop, views,   │
                          │  menus, message handling)    │
                          └──┬───────┬───────┬──────────┘
                             │       │       │
        ┌────────────────────┼───────┼───────┼──────────────────────┐
        │                    │       │       │                       │
   ┌────▼─────┐   ┌──────────▼──┐ ┌──▼───────▼──┐   ┌───────────────▼──┐
   │ Display  │   │   Input     │ │  System     │   │   Frame I/O      │
   │ API      │   │   API       │ │  Engine     │   │   API            │
   │          │   │             │ │             │   │                  │
   │ s65lib.h │   │ input.h     │ │ sta_manager │   │ frame_io.h       │
   │ display_ │   │             │ │ timer.c     │   │                  │
   │ backend.h│   │ input.c     │ │ globals.c   │   │ frame_io.c       │
   │          │   │             │ │ gps.c       │   │                  │
   │ display.c│   │ input_      │ │ config.c    │   │ frame_io_        │
   │          │   │ backend_    │ │ eeprom.c    │   │ simulated.c      │
   │ s65lib.c │   │ x11.c       │ │             │   │ frame_io_file.c  │
   │  (X11)   │   │             │ │  ~17,600    │   │ frame_io_udp.c   │
   │          │   │             │ │  lines      │   │                  │
   └──────────┘   └─────────────┘ └─────────────┘   └──────────────────┘
```

Each subsystem is independently replaceable — swap the X11 display for SDL2 or an
ncurses TUI, inject keystrokes programmatically, or switch frame sources from
simulated traffic to a live UDP modem without touching engine code.

---

## 1. System Engine

The engine is the original, unmodified core — it has no knowledge of displays,
input devices, or frame sources.

### Files

| File | Lines | Purpose |
|------|-------|---------|
| `sta_manager.c` | 2,802 | Packet dispatch (`update_aprs`), position/message/wx decoders, digipeating (`do_repeat`), beaconing (`send_beacon`), station database, distance/bearing math |
| `timer.c` | 288 | 2 Hz scheduler (`tc1`), 10 ms tick (`tc0`), SIGALRM handler, timing primitives |
| `globals.c` | 718 | All global state, initialization, config persistence, position recalculation |
| `gps.c` | 307 | NMEA parser (GPGGA, GPRMC), coordinate extraction, checksum validation |
| `config.c` | 1,556 | Configuration menu pages (position, filter, TRX, aliases, paths, status, etc.) |
| `eeprom.c` | 94 | Flash/EEPROM emulation (circular buffer, page management) |

### Key Data Structures (globals.h)

```c
station_   stations[MAX_STATIONS];   // station database (151 entries)
_txque     txqueue[TXQUEUELEN];      // transmit FIFO (8 slots)
rpque_     rpque[DIGIQUEUELEN];      // digipeat queue (30 slots)
_recmessage messages[MAX_MESSAGES];  // received messages (20 slots)
_txmessage  txmessages[TX_MESSAGEBUF]; // pending transmissions (8 slots)
_aliases   aliases[4];               // digipeater alias table (RELAY, WIDE, etc.)
_path      paths[3];                 // transmit path definitions
_status    statusy[3];               // status text slots
```

### Engine Entry Points

| Function | File:Line | Called From |
|----------|-----------|-------------|
| `update_aprs(msg_start)` | sta_manager.c:2424 | `tc1()` in timer ISR |
| `station_sort(opt)` | sta_manager.c:86 | app.c display refresh |
| `station_list(startrow, startcol, cursor)` | sta_manager.c:499 | app.c display refresh |
| `send_beacon()` | sta_manager.c:725 | `tc1()` timer scheduler |
| `do_repeat()` | sta_manager.c:918 | `tc1()` timer scheduler |
| `send_txqueue()` | sta_manager.c:652 | `tc1()` timer scheduler |
| `recalc_positions()` | globals.c:700 | app.c when GPS changes |
| `dist_calc(m, lat2, lon2, recalc)` | sta_manager.c | distance/bearing between stations |

---

## 2. Display API

### 2.1 s65lib API (Primary Display Layer)

`s65lib.h` — The high-level display API used by graphics.c and app.c for all
rendering. Currently implemented by `s65lib.c` (X11 backend, 2,003 lines).

```
          graphics.c / app.c
                 │
                 ▼
         ┌──────────────┐
         │   s65lib.h    │  ◄── Public API (unchanged)
         │   s65lib.c    │  ◄── X11 implementation (unchanged)
         └──────────────┘
```

**Pixel operations**

| Function | Signature | Description |
|----------|-----------|-------------|
| `initLCD` | `void initLCD(void)` | Initialize display, open window |
| `LCDclear` | `void LCDclear(void)` | Clear current layer(s) |
| `LCDselect` | `void LCDselect(unsigned long id)` | Select layer (1=left, 2=right, 3=both) |
| `setPixel` | `void setPixel(int x, int y)` | Draw pixel at LCD coordinates |
| `LCDyx` | `void LCDyx(char y, char x)` | Set text cursor position |

**Shape operations**

| Function | Signature |
|----------|-----------|
| `LCDcircle` | `void LCDcircle(int xCenter, int yCenter, int radius)` |
| `LCDrectangle` | `void LCDrectangle(int x0, int y0, int x1, int y1)` |
| `LCDfillrectangle` | `void LCDfillrectangle(int x0, int y0, int x1, int y1)` |
| `LCDline` | `void LCDline(int x0, int y0, int x1, int y1)` |
| `LCDellipse` | `void LCDellipse(int x, int y, int a, int b)` |

**Text operations**

| Function | Signature |
|----------|-----------|
| `LCDfixed` | `void LCDfixed(char fixed)` — toggle fixed/proportional spacing |
| `LCDtxt` | `void LCDtxt(char font, char *string)` |
| `LCDtxtt` | `void LCDtxtt(char font, char *string)` — transparent background |
| `LCDyxtxt` | `void LCDyxtxt(char font, char y, char x, char *string)` |
| `LCDyxtxtt` | `void LCDyxtxtt(char font, char y, char x, char *string)` |
| `LCDchr` | `void LCDchr(char font, char c)` |
| `LCDchrt` | `void LCDchrt(char font, char c)` — transparent |
| `pchar` | `void pchar(char font, char transparent, char c, int space)` |
| `get_x_key` | `int get_x_key(void)` — poll simulated keypad (0=none) |

**Font identifiers**

| Constant | Size | Description |
|----------|------|-------------|
| `F_DEFAULT` (0) | — | Default system font |
| `F_SMALL` (1) | — | Small font |
| `F_14X22` (3) | 14×22 | Medium font |
| `F_DIGITS` (4) | — | Numeric digits only |
| `F_8X10` (5) | 8×10 | Small fixed-width |
| `F_15X15` (6) | 15×15 | Medium fixed-width |
| `F_20X23` (7) | 20×23 | Large |
| `F_SYM_MONO` (8) | — | Monospaced symbols |
| `F_ZNAKI` (9) | — | Character glyphs |
| `F_APRS` (10) | 37×37 | APRS logo |
| `F_BIGSYM` (11) | — | Large symbols |

**Color constants**

| Constant | Hex | Description |
|----------|-----|-------------|
| `WHITE` | `0xFFFF` | White (RGB 31,63,31) |
| `BLACK` | `0x0000` | Black |
| `RED` | — | `rgb(31,0,0)` |
| `GREEN` | — | `rgb(0,63,0)` |
| `BLUE` | — | `rgb(0,0,31)` |
| `YELLOW` | — | `rgb(31,63,0)` |
| `GREY` | — | `rgb(21,43,21)` |
| `LIGHT_BLUE` | — | `rgb(15,31,31)` |
| `BURSZTYN` | — | `rgb(31,50,0)` |
| `PALIO` | `0xF000` | |

**Globals set by the application**

| Variable | Type | Description |
|----------|------|-------------|
| `LCDcolor_foreground` | `unsigned int` | Current foreground color (RGB565) |
| `LCDcolor_background` | `unsigned int` | Current background color (RGB565) |
| `cursorx`, `cursory` | `int` | Text cursor position (set by `LCDyx`) |
| `flagfixed` | `char` | Fixed-width font spacing flag |
| `pixmap_sync` | `int` | Set to 1 to trigger display flush on next `read_kbd()` |

**LCD coordinate system + 2x scaling**

Two virtual LCD panels, each 132×176 logical pixels rendered at 2× scale
(264×352 physical pixels). Configured via `#define SCALE 2` in s65lib.c.

```
  ┌──────────────────┬──────────────────┐
  │  Layer 1 (LEFT)  │  Layer 2 (RIGHT) │
  │  132×176 logical │  132×176 logical │
  │  264×352 phys    │  264×352 phys    │
  │  X: 30..294      │  X: 480..744     │
  └──────────────────┴──────────────────┘
         ↑                    ↑
      S1X=30             S2X=480      SY=70 (top margin)
```

`LCDselect(1)` draws to left panel, `LCDselect(2)` to right, `LCDselect(3)` to both.

**Smooth pixel rendering**: When `SMOOTH_PIXEL` is enabled (default), each logical
pixel is drawn as a rounded 2×2 block — filling 2×1 main area plus a corner pixel —
producing anti-aliased edges on fonts, lines, and circles. Set `SMOOTH_PIXEL 0` in
s65lib.c for crisp block-pixel scaling. Set `SCALE` to 1 for original resolution.

### 2.2 Display Backend (Pluggable)

`display_backend.h` — Low-level backend vtable. Implement this to port the display
to a new platform (SDL2, ncurses, headless, etc.).

```c
typedef struct display_backend {
    int  (*init)(void);
    void (*deinit)(void);
    void (*set_pixel)(int x, int y, unsigned int color);
    void (*fill_rect)(int x, int y, int w, int h, unsigned int color);
    void (*draw_line)(int x1, int y1, int x2, int y2, unsigned int color);
    void (*draw_text)(int x, int y, const char *str, int len);
    int  (*text_width)(const char *str, int len);
    void (*present)(void);
    int  (*poll_key)(void);
    void (*set_fg)(unsigned int color);
} display_backend_t;

void display_register_backend(display_backend_t *backend);
```

| Function Pointer | Description | Notes |
|---|---|---|
| `init` | One-time display setup | Called from `initLCD()` → `panel_init()` |
| `deinit` | Tear down, close window | — |
| `set_pixel` | Draw single pixel at (x, y) | RGB565 color; must handle LCD→screen coord mapping |
| `fill_rect` | Filled rectangle | x,y = top-left; w,h = size |
| `draw_line` | Single-pixel-width line | Bresenham in reference impl |
| `draw_text` | Render string at (x, y) | For button labels and UI chrome |
| `text_width` | Pixel width of a string | For centering text on buttons |
| `present` | Flush backbuffer to screen | Called from `pixmap_x_sync()` |
| `poll_key` | Non-blocking key/button poll | 0 = no input, >0 = keycode |
| `set_fg` | Set foreground color | Maps RGB565 to native color |

**Registration** (in `demo.c`):
```c
extern display_backend_t display_backend_x11;  // from display_backend_x11.c
display_register_backend(&display_backend_x11);
```

`pixmap_sync` (int) — set to 1 by any code that modifies the display; the main loop
checks this flag and calls `pixmap_x_sync()` → `backend->present()` to flush.

---

## 3. Input API

`input.h` / `input.c` — The input subsystem provides a unified key event stream
from either hardware or programmatic injection, with optional key remapping.

### 3.1 Architecture

```
   Hardware (X11 mouse)
        │
   ┌────▼──────────────┐
   │ input_backend_x11  │   wraps get_x_key() from s65lib.c
   │ .read = x11_read   │
   └────────┬───────────┘
            │
   ┌────────▼───────────┐     ┌──────────────────┐
   │    input.c          │     │  Injection Queue  │
   │  input_read_key()   │◄────│  (64-entry FIFO)  │
   └────────┬───────────┘     └──────────────────┘
            │                        ▲
            │                 ┌──────┴──────────┐
            ▼                 │  User Control    │
      keycode (int)           │  input_inject_*  │
      returned to             └──────────────────┘
      peripherial.c
      read_kbd()
```

### 3.2 Key Codes

These constants replace the scattered `#define` values that were previously
duplicated across s65lib.c, graphics.c, and app.c.

| Constant | Value | Button / Key |
|----------|-------|-------------|
| `KEY_UP` | 1 | Up / Button 2 |
| `KEY_LEFT` | 2 | Left / Button 4 |
| `KEY_RIGHT` | 3 | Right / Button 6 |
| `KEY_DOWN` | 4 | Down / Button 8 |
| `KEY_1` | 5 | 1 / Button 1 (Opt) |
| `KEY_2` | 6 | 2 |
| `KEY_3` | 7 | 3 / Button 3 (Disp) |
| `KEY_4` | 8 | 4 |
| `KEY_5` | 9 | 5 / Button 5 (Ent) |
| `KEY_6` | 10 | 6 |
| `KEY_BKSPC` | 12 | Backspace |
| `KEY_ENTER` | 13 | Enter / OK |
| `KEY_MENU` | 17 | Menu / Button 7 |
| `KEY_CONFIG` | 18 | Config / Button 10 |
| `KEY_SHIFT` | 20 | Shift / Button 9 |
| `KEY_ESC` | 27 | Escape |
| `KEY_INS` | 131 | Insert |

> **Note:** `graphics.c` defines separate constants `KBD_LEFT` (129) and `KBD_RIGHT` (130)
> for cursor navigation within the soft keyboard (`read_kbdstr`). These are in a
> different keycode namespace and do not conflict with the main input API.

### 3.3 Hardware Backend Interface

```c
typedef struct {
    int  (*init)(void);        // optional — setup hardware
    int  (*read)(void);        // non-blocking: 0 = no key, >0 = keycode
    void (*deinit)(void);      // optional — cleanup
} input_backend_t;

void input_register_backend(input_backend_t *backend);
```

**Built-in backends:**

| Backend | File | Description |
|---------|------|-------------|
| `input_backend_x11` | `input_backend_x11.c` | Wraps `get_x_key()` from s65lib — X11 mouse clicks on simulator button panel |

**Registration** (in `demo.c`):
```c
extern input_backend_t input_backend_x11;
input_register_backend(&input_backend_x11);
```

**Writing a new backend:**
```c
// my_keyboard.c
#include "input.h"

static int my_read(void) {
    // poll your hardware, return keycode or 0
}

input_backend_t input_backend_myhw = { 0, my_read, 0 };
```

### 3.4 Main API — `input_read_key()`

```c
int input_read_key(void);
```

Returns a keycode (>0) or 0 (no key). Called by `read_kbd()` in `peripherial.c`.

**Priority order:**
1. Drain injection queue (programmatic keys take priority)
2. Query hardware backend
3. Apply key remap table to the raw keycode before returning

### 3.5 User Control API — Simulated Configurable Keyboard

```c
void input_inject_key(int keycode);
```

Push a single key event into the 64-entry circular injection queue. If the queue
is full, the event is silently dropped.

```c
void input_inject_sequence(const char *keys);
```

Parse a string and inject each character as a key event. Character-to-keycode mapping:

| Char | Keycode | Meaning |
|------|---------|---------|
| `U` | `KEY_UP` (1) | Cursor up |
| `D` | `KEY_DOWN` (4) | Cursor down |
| `L` | `KEY_LEFT` (2) | Cursor left |
| `R` | `KEY_RIGHT` (3) | Cursor right |
| `E` | `KEY_ENTER` (13) | Enter / select |
| `M` | `KEY_MENU` (17) | Open menu |
| `C` | `KEY_CONFIG` (18) | Open config |
| `1`–`6` | `KEY_1`–`KEY_6` (5–10) | Function buttons |

Unknown characters are silently skipped.

```c
void input_set_keymap(int from_key, int to_key);
```

Remap a physical keycode to a different logical keycode. The remap is applied to
both injected and hardware keys. Set `to_key = 0` to disable a key.

The keymap is reset to identity (each key maps to itself) on every call to
`input_register_backend()`.

```c
void input_flush_queue(void);
```

Discard all pending injected keys. Useful when entering a new screen or mode.

### 3.6 Usage Examples

**Navigate to station list, select station #3, return:**
```c
input_inject_sequence("DDD");   // cursor down 3 times
input_inject_sequence("E");     // enter/view station
input_inject_sequence("E");     // back to list
```

**Open menu, navigate to config, enter:**
```c
input_inject_sequence("M");     // menu
input_inject_sequence("DDD");   // down to config option
input_inject_sequence("E");     // select
```

**Remap button 1 to act as Enter:**
```c
input_set_keymap(KEY_1, KEY_ENTER);
```

**Disable a key entirely:**
```c
input_set_keymap(KEY_MENU, 0);
```

**Flush stale keys on screen transition:**
```c
input_flush_queue();
```

---

## 4. Frame I/O API

`frame_io.h` / `frame_io.c` — Pluggable packet source/sink abstraction. Register
one or more drivers, select which one is active, and read/write APRS frames
through a unified interface.

### 4.1 Architecture

```
   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐
   │ simulated.c  │   │  file.c      │   │  udp.c       │   │  (future)    │
   │ sim_gen_     │   │  fgets()     │   │  recvfrom()  │   │  serial,     │
   │ packet()     │   │  on it.dat   │   │  UDP :464    │   │  APRS-IS,    │
   │              │   │              │   │              │   │  TNC, etc.   │
   └──────┬───────┘   └──────┬───────┘   └──────┬───────┘   └──────┬───────┘
          │                  │                  │                  │
   ┌──────▼──────────────────▼──────────────────▼──────────────────▼───────┐
   │                         frame_io.c                                     │
   │  Driver registry (MAX_DRIVERS=8)                                       │
   │  Active source selection                                               │
   │  frame_io_read() / frame_io_write() / frame_io_squelch()              │
   └──────────────────────────────────┬────────────────────────────────────┘
                                      │
                          ┌───────────▼───────────┐
                          │     aprs-rx.c          │
                          │  aprs_read() polls     │
                          │  frame_io when         │
                          │  aprs_packet_source    │
                          │  >= 90                 │
                          └───────────────────────┘
```

### 4.2 Source Identifiers

| Constant | Value | Driver |
|----------|-------|--------|
| `FRAME_IO_NONE` | 0 | No source active |
| `FRAME_IO_SIMULATED` | 10 | `frame_io_simulated.c` — synthetic APRS traffic |
| `FRAME_IO_UDP` | 1 | `frame_io_udp.c` — remote modem via UDP port 464 |
| `FRAME_IO_FILE` | 3 | `frame_io_file.c` — replay from `it.dat` |
| `FRAME_IO_APRSIS` | 4 | Reserved for APRS-IS TCP client |

### 4.3 Driver Interface

```c
typedef struct {
    int  (*init)(void);
    void (*deinit)(void);
    int  (*read_frame)(char *buffer, int maxlen);
    int  (*write_frame)(const char *buffer, int len);
    int  (*get_squelch)(void);
} frame_io_driver_t;
```

| Function Pointer | Returns | Description |
|---|---|---|
| `init` | 0 = ok, -1 = error | One-time resource setup (open socket, file, etc.) |
| `deinit` | — | Release resources |
| `read_frame` | >0 = bytes read, 0 = no data, -1 = error | Non-blocking frame read into buffer; buffer is null-terminated on success |
| `write_frame` | 0 = ok, -1 = error | Transmit a frame (optional — set to 0 for read-only drivers) |
| `get_squelch` | 0–255 | Current channel activity level (0 = silent) |

### 4.4 Registry API

```c
void frame_io_register(int source_id, frame_io_driver_t *driver);
```

Register a driver at the given source ID slot. Up to 8 drivers (IDs 0–7)
may be registered simultaneously. Overwrites any driver previously registered
at the same ID.

```c
void frame_io_select(int source_id);
```

Set the active frame source. All subsequent `frame_io_read()` / `frame_io_write()`
calls are dispatched to the driver registered at this ID.

```c
int frame_io_get_source(void);
```

Return the currently active source ID.

### 4.5 Unified I/O

```c
int frame_io_read(char *buffer, int maxlen);
```

Read one frame from the active driver. Returns frame length (>0), 0 (no data
available), or -1 (error). The buffer is null-terminated.

```c
int frame_io_write(const char *buffer, int len);
```

Transmit a frame through the active driver. Returns 0 on success, -1 on error.

```c
int frame_io_squelch(void);
```

Return the active driver's squelch level (0–255).

### 4.6 Built-in Drivers

**`frame_io_simulated`** — Generates synthetic APRS position reports from 10
virtual stations (SP3WBA, SP3CDE, SP3XX, etc.) with randomized timing. Frames
include latitude/longitude near 52°25'N 016°57'E (Poznan area). Read-only
(no transmit). No initialization required.

**`frame_io_file`** — Reads APRS frames from `it.dat`, one line at a time via
`fgets()`. Frame format: human-readable APRS text parsed by `parse_is()` in
aprs-rx.c. Requires `init()` call to open the file. Read-only.

**`frame_io_udp`** — Communicates with a remote APRS modem via UDP datagrams
on port 464. Implements the sequence-number protocol from the original
`remote_read()` / `remote_connect()` in aprs-rx.c. Requires `init()` call to
bind the UDP socket. Read + squelch supported.

### 4.7 Integration with aprs-rx.c

The frame_io path activates when `aprs_packet_source` is set to 90 or above:

```c
// In aprs_read():
if (aprs_packet_source >= 90 && (timeval > mtmv)) {
    int len;
    char fbuf[300];
    len = frame_io_read(fbuf, sizeof(fbuf) - 1);
    if (len > 0) {
        fbuf[len] = 0;
        // copy into bufmem[], set bufmem[0x201]=1 (rx bufflag)
    }
}
```

The legacy hardcoded sources (1=UDP, 2=network, 3=file, 10=simulated) remain
functional. The frame_io path is an additional option.

### 4.8 Usage Examples

**Registration (in demo.c):**
```c
extern frame_io_driver_t frame_io_simulated;
extern frame_io_driver_t frame_io_file;
extern frame_io_driver_t frame_io_udp;

frame_io_register(FRAME_IO_SIMULATED, &frame_io_simulated);
frame_io_register(FRAME_IO_FILE,      &frame_io_file);
frame_io_register(FRAME_IO_UDP,       &frame_io_udp);

// Select simulated source and set aprs-rx to use frame_io
frame_io_select(FRAME_IO_SIMULATED);
aprs_packet_source = 90;  // frame_io path
```

**Writing a custom driver (serial TNC):**
```c
// frame_io_tnc.c
#include "frame_io.h"

static int tnc_fd = -1;

static int tnc_init(void) {
    tnc_fd = open("/dev/ttyS0", O_RDWR | O_NONBLOCK);
    // configure baud rate, etc.
    return (tnc_fd >= 0) ? 0 : -1;
}

static int tnc_read(char *buf, int max) {
    int n = read(tnc_fd, buf, max - 1);
    if (n > 0) buf[n] = 0;
    return n;
}

static int tnc_write(const char *buf, int len) {
    return write(tnc_fd, buf, len) == len ? 0 : -1;
}

frame_io_driver_t frame_io_tnc = {
    tnc_init, 0, tnc_read, tnc_write, 0
};
```

---

## 5. Data Flow

### 5.1 Packet Reception

```
[Frame Source]                  [Engine]                    [Display]
     │                              │                           │
  frame_io_read()              update_aprs()              station_list()
     │                              │                           │
     ├─► bufmem[] ──► tc1() ──► buffer[] ──► stations[] ──► sort[]
     │   (aprs-rx)   (timer)      │            database       (filtered
     │                             │                           view)
     │                    ┌────────┼────────┐
     │                    │        │        │
     │               decode_*  digi_path  add_message
     │               (position (digipeat  (messages[])
     │                extract)  queue)
     │
  [Source drivers:
   simulated / file / UDP / serial]
```

### 5.2 Packet Transmission

```
[Engine]                       [Output]
     │                              │
  send_beacon()               send_txqueue()
  do_repeat()                      │
  send_ack()                  aprs_write()
     │                         (modem registers)
     └──► txqueue[] ──► tc1() ──► bufmem[] ──► (radio/TNC hardware)
```

### 5.3 Input Flow

```
[Hardware]           [Input Subsystem]           [Application]
    │                       │                         │
 X11 mouse ──► get_x_key()  │                         │
               (s65lib.c)   │                         │
                     │      │                         │
                     ▼      │                         │
              input_backend_x11.read()                │
                     │      │                         │
                     ▼      │                         │
              ┌──────────────┐                        │
              │ input_read_  │                        │
              │ key()        │──► read_kbd() ──► goapp() key handler
              │              │   (peripherial)   (app.c)
              │ [injection   │
              │  queue]      │
              └──────────────┘
```

### 5.4 Display Flow

```
[Application]            [s65lib API]           [X11 Backend]
     │                        │                       │
  goapp()                initLCD()               open_x_window()
  update_status          LCDselect()             XCreateSimpleWindow()
     │                   LCDclear()              XFillRectangle()
     │                   LCDyxtxt()              pchar()→setPixel()
     ├─► station_list()  LCDline()               XDrawPoint()
     ├─► radar()         LCDcircle()             XFillArc()
     ├─► compass_page()  pixmap_x_sync()         XCopyArea()+XSync()
     └─► station_show()  get_x_key()             XMaskEvent()
```

---

## 6. Startup Sequence

`demo.c:main()` performs initialization in this order:

```
 1. srand(time(NULL))
 2. init_timer()               ── ualarm() for 50 Hz SIGALRM
 3. initialize_peripherial()   ── no-op in PC port
 4. initialize_flash()         ── eeprom emulation
 5. globals_initialize()       ── loads config from flash pages 2002-2004
 6. restore_factory()          ── sets station identity, aliases, paths, status
 7. init_gps()                 ── zeros GPS buffers
 8. aprs_initialize()          ── resets modem flags, bufmem
 9. update_aprs_config()       ── programs modem registers
10. input_register_backend()   ── input_backend_x11 ← NEW
11. frame_io_register() × 3    ── simulated, file, UDP    ← NEW
12. initLCD()                  ── opens X11 window, draws button panel
13. goapp()                    ── enters main loop
```

---

## 7. Build System

### makefile

```makefile
CC      = gcc
CFLAGS  = -funsigned-char -Wall -Wextra -std=c99
LDLIBS  = -lX11 -lm

OBJS    = globals.o app.o aprs-rx.o config.o eeprom.o gps.o graphics.o \
          peripherial.o serial.o sta_manager.o demo.o timer.o s65lib.o \
          display.o input.o input_backend_x11.o \
          frame_io.o frame_io_simulated.o frame_io_file.o frame_io_udp.o
```

### Targets

| Target | Description |
|--------|-------------|
| `make` | Build `aprs` binary |
| `make clean` | Remove `aprs` and all `.o` files |

### File Map

```
aprs/
├── ARCHITECTURE.md          ← this document
├── makefile                 ← updated with new modules
│
├── [System Engine — unchanged]
│   ├── sta_manager.c/.h     APRS packet processing, station DB
│   ├── timer.c/.h           Scheduler, timekeeping
│   ├── globals.c/.h         Global state, config persistence
│   ├── gps.c/.h             NMEA parser
│   ├── config.c/.h          Configuration menu pages
│   ├── eeprom.c/.h          Flash/EEPROM emulation
│   ├── serial.c/.h          UART stubs
│   └── peripherial.c/.h     SPI handler, sounds, anti-piracy stubs
│
├── [Display Subsystem]
│   ├── s65lib.h             Public display API
│   ├── s65lib.c             X11 implementation (2,003 lines)
│   ├── graphics.c/.h        High-level UI rendering (radar, compass, menus)
│   ├── display_backend.h    Pluggable backend vtable          ← NEW
│   └── display.c            Backend registration              ← NEW
│
├── [Input Subsystem]
│   ├── input.h              Key codes + injection API         ← NEW
│   ├── input.c              Injection queue + remapping       ← NEW
│   └── input_backend_x11.c  X11 mouse→key wrapper             ← NEW
│
├── [Frame I/O Subsystem]
│   ├── frame_io.h           Driver interface + source IDs     ← NEW
│   ├── frame_io.c           Registry + dispatch               ← NEW
│   ├── frame_io_simulated.c Synthetic traffic generator       ← NEW
│   ├── frame_io_file.c      File replay driver                ← NEW
│   └── frame_io_udp.c       UDP remote modem driver           ← NEW
│
├── [Application Layer]
│   ├── app.c/.h             Main loop, views, message UI
│   ├── demo.c               Entry point + backend registration
│   └── includes.h           Central include file
│
├── [Legacy / Standalone]
│   ├── aprs-rx.c/.h         Modem register emulation + frame_io integration
│   ├── aprs-ip.c/.h         APRS-IS TCP client (standalone)
│   ├── telnet.c             Telnet client (standalone)
│   ├── xtea.c               XTEA encryption (keys redacted)
│   └── ff.c                 Flash format utility
│
├── [Font Data — unchanged]
│   ├── F15x22.h, f8x8.h, f8x10.h, f8x11.h, f9x14.h
│   ├── f12x16.h, f15x15.h, f19x23.h, f20x23.h, f23x28.h
│   ├── f7x10.h, bigsym.h, sym_mono.h, znaki.h, digits.h, aprs.h
│   └── icon.jpg, icon.png
│
├── ip/                     Standalone IP tools
│   ├── server.c            TCP station list server
│   ├── client.c            TCP client
│   ├── simserver.c         UDP modem simulator
│   └── nc.c                Netcat utility
│
└── it.dat                  Recorded APRS traffic for file replay
```

---

## 8. Compilation

### Requirements

- **Linux** with X11 development libraries
- **GCC** 15.x+ (or any C99-capable compiler)
- **libx11-dev** — `sudo apt-get install libx11-dev`
- **make**

### Build

```bash
cd aprs
make clean
make
```

**Result:** `aprs` — 238 KB ELF 64-bit executable

**Output:**
```
rm -f aprs *.o
gcc -c -funsigned-char -Wall -Wextra -std=c99 globals.c
... (all .c files)
gcc -o aprs ... globals.o ... frame_io_udp.o -lX11 -lm
```

- **Errors:** 0
- **New file warnings:** 0
- **Pre-existing warnings:** ~1,350 (char subscript, unused parameter, K&R decls — not regressions)

### Run

```bash
./aprs
```

Opens an X11 window with the simulator panel and two virtual LCD displays.
