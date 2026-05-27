#ifndef EEPROM_H
#define EEPROM_H

#define MAXEEPROM       2000


extern void initialize_flash();
extern char flash_status();
extern unsigned int get_freepage();
extern void flash_read(char *data,unsigned int page, char count);
extern void flash_write(char *data,unsigned int page, char count);

/* zmienna ustawia sie na 1 po zapisaniu wszystkich dostepnych slotow */
extern char eepromfull;

#endif

