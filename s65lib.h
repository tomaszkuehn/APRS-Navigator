extern void pixmap_x_sync(void);
extern int pixmap_sync;


extern unsigned int LCDcolor_foreground;
extern unsigned int LCDcolor_background;
extern char flagfixed; //fixedfont spacing
extern int cursorx;
extern int cursory;

extern void initLCD(void);
extern void LCDclear(void);
extern void LCDselect(unsigned long id);
extern void LCDyx(char y, char x);
extern void LCDcircle(int xCenter, int yCenter, int radius);
extern void LCDrectangle(int x0, int y0, int x1, int y1);
extern void LCDfillrectangle(int x0, int y0, int x1, int y1);
extern void LCDline(int x0, int y0, int x1, int y1);
//extern void LCDline_aa(int x0, int y0, int x1, int y1, char r, char g, char b);
extern void LCDellipse(int x, int y, int a, int b);
extern void setPixel(int x, int y);

extern void LCDfixed(char fixed);
extern void pchar(char font, char transparent, char c, int space);
extern void LCDtxt (char font, char *string);
extern void LCDtxtt (char font, char *string);
extern void LCDyxtxt(char font, char y, char x, char *string);
extern void LCDyxtxtt(char font, char y, char x, char *string);
//extern void LCDimg (char y, char x, unsigned char *string, int len);
extern void LCDchr(char font,char c);
extern void LCDchrt(char font,char c);
extern int get_x_key();

#define rgb(r,g,b)	(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

#define WHITE	0xFFFF
#define BLACK	0x0000
#define	PALIO	0xF000
#define GREEN   rgb(0,63,0)
#define YELLOW  rgb(31,63,0)
#define BLUE	rgb(0,0,31)
#define LIGHT_BLUE  rgb(15,31,31)
#define RED	rgb(31,0,0)
#define GREY    rgb(21,43,21)
#define BURSZTYN rgb(31,50,0)

#define F_SMALL   1
#define F_DEFAULT 0
#define F_14X22   3
#define F_DIGITS  4
#define F_8X10    5
#define F_15X15   6
#define F_20X23   7
#define F_SYM_MONO  8
#define F_ZNAKI     9
#define F_APRS    10
#define F_BIGSYM  11
//#define F_7X10    12

#define ICONKEY1  0
#define ICONKEY2  0x0E
#define ICONKEY3  0
#define ICONKEY4  0
#define ICONKEY5  0x11
#define ICONKEY6  0x12
#define ICONKEY7  0x13


