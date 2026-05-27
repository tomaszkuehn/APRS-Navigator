#ifndef DISPLAY_BACKEND_H
#define DISPLAY_BACKEND_H

/* Display backend vtable — implement one per platform (X11, SDL2, headless, etc.) */

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

/* These globals are set by the backend to enable main-loop sync */
extern int pixmap_sync;

#endif
