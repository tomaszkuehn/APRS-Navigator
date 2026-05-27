#ifndef GRAPHICS_H
#define GRAPHICS_H

extern void station_announce(int id);
extern void station_announce_clear();
extern void message_announce(int id);
extern void compass_page();
extern void text_scroll(int y, int x, int x2, char *str,int pos);
extern void list_header(char startpos, char markpos);
extern void radar(char cstation,int range,int ang,char rdrw);
extern void list_info_header();
extern void menu_header(char *str);
extern void menu();
extern void quick_menu();
extern void station_traker(char id,int mycourse);
extern char read_kbdstr(char y,char x,char len,char opt,char *string);
//extern int strlen(char *str);

extern char scrolling_txt[80];
typedef struct {
  char active;
  char x,y,x1;
  int pos,pose;
} __scrolling_txt_status;

extern __scrolling_txt_status scrolling_txt_status;
extern char announce_status;
extern char nightv;

extern char disp1;
extern char disp2;

#define DISP_LIST  1
#define DISP_RADAR 2
#define DISP_GPS   4
#define DISP_STATION 8
#define DISP_MESSAGES 16




extern const char f15x22[];
extern const char aprs[];
extern const char bigsym[];
extern const char digits[];
//#include "f12x16.h"
extern const char f15x15[];
//#include "f19x23.h"
extern const char f20x23[];
//#include "f23x28.h"
//#include "f7x10.h"
extern const char f8x10[];
//#include "f8x11.h"
//#include "f8x8.h"
//#include "f9x14.h"
extern const char sym_mono[];
extern const char znaki[];

#endif
