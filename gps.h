#ifndef GPS_H
#define GPS_H

extern char gpslat[10];
extern char gpslon[11];
extern char gpscourse[7];
extern char gpsspeed[7];
extern char gpsvalid;		//valid coordinates only if gpsvalid==1
extern char gpssatnum;

extern char askgpsdata;       //set this to one to fill above buffers

extern void gps_process(void);
extern void init_gps(void);
extern void update_bygps();
extern void gps_read();

#endif

