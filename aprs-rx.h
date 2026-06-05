

extern void aprs_write(unsigned int adr, unsigned char count, unsigned char *string);
extern void aprs_read(unsigned int adr, unsigned char count, unsigned char *string);

extern void aprs_initialize();
extern void aprs_rx_fopen();

extern char aprs_received_flag;
extern char aprs_squelch_flag;
//atomic flag jest zerowana przy update listy stacji w przerwaniu
extern char aprs_atomic_flag;
//exclusive flag zawiesza komunikacje z aprs-rx w przerwaniach
extern char aprs_exclusive_flag;
//blokada dostepu do danych
extern char aprs_data_flag;
extern char aprs_packet_source;
