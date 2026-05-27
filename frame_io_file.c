#include "includes.h"
#include "frame_io.h"
#include <stdio.h>
#include <string.h>

static FILE *it_fp = 0;

static int file_init(void)
{
    if (!it_fp) it_fp = fopen("it.dat", "r");
    return it_fp ? 0 : -1;
}

static int file_read_frame(char *buffer, int maxlen)
{
    if (!it_fp) return 0;
    if (fgets(buffer, maxlen, it_fp))
        return (int)strlen(buffer);
    return 0;
}

frame_io_driver_t frame_io_file = {
    file_init,
    0,               /* deinit */
    file_read_frame,
    0,               /* write_frame */
    0                /* get_squelch */
};
