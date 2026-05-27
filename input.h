#ifndef INPUT_H
#define INPUT_H

/* ── Key codes (shared with app.c, graphics.c, config.c) ── */
#define KEY_UP      1
#define KEY_LEFT    2
#define KEY_RIGHT   3
#define KEY_DOWN    4
#define KEY_1       5
#define KEY_2       6
#define KEY_3       7
#define KEY_4       8
#define KEY_5       9
#define KEY_6       10
#define KEY_BKSPC   12
#define KEY_ENTER   13
#define KEY_MENU    17
#define KEY_CONFIG  18
#define KEY_SHIFT   20
#define KEY_ESC     27
#define KEY_INS     131

/* ── Hardware backend interface ── */
typedef struct {
    int  (*init)(void);
    int  (*read)(void);       /* non-blocking: 0=none, >0=keycode */
    void (*deinit)(void);
} input_backend_t;

/* ── Registration ── */
void input_register_backend(input_backend_t *backend);

/* ── Unified key read (injection queue → hardware backend) ── */
int  input_read_key(void);

/* ── User Control API — simulated configurable keyboard ── */
void input_inject_key(int keycode);
void input_inject_sequence(const char *keys);
void input_set_keymap(int from_key, int to_key);
void input_flush_queue(void);

#endif
