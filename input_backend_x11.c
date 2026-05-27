#include "includes.h"
#include "input.h"

/* get_x_key() is in s65lib.c — the X11 keyboard/mouse poll */
extern int get_x_key(void);

static int x11_input_read(void)
{
    return get_x_key();
}

input_backend_t input_backend_x11 = {
    0,               /* init — display init handles X11 setup */
    x11_input_read,
    0                /* deinit */
};
