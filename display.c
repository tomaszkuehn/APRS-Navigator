#include "includes.h"
#include "display_backend.h"

static display_backend_t *disp_backend = 0;

void display_register_backend(display_backend_t *backend)
{
    disp_backend = backend;
}

display_backend_t *display_get_backend(void)
{
    return disp_backend;
}
