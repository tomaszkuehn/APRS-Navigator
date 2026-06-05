

extern unsigned long timeval;
extern unsigned long utimeval;
extern char lbufflag;

extern void init_timer (void);
extern void waitms(unsigned long tt);
extern void tc0 (void);
extern void tc1 (void);
extern void tc1_poll (void);
