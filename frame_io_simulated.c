#include "includes.h"
#include "frame_io.h"
#include <stdio.h>
#include <string.h>

static char s0[]=" APZ01 0SP3WBA0RELAY 0WIDE  0# =5225.09N/01657.09E-received simulated traffic packet#";
static char s1[]=" APZ01 0SP3CDE0RELAY 0WIDE  0# =5225.18N/01656.18E-received simulated traffic packet#";
static char s2[]=" APZ01 0SP3XX 0RELAY 0WIDE  0# =5226.27N/01657.27E-received simulated traffic packet#";
static char s3[]=" APZ01 0SP3VER0RELAY 0WIDE  0# =5227.36N/01658.96E-received simulated traffic packet#";
static char s4[]=" APZ01 0SP3TTP0RELAY 0WIDE  0# =5225.45N/01659.45E-received simulated traffic packet#";
static char s5[]=" APZ01 0SP3CLL0RELAY 0WIDE  0# =5224.54N/01656.54E-received simulated traffic packet#";
static char s6[]=" APZ01 0SP3GR 0RELAY 0WIDE  0# =5226.63N/01657.63E-received simulated traffic packet#";
static char s7[]=" APZ01 0SP3TU 0RELAY 0WIDE  0# =5227.72N/01658.72E-received simulated traffic packet#";
static char s8[]=" APZ01 0SP3PAS0RELAY 0WIDE  0# =5223.81N/01655.81E-received simulated traffic packet#";
static char s9[]=" APZ01 0SP3ECH0RELAY 0WIDE  0# =5225.95N/01657.95E-received simulated traffic packet#";

static unsigned long simt = 0;

static void sim_gen_packet(char *out, int maxlen)
{
    int c;
    char tmpbuf[256];

    simt = timeval + (unsigned long)(40.0 * rand() / (RAND_MAX + 1.0)) + 20;
    simt = simt + 10;
    c = (int)(10.0 * rand() / (RAND_MAX));
    switch (c) {
    case 0: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s0); break;
    case 1: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s1); break;
    case 2: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s2); break;
    case 3: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s3); break;
    case 4: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s4); break;
    case 5: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s5); break;
    case 6: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s6); break;
    case 7: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s7); break;
    case 8: snprintf(tmpbuf, sizeof(tmpbuf), "%s", s8); break;
    default: snprintf(tmpbuf, sizeof(tmpbuf), "%s%lu", s9, timeval); break;
    }
    tmpbuf[29] = 0x03;
    tmpbuf[30] = 0xF0;
    c = (int)(10.0 * rand() / (RAND_MAX + 1.0)) + 65;
    c = 0;
    while (tmpbuf[c] != 0 && c < maxlen - 1) { out[c] = tmpbuf[c]; c++; }
    out[c] = 0;
}

static unsigned long mtmv = 0;

static int sim_read_frame(char *buffer, int maxlen)
{
    if (timeval <= mtmv) return 0;
    mtmv = timeval + (unsigned long)(40.0 * rand() / (RAND_MAX + 1.0)) + 20;
    sim_gen_packet(buffer, maxlen);
    return (int)strlen(buffer);
}

frame_io_driver_t frame_io_simulated = {
    0,               /* init */
    0,               /* deinit */
    sim_read_frame,
    0,               /* write_frame */
    0                /* get_squelch */
};
