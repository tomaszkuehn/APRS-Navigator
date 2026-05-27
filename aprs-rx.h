

#ifndef APRS_RX_H
#define APRS_RX_H

#include <signal.h>

extern void aprs_write(unsigned int adr, unsigned char count, unsigned char *string);
extern void aprs_read(unsigned int adr, unsigned char count, unsigned char *string);

extern void aprs_initialize();
extern void aprs_rx_fopen();

extern volatile sig_atomic_t aprs_received_flag;
extern volatile sig_atomic_t aprs_squelch_flag;
//atomic flag jest zerowana przy update listy stacji w przerwaniu
extern volatile sig_atomic_t aprs_atomic_flag;
//exclusive flag zawiesza komunikacje z aprs-rx w przerwaniach
extern volatile sig_atomic_t aprs_exclusive_flag;
//blokada dostepu do danych
extern volatile sig_atomic_t aprs_data_flag;
extern char aprs_packet_source;

#endif
