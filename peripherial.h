
#define SPI_SPEED 0x00000008

//extern char kbdbuf[16];

extern void initialize_peripherial();
extern char read_kbd();
extern void setsound(char id);
extern void spi_request_handler();
extern void send_m8_command(char comm,char *ss);
extern char get_m8_byte();
extern char test_crypt(char *cc);
extern char test_call(char *cc);



/* IO MAP */

//0x100000;   spi slave
//0x200000;   spi slave
//0x001000    P0.12 aprs - reset


