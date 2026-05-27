

#ifndef STA_MANAGER_H
#define STA_MANAGER_H

extern void update_aprs(char msg_start);
extern char station_list(char startrow,char startcol,char cursor);
extern void station_sort(char opt);
extern float convert_position(char latlon, char m);
extern void dist_calc(char m,float lat2, float lon2, char recalc);
extern void send_beacon();
extern void do_repeat();
extern void station_serial_list();
extern void station_www_list();
extern void send_txqueue();
extern void transmit_message();
extern void application_frame_get();
extern void application_frame_send(char *ss);

#endif

