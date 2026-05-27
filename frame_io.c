#include "includes.h"
#include "frame_io.h"

#define MAX_DRIVERS 8

static frame_io_driver_t *drivers[MAX_DRIVERS];
static int active_source = FRAME_IO_NONE;

void frame_io_register(int source_id, frame_io_driver_t *driver)
{
    if (source_id >= 0 && source_id < MAX_DRIVERS)
        drivers[source_id] = driver;
}

void frame_io_select(int source_id)
{
    active_source = source_id;
}

int frame_io_get_source(void)
{
    return active_source;
}

int frame_io_read(char *buffer, int maxlen)
{
    frame_io_driver_t *d;
    if (active_source < 0 || active_source >= MAX_DRIVERS) return 0;
    d = drivers[active_source];
    if (!d || !d->read_frame) return 0;
    return d->read_frame(buffer, maxlen);
}

int frame_io_write(const char *buffer, int len)
{
    frame_io_driver_t *d;
    if (active_source < 0 || active_source >= MAX_DRIVERS) return -1;
    d = drivers[active_source];
    if (!d || !d->write_frame) return -1;
    return d->write_frame(buffer, len);
}

int frame_io_squelch(void)
{
    frame_io_driver_t *d;
    if (active_source < 0 || active_source >= MAX_DRIVERS) return 0;
    d = drivers[active_source];
    if (!d || !d->get_squelch) return 0;
    return d->get_squelch();
}
