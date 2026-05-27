#include "includes.h"
#include "frame_io.h"
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

static int sock = -1;
static unsigned long myseq = 0;

static int udp_init(void)
{
    struct sockaddr_in echoserver;

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("frame_io_udp: Failed to create socket\n");
        return -1;
    }
    memset(&echoserver, 0, sizeof(echoserver));
    echoserver.sin_family = AF_INET;
    echoserver.sin_port = htons(464);
    if (bind(sock, (struct sockaddr *)&echoserver, sizeof(echoserver)) < 0) {
        printf("frame_io_udp: Failed to bind\n");
        return -1;
    }
    fcntl(sock, F_SETFL, O_NONBLOCK);
    myseq = 0;
    printf("frame_io_udp: connected\n");
    return 0;
}

static int udp_read_frame(char *buffer, int maxlen)
{
    struct sockaddr_in csin, sin;
    char rbuf[300];
    int len, i, bytes;
    unsigned long nseq;

    memset(&csin, 0, sizeof(csin));
    bytes = sizeof(csin);
    len = recvfrom(sock, rbuf, sizeof(rbuf), 0, (struct sockaddr *)&csin, &bytes);
    if (len <= 1) return 0;

    nseq = 0;
    nseq = nseq + rbuf[8];  nseq = nseq << 8;
    nseq = nseq + rbuf[9];  nseq = nseq << 8;
    nseq = nseq + rbuf[10]; nseq = nseq << 8;
    nseq = nseq + rbuf[11];

    if (rbuf[0] == 0x20 && nseq > myseq) {
        i = 0;
        while (i < 250 && i < maxlen - 1 && rbuf[12 + i] != 0x03) {
            buffer[i] = rbuf[12 + i];
            i++;
        }
        while (i < 250 && i < maxlen - 1 && rbuf[12 + i] != 0) {
            buffer[i] = rbuf[12 + i];
            i++;
        }
        buffer[i] = 0;
        myseq = nseq;
    }
    if (rbuf[0] == 0x10 && nseq < myseq) {
        myseq = nseq;
    }

    /* Send status ack */
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = htons(464);
    rbuf[0] = 0x10;
    rbuf[4] = (unsigned char)(myseq & 0xFF000000) >> 24;
    rbuf[5] = (unsigned char)(myseq & 0x00FF0000) >> 16;
    rbuf[6] = (unsigned char)(myseq & 0x0000FF00) >> 8;
    rbuf[7] = (unsigned char)(myseq & 0x000000FF);
    rbuf[8] = 0;
    sendto(sock, rbuf, 9, 0, (struct sockaddr *)&sin, sizeof(sin));

    return (int)strlen(buffer);
}

static int udp_squelch(void)
{
    return 0; /* squelch comes from bufmem[0x200] which aprs-rx manages */
}

frame_io_driver_t frame_io_udp = {
    udp_init,
    0,               /* deinit */
    udp_read_frame,
    0,               /* write_frame */
    udp_squelch
};
