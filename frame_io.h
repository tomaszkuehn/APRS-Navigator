#ifndef FRAME_IO_H
#define FRAME_IO_H

/* ── Packet source identifiers ── */
#define FRAME_IO_NONE        0
#define FRAME_IO_SIMULATED  10
#define FRAME_IO_UDP         1
#define FRAME_IO_FILE        3
#define FRAME_IO_APRSIS      4

/* ── Driver interface ── */
typedef struct {
    int  (*init)(void);
    void (*deinit)(void);
    int  (*read_frame)(char *buffer, int maxlen);
    int  (*write_frame)(const char *buffer, int len);
    int  (*get_squelch)(void);
} frame_io_driver_t;

/* ── Registry ── */
void frame_io_register(int source_id, frame_io_driver_t *driver);
void frame_io_select(int source_id);
int  frame_io_get_source(void);

/* ── Unified I/O ── */
int  frame_io_read(char *buffer, int maxlen);
int  frame_io_write(const char *buffer, int len);
int  frame_io_squelch(void);

#endif
