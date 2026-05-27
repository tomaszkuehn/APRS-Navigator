
#include "includes.h"

#include <stdio.h>		/* For NULL	*/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <signal.h>
#include <sys/io.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

//#define sim


#define S1X	15
#define S2X	240
#define SY	35

#define  uint32_t  unsigned long
#define  uint16_t  unsigned int
#define	 uint8_t   unsigned char

#define DISP_W 132
#define DISP_H 176
#define CHAR_H 14
#define CHAR_W 8
#define TEXT_COL 16
#define TEXT_ROW 12

#define SPIF 0x80

int cursorx=0;
int cursory=0;
char flagfixed=0; //fixedfont spacing
uint32_t  lcds;
unsigned int LCDcolor_foreground;
unsigned int LCDcolor_background;


	
static	Display *disp;
static	GC gc;
static	Window window;
static  int width=0, height=0;
static  Colormap cm;
static  Pixmap pixmap;
static  XFontStruct *font_info;
static  XFontStruct *font1;	//smallest
static  XFontStruct *font2;
static  XFontStruct *font3;	//largest
static  int myx_colmode=0;
static  unsigned long coltab[4097];
struct b1 {
    int x1;
    int x2;
    int y1;
    int y2;
    int active;
    char title[30];
};
typedef struct b1 button;
button buttons[100];
int buttons_num=0;
int pixmap_sync=0;

#define XWh	260

//############## XWindows graphics procedures ##############

static XColor colmem;

void set_x16_color(unsigned int c)
{
static XColor xc;
static unsigned int cmem;
unsigned int c1,c2,c3;
if(c==cmem)return;
cmem=c;
c1=c&0xF800;
c2=c&0x07E0;c2=c2<<5;
c3=c&0x001F;c3=c3<<11;
colmem.red=c1;
colmem.green=c2;
colmem.blue=c3;
colmem.flags=255;
 XAllocColor(disp,cm,&colmem);
 XSetForeground(disp,gc,colmem.pixel);
}

void set_x_color(r,g,b)
double r,g,b;
{
 static unsigned long k;
 static XColor xc;

 if(myx_colmode==0)
   { 
 xc.red=65534*r; xc.green=65534*g; xc.blue=65534*b; xc.flags=255;
 if(XAllocColor(disp,cm,&xc));
 XSetForeground(disp,gc,xc.pixel);
   }
else
  {
 k=r+16*g+256*b;
 XSetForeground(disp,gc,k);
  }
}

void set_x_colmode(m)
int m;
{
 myx_colmode=m;
}

unsigned long get_x_color(r,g,b)
double r,g,b;
{
 static XColor xc;
  
 xc.red=65534*r; xc.green=65534*g; xc.blue=65534*b; xc.flags=255;
 if(XAllocColor(disp,cm,&xc));
 return(xc.pixel);
}


void set_x_coltab()
{
int ri,gi,bi;

for(ri=0;ri<16;ri++)
 for(gi=0;gi<16;gi++)
  for(bi=0;bi<16;bi++)
    {
   coltab[ri+16*gi+256*bi]=get_x_color((double)ri/15,(double)gi/15,(double)bi/15);
    };
}


void draw_text(int x, int y, char *napis)
{
XDrawString(disp,pixmap,gc,x,y,napis,strlen(napis));
}

int text_length(char *napis, XFontStruct *font)
{
int ll;

ll=XTextWidth(font,napis,strlen(napis));
return ll;
}


void draw_x_line(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
 XDrawLine(disp,pixmap,gc,x1,y1,x2,y2);
}

void draw_x_pixel(x,y)
int x,y;
{
 XDrawPoint(disp,pixmap,gc,x,y);
}

void clear_x_window()
{

 XFillRectangle(disp,pixmap,gc,0,0,490,XWh);
}

void pixmap_x_sync()
{
 XCopyArea(disp,pixmap,window,gc,0,0,490,XWh,0,0);
 XSync(disp,0);
}

#define	KEY_UP		1
#define KEY_LEFT	2
#define KEY_RIGHT	3
#define KEY_DOWN	4
#define KEY_1		5
#define KEY_2		6
#define KEY_3		7
#define KEY_4		8
#define KEY_5		9
#define KEY_6		10
/*
unsigned int get_x_key()
{
unsigned char keys[32];
int key;

key=0;				//no action
XQueryKeymap(disp,keys);
if (keys[12]==4){key=KEY_UP;};
if (keys[12]==16){key=KEY_LEFT;};
if (keys[12]==64){key=KEY_RIGHT;};
if (keys[13]==1){key=KEY_DOWN;};
if (keys[1]==4){key=KEY_1;};
if (keys[1]==8){key=KEY_2;};
if (keys[1]==16){key=KEY_3;};
if (keys[1]==32){key=KEY_4;};
if (keys[1]==64){key=KEY_5;};
if (keys[1]==128){key=KEY_6;};
return(key);
}
*/
void open_x_window()
{
int screen, depth;
unsigned long bg,fg;
char *font="8x13";
//char *font="9x14";
	
	printf("Disp\n");
        disp = XOpenDisplay(0);
	printf("Disp done\n");
	if (disp == (Display *) NULL)
	{	fprintf(stderr, "Could not open display\n");
		exit(1);
	}
	screen = DefaultScreen(disp);
	printf("screen done\n");
	bg = BlackPixel(disp, screen);
	fg = WhitePixel(disp, screen);
	
	font_info=XLoadQueryFont(disp,font);
  if (!(font_info)){printf("No font_info!\n");exit(1);}
	printf("font done\n");
        width=490; height=XWh;
	
        depth = DefaultDepth (disp, screen);
	window = XCreateSimpleWindow (disp, DefaultRootWindow(disp),
		100, 50, width, height, 0, bg, fg);
	printf("window done\n");	
	pixmap = XCreatePixmap(disp,window,width,height,depth);
	printf("pixmap done\n");
	gc = XCreateGC (disp, window, 0, 0);
	printf("gc done\n");
        //XSetFont(disp,gc,font_info->fid);
        printf("XSet font done\n");
	
        /*XSelectInput (disp, window,
	   KeyPressMask | KeyReleaseMask | StructureNotifyMask); */
	XMapRaised (disp, window);
	printf("XMap done\n");
        cm=DefaultColormap(disp,0);
        XSetGraphicsExposures(disp, gc, 0);
	printf("XSetGr done\n");
        set_x_coltab();
        set_x_colmode(0);
	printf("done\n");
}

void close_x_window()
{
 XFreeGC (disp, gc);
 XDestroyWindow (disp, window);
 XCloseDisplay (disp);
}



//#############################################################
//############# APPLICATION ###################################
//#############################################################


int get_x_numkey()
{
XEvent event;
unsigned char keys[32];
int key;
		key=255; //no key

		XMaskEvent(disp,KeyPressMask|ButtonPressMask,&event);
		
		XQueryKeymap(disp,keys);
		if (keys[1]==4){key=1;};
		if (keys[1]==8){key=2;};
		if (keys[1]==16){key=3;};
		if (keys[1]==32){key=4;};
		if (keys[1]==64){key=5;};
		if (keys[1]==128){key=6;};
		if (keys[2]==1){key=7;};
		if (keys[2]==2){key=8;};
		if (keys[2]==4){key=9;};
		if (keys[2]==8){key=0;};
		if (keys[4]==16){key=100;}; //enter
		//mouse event
		if (!(keys[1]+keys[2]+keys[3]+keys[4])){key=100;}
		//while(keys[1]+keys[2]+keys[3]+keys[4]){XQueryKeymap(disp,keys);}
		
return key;
}



void draw_button(int x,int y,int size,char *title)
{
int ll;

set_x_color(0.8,0.8,0.9);
XFillRectangle(disp,pixmap,gc,x,y,size,16);
set_x_color(0.0,0.0,0.0);
XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+size/2-ll/2,y+13,title);
}

void mask_button(int x,int y,int size,char *title) //display button inactive
{
int ll;

set_x_color(0.85,0.85,0.85);
XFillRectangle(disp,pixmap,gc,x,y,size,16);
set_x_color(0.5,0.5,0.5);
XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+size/2-ll/2,y+13,title);
}

void draw_radio(int x,int y,int size,char *title,int status)
{
int ll;

//set_x_color(0.9,0.9,0.9);
//XFillRectangle(disp,pixmap,gc,x,y,size,16);
set_x_color(1.0,1.0,1.0);
//XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(title,font2);
XSetFont(disp,gc,font1->fid);
draw_text(x+2,y+11,title);
set_x_color(0.0,0.0,0.0);
XFillArc(disp,pixmap,gc,x+size-14,y,10,10,0,360*64);
//XFillRectangle(disp,pixmap,gc,x+size-14,y+3,10,10);
set_x_color(1.0,1.0,1.0);
XDrawArc(disp,pixmap,gc,x+size-14,y,10,10,0,360*64);
//XDrawRectangle(disp,pixmap,gc,x+size-14,y+3,10,10);
if (status)
//    {XFillRectangle(disp,pixmap,gc,x+size-12,y+5,7,7);}
{
    XFillArc(disp,pixmap,gc,x+size-12,y+2,6,6,0,360*64);
}
//set_x_color(0.9,0.9,0.9);
//draw_text(x+124,y+13,text);


}



void draw_switch(int x,int y,int size,char *title,int status)
{
int ll;

if (0){set_x_color(0.8,0.8,0.9);}else{set_x_color(0.6,0.6,0.6);}
XFillRectangle(disp,pixmap,gc,x,y,size-20,16);
set_x_color(0.0,0.0,0.0);
XDrawRectangle(disp,pixmap,gc,x,y,size-20,16);
if (status){set_x_color(0.99,0.99,0.99);}else{set_x_color(0.2,0.2,0.2);}
ll=text_length(title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+2,y+13,title);
if (status){set_x_color(0.99,0.99,0.0);}else{set_x_color(0.8,0.8,0.8);}
XFillArc(disp,pixmap,gc,x+size-14,y+2,10,10,0,360*64);
//XFillRectangle(disp,pixmap,gc,x+size-14,y+3,10,10);
if (status){set_x_color(0.99,0.99,0.99);}else{set_x_color(0.7,0.7,0.7);}
XDrawArc(disp,pixmap,gc,x+size-14,y+2,10,10,0,360*64);
//XDrawRectangle(disp,pixmap,gc,x+size-14,y+3,10,10);
//if (status)
//    {XFillRectangle(disp,pixmap,gc,x+size-12,y+5,7,7);}
//{
//    XFillArc(disp,pixmap,gc,x+size-12,y+2,8,8,0,360*64);
//}
//set_x_color(0.9,0.9,0.9);
//draw_text(x+124,y+13,text);


}



void draw_input(int x,int y,int size,char *title,int value)
{
int ll;
char napis[6];
//set_x_color(0.9,0.9,0.9);
//XFillRectangle(disp,pixmap,gc,x,y,size,16);
set_x_color(0.0,0.0,0.0);
//XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+2,y+13,title);
set_x_color(1.0,1.0,1.0);
XFillRectangle(disp,pixmap,gc,x+size-44,y,44,14);
set_x_color(0.0,0.0,0.0);
XDrawRectangle(disp,pixmap,gc,x+size-44,y,44,14);
sprintf(napis,"%5d",value);
draw_text(x+size-42,y+12,napis);
//set_x_color(0.9,0.9,0.9);
//draw_text(x+124,y+13,text);


}

void draw_input_text(int x,int y,int size,char *title,char *value)
{
int ll;
char napis[5];
//set_x_color(0.9,0.9,0.9);
//XFillRectangle(disp,pixmap,gc,x,y,size,16);
set_x_color(0.0,0.0,0.0);
//XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+2,y+13,title);
set_x_color(1.0,1.0,1.0);
XFillRectangle(disp,pixmap,gc,x+size-44,y,44,14);
set_x_color(0.0,0.0,0.0);
XDrawRectangle(disp,pixmap,gc,x+size-44,y,44,14);
//sprintf(napis,"%5d",value);
draw_text(x+size-42,y+12,value);
//set_x_color(0.9,0.9,0.9);
//draw_text(x+124,y+13,text);


}

void add_button(int n,int x,int y,int size,char *title)
{
int ll;
if (n>buttons_num){buttons_num=n;}
buttons[n].x1=x;
buttons[n].y1=y;
buttons[n].x2=x+size;
buttons[n].y2=y+16;
buttons[n].active=1;
sprintf(buttons[n].title,"%s",title);
}

void press_button(int n)
{
int x,y,size,ll;
  size=buttons[n].x2-buttons[n].x1;
  x=buttons[n].x1;
  y=buttons[n].y1;
  set_x_color(0.5,0.5,0.5);
  XFillRectangle(disp,pixmap,gc,x+2,y+2,size-3,13);
  XAllocColor(disp,cm,&colmem);
  XSetForeground(disp,gc,colmem.pixel);
  set_x_color(0.0,0.0,0.0);
//XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(buttons[n].title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+size/2-ll/2,y+13,buttons[n].title);
}

void draw_n_button(int n)
{
int x,y,size,ll;

x=buttons[n].x1;
y=buttons[n].y1;
size=buttons[n].x2-buttons[n].x1;
set_x_color(0.8,0.8,0.9);
XFillRectangle(disp,pixmap,gc,x,y,size,16);
set_x_color(0.0,0.0,0.0);
XDrawRectangle(disp,pixmap,gc,x,y,size,16);
ll=text_length(buttons[n].title,font2);
XSetFont(disp,gc,font2->fid);
draw_text(x+size/2-ll/2,y+13,buttons[n].title);
}

void add_radio_button(int n,int x,int y,int sizex,int sizey,char *title)
{
int ll;
if (n>buttons_num){buttons_num=n;}
buttons[n].x1=x;
buttons[n].y1=y;
buttons[n].x2=x+sizex;
buttons[n].y2=y+sizey;
buttons[n].active=1;

}


int check_button(int x, int y)
{
int i,b;
b=0;
for (i=1;i<=buttons_num;i++)
{
    if ((buttons[i].active)&&(x>buttons[i].x1)&&(x<buttons[i].x2)&&(y>buttons[i].y1)&&(y<buttons[i].y2))
    {b=i;}    
}
//printf("BUTTON %d\n",b);
return b;
}

void info()
{

}




//###################### MAIN ######################


void sim_redraw()
{
set_x_color(0.2,0.25,0.2);
XFillRectangle(disp,pixmap,gc,S1X-10,SY-10,421,152);
set_x_color(0.0,0.0,0.0);
XFillRectangle(disp,pixmap,gc,S1X-4,SY-4,176+8,132+8);
XFillRectangle(disp,pixmap,gc,S2X-4,SY-4,176+8,132+8);
  
set_x_color(0.7,0.7,0.7);
//XDrawRectangle(disp,pixmap,gc,0,0,176,140);

pixmap_x_sync();

}



void simconfig()
{
char napis[100];
Window r,c;
int	rx,ry,wx,wy,m;
int i,ll,key;
char rb[8];

XEvent	event;
    
    
    //set_x_color(1.0,1.0,1.0);
    set_x_color(0.0,0.0,0.0);
    XFillRectangle(disp,pixmap,gc,10,180,410,70);
    set_x_color(1.0,1.0,1.0);
    XDrawRectangle(disp,pixmap,gc,11,181,408,68);
    XSetFont(disp,gc,font1->fid);
    //set_x_color(0.0,0.0,0.0);
    set_x_color(1.0,1.0,1.0);
    draw_text(20,195,"Configuration");
    //draw_text(6,17,"nanoNET TRX Station v.1.2");    
    //XSetFont(disp,gc,font1->fid);
    //draw_text(5,30,"This program is a part of nanoTRX demonstration kit");
    //draw_text(35,560,"Software by T.Kuehn (C)Selfco 2004");

    //XDrawRectangle(disp,pixmap,gc,10,50,220,220);
    //XFillRectangle(disp,pixmap,gc,10,50,220,20);
    //ll=text_length("Configuration",font1);
    //set_x_color(0.9,0.9,0.9);
    //draw_text(120-ll/2,65,"Configuration");
    
    //set_x_color(0.0,0.0,0.0);
    //XDrawRectangle(disp,pixmap,gc,260,50,220,220);
    //XFillRectangle(disp,pixmap,gc,260,50,220,20);
    //set_x_color(0.9,0.9,0.9);
    //ll=text_length("Station mode",font1);
    //draw_text(375-ll/2,65,"Station mode");

    //set_x_color(0.0,0.0,0.0);
    //draw_text(10,315,"Receiver window");
    //draw_text(10,435,"Transmitter window");
    //set_x_color(0.99,0.99,0.99);
    //XFillRectangle(disp,pixmap,gc,10,320,470,100);
    //XFillRectangle(disp,pixmap,gc,10,440,470,100);
    //draw_button(300,220,40,"Config");
    draw_button(370,230,40,"OK");
    //draw_switch(20,200,80,"switch",1);
    add_radio_button(12,120,185,80,12,"");
    add_radio_button(13,120,200,80,12,"");
    add_radio_button(14,120,215,80,12,"");
    add_radio_button(15,120,230,80,12,"");    
    key=0;
    for(i=0;i<8;i++){rb[i]=0;}
    rb[aprs_packet_source]=1;
    draw_radio(120,185,80,"No packets",rb[0]);
    draw_radio(120,200,80,"Simulate",rb[1]);
    draw_radio(120,215,80,"Server",rb[2]);
    draw_radio(120,230,80,"File",rb[3]);
    draw_radio(220,185,90,"Serial modem",rb[4]);
    draw_radio(220,200,90,"Remote modem",rb[5]);
    pixmap_x_sync();
    while(key==0)
    {
    if(spirequest>0){spi_request_handler();}
    XSync(disp,0);
    //pixmap_x_sync();
    i=XEventsQueued(disp,QueuedAlready);
    /*if (XCheckIfEvent(disp,window,ButtonPressMask,&event))
    */
    if(i>0){
	XMaskEvent(disp,ButtonPressMask,&event);
	XQueryPointer(disp,window,&r,&c,&rx,&ry,&wx,&wy,&m);
	
	i=check_button(wx,wy);
	switch(i){
	case 11: key=1;break; //exit
	case 12: aprs_packet_source=0;break;
	case 13: aprs_packet_source=1;break;
	//case 14: aprs_packet_source=2;it_connect();break;
	case 15: aprs_packet_source=3;aprs_rx_fopen();break;
	}
	for(i=0;i<8;i++){rb[i]=0;}
    rb[aprs_packet_source]=1;
    draw_radio(120,185,80,"No packets",rb[0]);
    draw_radio(120,200,80,"Simulate",rb[1]);
    draw_radio(120,215,80,"Server",rb[2]);
    draw_radio(120,230,80,"File",rb[3]);
    draw_radio(220,185,90,"Serial modem",rb[4]);
    draw_radio(220,200,90,"Remote modem",rb[5]);
	pixmap_x_sync();
    }

    }
    //restore view
    set_x_color(0.0,0.0,0.0);
    XFillRectangle(disp,pixmap,gc,10,180,410,70);
    draw_button(20,180,40,"Opt");
    	draw_button(120,180,40,"Up");
    	draw_button(220,180,40,"Disp");
    	draw_button(20,200,40,"<-");
    	draw_button(120,200,40,"Ent");
    	draw_button(220,200,40,"->");
    	draw_button(20,220,40,"Menu");
    	draw_button(120,220,40,"Dn");
    	draw_button(220,220,40,"Shift");
    	draw_button(320,230,60,"Config");
	buttons_num=11;
}

static int keymem=0;


int get_x_key()
{
int ll,i,b;
Window r,c;
int	rx,ry,wx,wy,m;
int mx,my;
XEvent	event;
int key;

    key=0;
    //pixmap_x_sync();
    i=XEventsQueued(disp,QueuedAlready);
    /*if (XCheckIfEvent(disp,window,ButtonPressMask,&event))
    */
    if(keymem){
     //draw_n_button(keymem);
     //keymem=0;
     //pixmap_x_sync();
    }
    if (i>0)
    {
	XMaskEvent(disp,ButtonPressMask,&event);
	XQueryPointer(disp,window,&r,&c,&rx,&ry,&wx,&wy,&m);
	
	b=check_button(wx,wy);
	//press_button(b); keymem=b;
	//pixmap_x_sync();
	//printf("%d %d - %d\n",wx,wy,b);
	if(b>9){
	/* operacja w symulatorze */
	key=0;
	if(b==10){simconfig();}
	}
	else
	{
	key=b;
	}
    }
    return(key);
}



void panel_init()
{
int i;

    printf("Open\n");
    open_x_window();
    printf("Clear\n");
    clear_x_window();
    printf("XSelect\n");
    sleep(3);
    XSelectInput(disp,window,ButtonPressMask|KeyPressMask);
    sleep(3);
/*
    font1=XLoadQueryFont(disp,"f8x8.h");
    if (!(font1)){printf("No fonts1!\n");exit(1);}
    font2=XLoadQueryFont(disp,"f8x8");
    if (!(font2)){printf("No fonts2!\n");exit(1);}
    font3=XLoadQueryFont(disp,"f9x14");
    if (!(font3)){font3=XLoadQueryFont(disp,"9x15");}//alternate
    if (!(font3)){printf("No fonts3!\n");exit(1);}
*/
    font1=XLoadQueryFont(disp,"5x8");
    font2=font1;
    font3=font1;

    for(i=0;i<100;i++){buttons[i].active=0;}
    buttons_num=11; 
    add_button(1,20,180,40,"Opt");	draw_button(20,180,40,"Opt");
    add_button(2,120,180,40,"Up");	draw_button(120,180,40,"Up");
    add_button(3,220,180,40,"Disp");	draw_button(220,180,40,"Disp");
    add_button(4,20,200,40,"<-");	draw_button(20,200,40,"<-");
    add_button(5,120,200,40,"Ent");	draw_button(120,200,40,"Ent");
    add_button(6,220,200,40,"->");	draw_button(220,200,40,"->");
    add_button(7,20,220,40,"Menu");	draw_button(20,220,40,"Menu");
    add_button(8,120,220,40,"Dn");	draw_button(120,220,40,"Dn");
    add_button(9,220,220,40,"Shift");	draw_button(220,220,40,"Shift");
    add_button(10,320,230,60,"Config");	draw_button(320,230,60,"Config");
    add_button(11,370,230,40,"OK");
    //add_button(4,10,280,90,"Pause TX");
    //add_button(5,110,280,90,"Pause RX");
    //add_button(6,210,280,90,"Pause RP");
    //add_button(7,350,280,90,"TX");
    //add_button(8,250,550,90,"Quit");
    set_x_color(1.0,1.0,1.0);
    XSetFont(disp,gc,font3->fid);
    draw_text(5,20,"APRS Navigator simulator");
    draw_text(5,21,"APRS Navigator simulator");
}

void simled(int x, int y,float color1, float color2, float color3)
{
    set_x_color(color1,color2,color3);
    XFillArc(disp,pixmap,gc,x,y,12,12,0,360*64);
    set_x_color(0.0,0.0,0.0);
    XDrawArc(disp,pixmap,gc,x,y,12,12,0,360*64);
}


void status(int s1,int s2,int s3,int s4,int s5,char *text)
{
int ll;
char napis[100];

    XSetFont(disp,gc,font2->fid);
    set_x_color(0.7,0.7,0.7);
    XFillRectangle(disp,pixmap,gc,7,50,626,17);
    set_x_color(0.0,0.0,0.0);
    XDrawRectangle(disp,pixmap,gc,7,50,626,60);
    sprintf(napis,"Connection status");
    ll=text_length(napis,font2);
    draw_text(320-ll/2,63,napis);
    draw_text(320-ll/2+1,63,napis);
    if (s1){simled(20,74,1.0,1.0,0.0);}else{simled(20,74,0.8,0.8,0.8);}
    draw_text(40,86,"WAITING FOR CONNECTION");
    if (s2){simled(220,74,0.0,1.0,0.0);}else{simled(220,74,0.8,0.8,0.8);}
    draw_text(240,86,"CONNECTED");
    if (s3)
	{simled(340,74,0.0,1.0,0.0);
	draw_text(360,86,"SYNCHRONIZED");
	}
	else
	{simled(340,74,1.0,0.0,0.0);
	draw_text(360,86,"NOT SYNCHRONIZED");
	}
    XSetFont(disp,gc,font1->fid);
    if (s4){simled(510,74,1.0,0.0,0.0);}else{simled(510,74,0.8,0.8,0.8);}
    draw_text(530,84,"TRX ERROR");
    if (s5){simled(510,92,1.0,1.0,0.0);}else{simled(510,92,0.8,0.8,0.8);}
    draw_text(530,102,"INTERFERENCE");
    XSetFont(disp,gc,font1->fid);
    draw_text(20,105,"Module information:");
    draw_text(150,105,text);
}


void panel()
{
char napis[100];
Window r,c;
int	rx,ry,wx,wy,m;
int i,ll;

XEvent	event;
 //getperm();
/*
buttons_num=2; 
//add_button(1,300,340,40,"OK");
//add_button(2,5,455,20,"?");

    open_x_window();
    clear_x_window();
XSelectInput(disp,window,ButtonPressMask);

font1=XLoadQueryFont(disp,"6x10");
font2=XLoadQueryFont(disp,"7x14");
font3=XLoadQueryFont(disp,"9x18");
*/
//XMaskEvent(disp, ButtonPressMask, &event);
    
    
    set_x_color(0.8,0.8,0.8);
    clear_x_window();
    
    draw_button(5,550,20,"?");
    draw_button(380,550,90,"Quit");
    draw_button(10,280,90,"Pause TX");
    draw_button(110,280,90,"Pause RX");
    draw_button(210,280,90,"Pause RP");
    draw_button(350,280,90,"TX!");
    draw_button(250,550,90,"Clear");
    
    set_x_color(1.0,1.0,1.0);
    //XFillRectangle(disp,pixmap,gc,5,145,630,200);
    //XFillRectangle(disp,pixmap,gc,5,360,630,90);
    XSetFont(disp,gc,font3->fid);
    set_x_color(0.0,0.0,0.0);
    draw_text(5,17,"nanoNET TRX Station v.1.2");
    draw_text(6,17,"nanoNET TRX Station v.1.2");    
    XSetFont(disp,gc,font1->fid);
    draw_text(5,30,"This program is a part of nanoTRX demonstration kit");
    draw_text(35,560,"Software by T.Kuehn (C)Selfco 2004");

    XDrawRectangle(disp,pixmap,gc,10,50,220,220);
    XFillRectangle(disp,pixmap,gc,10,50,220,20);
    ll=text_length("Configuration",font1);
    set_x_color(0.9,0.9,0.9);
    draw_text(120-ll/2,65,"Configuration");
    
    set_x_color(0.0,0.0,0.0);
    XDrawRectangle(disp,pixmap,gc,260,50,220,220);
    XFillRectangle(disp,pixmap,gc,260,50,220,20);
    set_x_color(0.9,0.9,0.9);
    ll=text_length("Station mode",font1);
    draw_text(375-ll/2,65,"Station mode");

    set_x_color(0.0,0.0,0.0);
    draw_text(10,315,"Receiver window");
    draw_text(10,435,"Transmitter window");
    set_x_color(0.99,0.99,0.99);
    XFillRectangle(disp,pixmap,gc,10,320,470,100);
    XFillRectangle(disp,pixmap,gc,10,440,470,100);
}









#define LCD_CS_SET 0x90000
//P0.16,P0.19 - both lcd's
#define LCD_RESET  0x20000
//P0.17
#define LCD_RS	   0x40000
//P0.18

//#define LCD_MOSI   0x20 //PB5
//#define LCD_MISO   0x40 //PB6
//#define LCD_SCK    0x80 //PB7

void LCDselect(unsigned long id)
{
int sid;
  
  if(id==1){sid=1;}
  if(id==2){sid=2;}
  if(id==3){sid=3;}
  //if((id==1)||(id==2)){id=3-id;}
  lcds=sid;
  //lcds=(((id & 0x01)<<16)|((id & 0x02)<<18));
  //if(id==0){lcds=0x10000;}
  //else{lcds=0x80000;}
}

void lcd_wrcmd(uint8_t d)
{
  
}

void lcd_wrdata(uint8_t d)
{

}

void lcd_wrcmd16(uint16_t dat)
{
  
}

void lcd_wrdat16(uint16_t dat)
{
  
}

const uint8_t init1[]={
0xFD, 0xFD, 0xFD, 0xFD,
0xEF, 0x00, 0xEE, 0x04, 0x1B, 0x04, 0xFE, 0xFE, 0xFE, 0xFE, 0xEF, 0x90, 0x4A, 0x04,
0x7F, 0x3F, 0xEE, 0x04, 0x43, 0x06};

const uint8_t init2[]={
0xEF, 0x90,  0x09, 0x83,  0x08, 0x00,  0x0B, 0xAF,  0x0A, 0x00,  0x05, 0x00,  0x06, 0x00,
0x07, 0x00,  0xEF, 0x00,  0xEE, 0x0C,  0xEF, 0x90,  0x00, 0x80,  0xEF, 0xB0,  0x49, 0x02,
0xEF, 0x00,  0x7F, 0x01,  0xE1, 0x81,  0xE2, 0x02,  0xE2, 0x76,  0xE1, 0x83};



void initLCD()
{
char c;

//printf("init lcd\n");
//open_x_window();
panel_init();
//panel();
pixmap_x_sync();

sim_redraw();

//printf("...init lcd\n");
}


void LCDclear()
{
set_x16_color(LCDcolor_background);
if((lcds==1)||(lcds==3))XFillRectangle(disp,pixmap,gc,S1X,SY,176,132);
if((lcds==2)||(lcds==3))XFillRectangle(disp,pixmap,gc,S2X,SY,176,132);
  	

}


const char _chgen[]={	//636
0,0,0,0,0,0,		  	   	   //spacja
0,0,111,111,0,0,			   //!
14,7,0,14,7,0,				   //"
36,126,36,126,36,0,			   //#
36,74,255,82,36,0,			   //$
35,19,8,100,98,0,			   //%
54,73,89,38,80,0,			   //&
0,0,7,0,0,0,			   	   //'
28,62,99,65,0,0,			   //(
0,65,99,62,28,0,			   //)
36,24,126,24,36,0,			   //*
8,8,62,8,8,0,			   //+
0,0,48,112,0,0,				   //,
24,24,24,24,24,0,			   //-
0,0,96,96,0,0,				   //.
48,24,12,6,3,0,				   ///
62,115,107,103,62,0,		   //0
4,6,127,127,0,0,			   //1
102,119,123,111,102,0,		   //2
34,99,107,127,54,0,			   //3
24,28,30,123,121,0,			   //4
47,111,109,125,57,0,		   //5
62,127,109,125,56,0,		   //6
99,115,27,15,7,0,			   //7
54,127,107,127,54,0,		   //8
38,111,75,127,62,0,			   //9
0,54,54,0,0,0,				   //:
64,118,54,0,0,0,			   //;
8,28,54,99,0,0,				   //<
108,108,108,108,108,0,		   //=
0,99,54,28,8,0,				   //>
2,105,109,7,2,0,			   //?
62,65,93,94,16,0,			   //@
126,127,27,127,126,0,	   	   //A
127,127,107,127,54,0,	 	   //B
62,127,99,99,34,0,			   //C
127,127,99,127,62,0,		   //D
127,127,107,107,99,0,		   //E
127,127,27,27,3,0,			   //F
62,127,99,107,58,0,			   //G
127,127,24,127,127,0,		   //H
0,127,127,0,0,0,			   //I
48,112,96,127,63,0,			   //J
127,127,28,54,99,0,			   //K
127,127,96,96,96,0,			   //L
127,6,12,6,127,0,			   //M
127,127,12,24,127,0,		   //N
62,127,99,127,62,0,			   //O
127,127,27,31,14,0,			   //P
30,63,51,127,110,0,			   //Q
127,127,51,127,110,0,		   //R
102,111,107,123,51,0,		   //S
0,3,127,127,3,0,			   //T
63,127,96,127,63,0,			   //U
31,63,112,63,31,0,			   //V
127,48,24,48,127,0,			   //W
99,119,28,119,99,0,			   //X
7,15,124,127,7,0,			   //Y
99,115,123,111,103,0,		   //Z
127,127,99,99,0,0,			   //[
3,6,12,24,48,0,				   //backslash
0,99,99,127,127,0,			   //]
12,6,3,6,12,0,				   //^
96,96,96,96,96,0,			   //_
0,1,3,2,0,0,				   //`
126,127,27,127,126,0,	   	   //A - tu powinny byc male litery
127,127,107,127,54,0,	 	   //B
62,127,99,99,34,0,			   //C
127,127,99,127,62,0,		   //D
127,127,107,107,99,0,		   //E
127,127,27,27,3,0,			   //F
62,127,99,107,58,0,			   //G
127,127,24,127,127,0,		   //H
0,127,127,0,0,0,			   //I
48,112,96,127,63,0,			   //J
127,127,28,54,99,0,			   //K
127,127,96,96,96,0,			   //L
127,6,12,6,127,0,			   //M
127,127,12,24,127,0,		   //N
62,127,99,127,62,0,			   //O
127,127,27,31,14,0,			   //P
30,63,51,127,110,0,			   //Q
127,127,51,127,110,0,		   //R
102,111,107,123,51,0,		   //S
0,3,127,127,3,0,			   //T
63,127,96,127,63,0,			   //U
31,63,112,63,31,0,			   //V
127,48,24,48,127,0,			   //W
99,54,28,54,99,0,			   //X
7,15,124,127,7,0,			   //Y
99,115,123,111,103,0,		   //Z
8,62,119,65,0,0,			   //{
0,127,127,0,0,0,			   //|
0,65,119,62,8,0,			   //}
20,28,62,28,20,0,	  	       //digi (126)
124,70,119,118,124,0,	       //dom
24,60,28,60,24,0,	  	   	   //car
0,62,28,8,0,0,		  	       //trojkat
127,99,85,73,127,0,			   //koperta
2,100,127,100,2,0,			   //antena
124,124,116,119,127,0,		   //radio (132)
7,82,33,82,7,0,				   //WX
94,64,94,74,78,0,			   // IP
16,63,127,63,16,0,			   //strzalka dol
4,126,127,126,4,0,			   //strzalka gora
8,12,14,12,8,0,				   //pozycja-trojkat 137
21,19,51,85,51,0,			   //RP
95,0,65,31,65,0,			   //IT
28,8,62,8,127,0,				   //in range
0x1C, 0x22, 0x4F, 0x51, 0x22, 0x1C, 0
};


const static unsigned char small[]={
62,34,34,34,62,		  	       //box (33)
62,54,42,54,62,		  	   	   //box checked (34)
62,28,8,0,0,				   //trojkat
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,
0,0,0,0,0,	 		   	   	   //
0,0,0,0,0,
0,0,0,0,0,
20,8,20,0,0,				   //*
8,28,8,0,0,					   //+
48,0,0,0,0,					   //,
8,8,8,0,0,					   //-
32,0,0,0,0,					   //.
32,24,4,0,0,				   ///
62,34,62,0,0,				   //0
36,62,32,0,0,				   //1
58,42,46,0,0,				   //2
42,42,62,0,0,				   //3
14,8,60,0,0,				   //4
46,42,58,0,0,				   //5
62,42,58,0,0,				   //6
50,10,6,0,0,				   //7
62,42,62,0,0,		  	   	   //8	
46,42,62,0,0,		  	   	   //9
20,0,0,0,0,					   //:
20,52,0,0,0,				   //;
8,20,34,0,0,				   //<
20,20,20,0,0,				   //
34,20,8,0,0,				   //>
4,82,12,0,0,				   //?
28,34,42,12,0,				   //@
60,18,60,0,0,		  	   	   //A
62,42,20,0,0,				   //B
28,34,34,0,0,				   //C
62,34,28,0,0,				   //D
62,42,34,0,0,				   //E
62,10,2,0,0,				   //F
28,34,42,56,0,				   //G
62,8,62,0,0,				   //H
62,0,0,0,0,					   //I
18,34,30,0,0,				   //J
62,20,34,0,0,				   //K
62,32,32,0,0,				   //L
62,4,8,4,62,				   //M
62,8,16,62,0,				   //N
28,34,34,28,0,				   //O
62,10,4,0,0,				   //P
28,34,18,44,0,				   //Q
62,10,52,0,0,				   //R
36,42,42,18,0,				   //S
2,62,2,0,0,					   //T
30,32,32,30,0,				   //U
30,32,30,0,0,				   //V
62,16,8,16,62,				   //W
54,8,54,0,0,				   //X
6,56,6,0,0,					   //Y
50,42,38,0,0				   //Z	
};

/*
const unsigned char ascii_tab[96][14]={
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space (32)
{ 0x00, 0x00, 0x00, 0x18, 0x3c, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, //!
{ 0x00, 0x66, 0x66, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //"
{ 0x00, 0x00, 0x00, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c, 0x00, 0x00}, //#
{ 0x00, 0x18, 0x18, 0x7c, 0xc6, 0xc2, 0xc0, 0x7c, 0x06, 0x86, 0xc6, 0x7c, 0x18, 0x18}, //$
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0xc2, 0xc6, 0x0c, 0x18, 0x30, 0x66, 0xc6, 0x00, 0x00}, // %
{ 0x00, 0x00, 0x00, 0x38, 0x6c, 0x6c, 0x38, 0x76, 0xdc, 0xcc, 0xcc, 0x76, 0x00, 0x00}, // &
{ 0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
{ 0x00, 0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00, 0x00}, // (
{ 0x00, 0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00, 0x00}, // )
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x00}, // *
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00}, // +
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x30, 0x00}, // Ž
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // -
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00}, // .
{ 0x00, 0x00, 0x00, 0x02, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x80, 0x00, 0x00, 0x00}, // /
{ 0x00, 0x00, 0x00, 0x38, 0x6c, 0xc6, 0xc6, 0xd6, 0xc6, 0xc6, 0x6c, 0x38, 0x00, 0x00}, // 0 (48-32)
{ 0x00, 0x00, 0x00, 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc6, 0xfe, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0x06, 0x06, 0x3c, 0x06, 0x06, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x0c, 0x1c, 0x3c, 0x6c, 0xcc, 0xfe, 0x0c, 0x0c, 0x1e, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfe, 0xc0, 0xc0, 0xc0, 0xfc, 0x06, 0x06, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x38, 0x60, 0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfe, 0xc6, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x06, 0x0c, 0x78, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0x0c, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xde, 0xde, 0xde, 0xdc, 0xc0, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x66, 0xfc, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x3c, 0x66, 0xc2, 0xc0, 0xc0, 0xc0, 0xc2, 0x66, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xf8, 0x6c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6c, 0xf8, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfe, 0x66, 0x62, 0x68, 0x78, 0x68, 0x62, 0x66, 0xfe, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfe, 0x66, 0x62, 0x68, 0x78, 0x68, 0x60, 0x60, 0xf0, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x3c, 0x66, 0xc2, 0xc0, 0xc0, 0xde, 0xc6, 0x66, 0x3a, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xcc, 0xcc, 0x78, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xe6, 0x66, 0x6c, 0x6c, 0x78, 0x6c, 0x6c, 0x66, 0xe6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xf0, 0x60, 0x60, 0x60, 0x60, 0x60, 0x62, 0x66, 0xfe, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xee, 0xfe, 0xd6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xe6, 0xf6, 0xfe, 0xde, 0xce, 0xc6, 0xc6, 0xc6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0xf0, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xd6, 0xde, 0x7c, 0x0e, 0x00},
{ 0x00, 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x6c, 0x66, 0x66, 0xe6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0x60, 0x38, 0x0c, 0xc6, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x5a, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x10, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xd6, 0xd6, 0xfe, 0x6c, 0x6c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x7c, 0x38, 0x7c, 0xc6, 0xc6, 0xc6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xfe, 0xc6, 0x8c, 0x18, 0x30, 0x60, 0xc2, 0xc6, 0xfe, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x06, 0x02, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x00},
{ 0x10, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff},
{ 0x00, 0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0c, 0x7c, 0xcc, 0xcc, 0x76, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0xe0, 0x60, 0x60, 0x78, 0x6c, 0x66, 0x66, 0x66, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x1c, 0x0c, 0x0c, 0x3c, 0x6c, 0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xfe, 0xc0, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x1c, 0x36, 0x32, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0xcc, 0x78},
{ 0x00, 0x00, 0x00, 0xe0, 0x60, 0x60, 0x6c, 0x76, 0x66, 0x66, 0x66, 0xe6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3c},
{ 0x00, 0x00, 0x00, 0xe0, 0x60, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0xe6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0xfe, 0xd6, 0xd6, 0xd6, 0xd6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xf0},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0x0c, 0x1e},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x76, 0x66, 0x60, 0x60, 0xf0, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0x70, 0x1c, 0xc6, 0x7c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x10, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x36, 0x1c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x10, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xd6, 0xd6, 0xfe, 0x6c, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x38, 0x6c, 0xc6, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x0c, 0x78},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xcc, 0x18, 0x30, 0x66, 0xfe, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x0e, 0x18, 0x18, 0x18, 0x70, 0x18, 0x18, 0x18, 0x0e, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x18, 0x70, 0x00, 0x00},
{ 0x00, 0x76, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0x00, 0x00, 0x00},
};

*/
char ascii_tab[1][1];

/*

void chargen3(char c)
{

  uint8_t h,ch,p,mask;
  char x,y;

  x=cursorx;
  y=cursory;

  lcd_wrcmd16(0xEF90);

  if (0)
  {
    lcd_wrcmd16(0x0500);
    lcd_wrcmd16(0x0800+x);
    lcd_wrcmd16(0x0A00+y);
    lcd_wrcmd16(0x0900+x+CHAR_W-1);
    lcd_wrcmd16(0x0B00+y+CHAR_H-1);
  }
  else
  {
    lcd_wrcmd16(0x0504);
    lcd_wrcmd16(0x0800+y);
    lcd_wrcmd16(0x0A00+x);
    lcd_wrcmd16(0x0900+y+CHAR_H-1);
    lcd_wrcmd16(0x0B00+x+CHAR_W-1);
  }

  for (h=0; h<CHAR_H; h++) // every column of the character
  {
        if (0)
          ch=ascii_tab[ c-32 ][h];	
	else
          ch=ascii_tab[ c-32 ][CHAR_H-h-1];
        mask=0x80;
        for (p=0; p<CHAR_W; p++)  // write the pixels
        {
          if (ch&mask)
          {
            lcd_wrdat16(LCDcolor_foreground);
          }
          else
          {
            lcd_wrdat16(LCDcolor_background);
          }
          mask=mask/2;
        }  // for p
  }
  cursorx=cursorx+CHAR_W;

}
*/

void chargen(unsigned char c)
{

int i,k;
char ch,mask,p;
int ofsx,ofsy;

    if(lcds==2){ofsx=S2X;}else{ofsx=S1X;}
    ofsy=SY;
    
    set_x16_color(LCDcolor_foreground);
	//PORTB&=~LCD_CS;
	//IO0CLR=lcds;
    //PORTB&=~LCD_RS;
	//IO0CLR=LCD_RS;
  i=c;
  if ((i<32)||(i>141)){i=46;}

 i=i-32;
 i=i*6;
 for(k=0;k<6;k++){
   ch=_chgen[i+k];
  mask=0x80;
        for (p=0; p<8; p++)  // write the pixels
        {
          if (ch&mask)
          {
	  set_x16_color(LCDcolor_foreground);
	  if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
	  
		    //S0SPDR=LCDcolor_foreground>>8;
	//udelay();
    //while(!(S0SPSR & SPIF));//wait send
	//S0SPDR=LCDcolor_foreground;
	//udelay();
    //while(!(S0SPSR &(SPIF)));//wait send
            //lcd_wrdat16(LCDcolor_foreground);
          }
          else
          {
	  set_x16_color(LCDcolor_background);
	  if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
//		  S0SPDR=LCDcolor_background>>8;
	//udelay();
//    while(!(S0SPSR &(SPIF)));//wait send
//	S0SPDR=LCDcolor_background;
	//udelay();
//    while(!(S0SPSR &(SPIF)));//wait send
            //lcd_wrdat16(LCDcolor_background);
          }
          mask=mask/2;
        }  // for p
		cursorx++;
  }
  for(p=0;p<8;p++){
          set_x16_color(LCDcolor_background);
	  if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
  // S0SPDR=LCDcolor_background>>8;
	//udelay();
    //while(!(S0SPSR &(SPIF)));//wait send
//	S0SPDR=LCDcolor_background;
	//udelay();
//    while(!(S0SPSR &(SPIF)));//wait send
   //lcd_wrdat16(LCDcolor_background);
   }
  //PORTB|=LCD_CS;
  //IO0SET=LCD_CS_SET;

  cursorx++;
  if(spirequest>0){spi_request_handler();}
  //pixmap_x_sync();
}

void smchargen(unsigned char c)
{

int i,k;
char ch,mask,p;
int ofsx,ofsy;
    
    if(lcds==2){ofsx=S2X;}else{ofsx=S1X;}
    ofsy=SY;
    
    set_x16_color(LCDcolor_foreground);
	
i=c;
if ((i<32)||(i>136)){i=46;};
if (i>90){i=i-32;}			//male litery na duze
i=i-33;
i=i*5;
k=0;
if(c==32)
{
  for(p=0;p<7;p++){
	    set_x16_color(LCDcolor_background);
	    if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
  }
  cursorx=cursorx+1;
  for(p=0;p<7;p++){
	    set_x16_color(LCDcolor_background);
	    if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
  }
  cursorx=cursorx+1;
}
else
{
while((small[i+k]>0)&&(k<5)){

 ch=small[i+k]*2;
  mask=0x80;
        for (p=0; p<7; p++)  // write the pixels
        {
          if (ch&mask)
          {
            set_x16_color(LCDcolor_foreground);
	    if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
          }
          else
          {
            set_x16_color(LCDcolor_background);
	    if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
          }
          mask=mask/2;
        }  // for p
		cursorx++;
 k=k+1;
}
for(p=0;p<7;p++){
	    set_x16_color(LCDcolor_background);
	    if(cursorx<176)XDrawPoint(disp,pixmap,gc,cursorx+ofsx,131-cursory+ofsy-p);
}
cursorx++;
}
}





void LCDyx(char y, char x)
{
  cursory=y;
  cursorx=x;


}



void setPixel(int x, int y)
{
int x1,y1;

  //if(x>175){return;}
  if(lcds==2){x=x+S2X;}else{x=x+S1X;}
  //y=(131-y)+10;
  y=(131+SY)-y;
  set_x16_color(LCDcolor_foreground);
  XDrawPoint(disp,pixmap,gc,x,y);
  
  //XSync(disp,0);
  //pixmap_x_sync();
}

void setPixelgv(int x, int y, char r, char g, char b)
{
 LCDcolor_foreground=(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F));
 setPixel(x,y);
 r=r/2; g=g-10; b=b/2;
 LCDcolor_foreground=(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F));
 setPixel(x,y-1);
 setPixel(x,y+1);
}

void setPixelgh(int x, int y, char r, char g, char b)
{
 LCDcolor_foreground=(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F));
 setPixel(x,y);
 r=r/2; g=g/2; b=b/2;
 LCDcolor_foreground=(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F));
 setPixel(x-1,y);
 setPixel(x+1,y);
}

void LCDline(int x0, int y0, int x1, int y1)
    {
    
	if(x0>175){x0=175;}
	if(x1>175){x1=175;}
        //int pix = LCDcolor_foreground;
        int dy = y1 - y0;
        int dx = x1 - x0;
        int stepx, stepy;

        if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
        if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
        dy <<= 1;                                                  // dy is now 2*dy
        dx <<= 1;                                                  // dx is now 2*dx

        setPixel(x0, y0);
        if (dx > dy) {
            int fraction = dy - (dx >> 1);                         // same as 2*dy - dx
            while (x0 != x1) {
                if (fraction >= 0) {
                    y0 += stepy;
                    fraction -= dx;                                // same as fraction -= 2*dx
                }
                x0 += stepx;
                fraction += dy;                                    // same as fraction -= 2*dy
                setPixel(x0, y0);
            }
        } else {
            int fraction = dx - (dy >> 1);
            while (y0 != y1) {
                if (fraction >= 0) {
                    x0 += stepx;
                    fraction -= dy;
                }
                y0 += stepy;
                fraction += dx;
                setPixel(x0, y0);
            }
        }
    }
/*

void LCDline_aa(int x0, int y0, int x1, int y1, char r, char g, char b)
    {
        //int pix = LCDcolor_foreground;
        int dy = y1 - y0;
        int dx = x1 - x0;
        int stepx, stepy;

        if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
        if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
        dy <<= 1;                                                  // dy is now 2*dy
        dx <<= 1;                                                  // dx is now 2*dx


        setPixel(x0, y0);
        if (dx > dy) {
            int fraction = dy - (dx >> 1);                         // same as 2*dy - dx
            while (x0 != x1) {
                if (fraction >= 0) {
                    y0 += stepy;
                    fraction -= dx;                                // same as fraction -= 2*dx
                }
                x0 += stepx;
                fraction += dy;                                    // same as fraction -= 2*dy
                setPixelgv(x0, y0, r, g, b);
            }
        } else {
            int fraction = dx - (dy >> 1);
            while (y0 != y1) {
                if (fraction >= 0) {
                    x0 += stepx;
                    fraction -= dy;
                }
                y0 += stepy;
                fraction += dx;
                setPixelgh(x0, y0, r, g, b);
            }
        }
    }
*/

void LCDhline(int x0, int y0, int x1, int y1)
{
int i;
unsigned int d;

  LCDline(x0,y0,x1,y1);
  
}

void LCDrectangle(int x0, int y0, int x1, int y1)
{
  LCDhline(x0,y0,x1,y0);
  LCDline(x1,y0,x1,y1);
  LCDhline(x1,y1,x0,y1);
  LCDline(x0,y1,x0,y0);
}

void LCDfillrectangle(int x0, int y0, int x1, int y1)
{
int i;
unsigned int d;

 set_x16_color(LCDcolor_foreground);
 if(lcds==2){x0=x0+S2X;x1=x1+S2X;}else{x0=x0+S1X;x1=x1+S1X;}
  //y=(131-y)+10;
  y0=(131+SY)-y0;
  y1=(131+SY)-y1;
  if(x0>x1){i=x0;x0=x1;x1=i;}
  if(y0>y1){i=y0;y0=y1;y1=i;}
  XFillRectangle(disp,pixmap,gc,x0,y0,x1-x0+1,y1-y0+1);
}



void circlePoints(int cx, int cy, int x, int y)
    {
        //int act = Color.red.getRGB();


        if (x == 0) {
            setPixel( cx, cy + y);
            setPixel( cx, cy - y);
            setPixel( cx + y, cy);
            setPixel( cx - y, cy);
        } else
        if (x == y) {
            setPixel( cx + x, cy + y);
            setPixel( cx - x, cy + y);
            setPixel( cx + x, cy - y);
            setPixel( cx - x, cy - y);
        } else
        if (x < y) {
            setPixel( cx + x, cy + y);
            setPixel( cx - x, cy + y);
            setPixel( cx + x, cy - y);
            setPixel( cx - x, cy - y);
            setPixel( cx + y, cy + x);
            setPixel( cx - y, cy + x);
            setPixel( cx + y, cy - x);
            setPixel( cx - y, cy - x);
        }
    }

void LCDcircle(int xCenter, int yCenter, int radius)
    {
        //int pix = c.getRGB();
        int x = 0;
        int y = radius;
        int p = (5 - radius*4)/4;

        circlePoints(xCenter, yCenter, x, y);
        while (x < y) {
            x++;
            if (p < 0) {
                p += 2*x+1;
            } else {
                y--;
                p += 2*(x-y)+1;
            }
            circlePoints(xCenter, yCenter, x, y);
        }
    }
/*
void glcdDoPixelLine(int16_t x1, int16_t x2, const int16_t y, const uint8_t fill) {

    if ((y >= glcd_Clip.Y1 ) & (y <= glcd_Clip.Y2)) {
      if (x1 < glcd_Clip.X1) {x1 = glcd_Clip.X1;} else {
        if (x1 > glcd_Clip.X2) {return;} else {
          if (glcdFgColor != NONE) {
            setPixel(x1, y);
          }
          x1++;
        }
      }
      if (x2 < glcd_Clip.X1) {return;} else {
        if (x2 > glcd_Clip.X2) {x2 = glcd_Clip.X2;} else {
          if (glcdFgColor != NONE) {
            setPixel(x2, y);
          }
          x2--;
        }
      }
      if ((fill) & (glcdBkColor != NONE) & (x1 <= x2)) {
        glcdDoFillRect(x1, y, x2, y, glcdBkColor);
      }
    }
}
*/
void LCDellipse(int x, int y, int a, int b) {



   int aa = a * a;
   int bb = b * b;
   int er, cr, ir;
   int ys,ye,xs,xe;
   //int fill = 1;

   cr = bb >> 1;
   cr = cr * (a + a -1);
   ir = aa >> 1;
   ir = -ir;
   er = 0;

   xs = x;
   xs = xs - a;
   xe = x;
   xe = xe + a;
   ys = y;
   ye = y;
   while (cr >= ir) {
     LCDhline(xs, ys, xe, ys);
     if (ys != ye) {
       LCDhline(xs, ye, xe, ye);
     }
     ys--;
     ye++;
     ir += aa;
     er += ir;
     if (2 * er > cr) {
       er -= cr;
       cr -= bb;
       xs++;
       xe--;
     }
   }

   cr = aa >> 1;
   cr = cr * (b + b -1);
   ir = bb >> 1;
   ir = -ir;
   er = 0;

   xs = x;
   xe = x;
   ys = y;
   ys = ys - b;
   ye = y;
   ye = ye + b;

   while (ir <= cr) {
     LCDhline(xs, ys, xe, ys);
     if (ys != ye) {
       LCDhline(xs, ye, xe, ye);
     }
     //fill = 0;
     ir += bb;
     er += ir;
     if (2 * er > cr) {
       er -= cr;
       cr -= aa;
       ys++;
       ye--;
       //fill = 1;
     }
     xs--;
     xe++;
   }

}


#define FONT_HEADER_SIZE        7      // Header größe eines Fonts

void pchar(char font, char transparent, char c, int space)
{

    const char *glcd_FontData;
    switch(font)
    {
    case F_APRS    : glcd_FontData = &aprs[0];break;
    case F_BIGSYM  : glcd_FontData = &bigsym[0];break;
    case F_DIGITS  : glcd_FontData = &digits[0];break;
    case F_15X15   : glcd_FontData = &f15x15[0];break;
    case F_14X22   : glcd_FontData = &f15x22[0];break;
    case F_20X23   : glcd_FontData = &f20x23[0];break;
    //case F_7X10    : glcd_FontData = &f7x10[0];break;
    case F_8X10    : glcd_FontData = &f8x10[0];break;
    case F_SYM_MONO: glcd_FontData = &sym_mono[0];break;
    case F_ZNAKI   : glcd_FontData = &znaki[0];break;

    }
    //char glcd_FontWidth = *(glcd_FontData + 2);
    int glcd_FontHeight = *(glcd_FontData + 3);
    int glcd_FontBitsPixel = *(glcd_FontData + 4);
    int glcd_FontFirstChar = *(glcd_FontData + 5);
    int glcd_FontLastChar = *(glcd_FontData + 6);

    if(c>127){c='.';}
    if(c<glcd_FontFirstChar){c='.';}//{c=glcd_FontFirstChar;}

 	int x;
	int y;
	char charwidth = *(glcd_FontData + FONT_HEADER_SIZE + c - glcd_FontFirstChar);
    uint8_t width = charwidth;
	
    const char *data;
	unsigned int index;
	char table[4];
	char i,padding,bitscount,bitspixel,bitsmask,pixelcount;
	unsigned int bits,pixelcolor,colmem;
	//char charwidth = *(glcd_FontData + FONT_HEADER_SIZE + c - glcd_FontFirstChar);
 	
    char stopY; // unterste Pixelzeile des Zeichens
    char stopX;
	//char txt[30];
	
	index=0;
	x=cursorx;
	y=cursory;
	stopY = cursory + glcd_FontHeight -1; // unterste Pixelzeile des Zeichens
        stopX = x+charwidth -1;
	
	bitspixel = glcd_FontBitsPixel & 0x7F;
	bitsmask = 0xFF >> (8 - bitspixel);
	colmem=LCDcolor_foreground;
	//sprintf(txt,"%d %d %d %d %d",c,glcd_FontFirstChar,glcd_FontLastChar,charwidth,glcd_FontHeight);
	//LCDyxtxt(0,30,30,txt);
	
	if (c < 128) {width++;}
	if(flagfixed){
          width = *(glcd_FontData + 2);
	  i=0;
	  LCDcolor_foreground=LCDcolor_background;
	  if ((flagfixed == 1) & (width > charwidth)) {
            i = (width - charwidth) / 2;
            if (!transparent) {
            LCDfillrectangle(x, y, x + i +space, y + glcd_FontHeight -1);
            }
            x += (int)i;
	    stopX += (int)i;
          }
          i = width - charwidth - i;
          if ((i > 0) & (!transparent)) {
           LCDfillrectangle(x + charwidth, y, x + charwidth + i +space, y + glcd_FontHeight -1);
          }
	
	} //if flagfixed
	
	if (glcd_FontBitsPixel & 0x80) {
    // komprimierte Fonts

    // Hole das BytePadding die RLE Längentabelle und berechne den Byteindex zu den Daten.
    // Dieser Lesevorgang hat einen zusätzliche Overhead von 4 bytes bei komprimierten Fonts.
    // Allerdings wird dieser Overhead bei weitem in der eigentlichen Fontzeichendarstellungsschleife
    // wieder wett gemacht !
    // Bei komprimierten Fonts kann kein linkes Clipping vorgenommen werden :(
      data = glcd_FontData + FONT_HEADER_SIZE + glcd_FontLastChar - glcd_FontFirstChar +1;
      //uint8_t padding = glcd_FontRead(data++);
	  padding=(*data++);
      table[0] = 1;
      table[1] = (*data++);
      table[2] = (*data++);
      table[3] = (*data++);

      for (i = glcd_FontFirstChar; i < c; i++) {
        index += (*data++);
      }
      index *= padding;
      data = glcd_FontData + FONT_HEADER_SIZE + (glcd_FontLastChar - glcd_FontFirstChar +3) * 2 + index;
      bits = (*data++);
      bitscount = 8;
    } else {
    // unkomprimierte Fonts

    // berechne nun den Bitindex zu den Bitdaten im Font
      data = glcd_FontData + FONT_HEADER_SIZE;
      //uint32_t index = 0;
      for (i = glcd_FontFirstChar; i < c; i++) {
        index += (*data++);
      }

      index *= glcd_FontHeight * bitspixel;

    // positioniere data auf's erste Byte mit Fontdaten zum Zeichen und hole die ersten Bits
      bitscount = index % 8;
      index /= 8;
      data = glcd_FontData + FONT_HEADER_SIZE + glcd_FontLastChar - glcd_FontFirstChar +1 + index;
      bits = (*data++) >> bitscount;
      bitscount = 8 - bitscount;
    }
	
	
	pixelcount=0;
	pixelcolor=0;
	//stopX = x+charwidth -1;
	//sprintf(txt,"%d %d %d %d %d ",bits,bitscount,bitspixel,*(data+4), stopY);
	//LCDyxtxt(0,100,10,txt);
	//LCDyxtxt(0,50,10,":");
	for (; x <= stopX; x++) {
     // if (clipY |= glcd_Cursor.Y) {
     //   clipinvalid = 1;
     // }
      //for (y = cursory; y <= stopY; y++) {
	  for (y = stopY; y >= cursory; y--) {
        if (pixelcount == 0) {//true
          if (bitscount <= 8) {//true
            bits |= (*data++) << bitscount;//0x08
            bitscount += 8;//16
          }
          if (glcd_FontBitsPixel & 0x80) {
            pixelcount = table[bits & 3];
            bits >>= 2;
            bitscount -= 2;
          } else {
            pixelcount++;
          }
		
          if (bitspixel <= 2) {
		    c=bits&bitsmask;
			if(c>0){pixelcolor=colmem;}
			//if(c==2){pixelcolor=GREEN;}
			//if(c==3){pixelcolor=PALIO;}
			
          } else {
            pixelcolor = bits & bitsmask;
			c=pixelcolor;
			//if(c==0){pixelcolor=WHITE;}
                        switch(c){
                        case 0:break;
                        case 4:pixelcolor=BLUE;break;
                        case 5:pixelcolor=GREEN;break;
                        case 10:pixelcolor=YELLOW;break;
                        case 11:pixelcolor=RED;break;
                        case 8:pixelcolor=BLACK;break;
                        default :pixelcolor=WHITE;
                        }
                        /*
			if(c==10){pixelcolor=YELLOW;}
			if(c==11){pixelcolor=RED;}
			if(c==5){pixelcolor=GREEN;}	
			if(c==4){pixelcolor=BLUE;}
			if(c==12){pixelcolor=BLACK;}
			*/
          }
          bits >>= bitspixel;
          bitscount -= bitspixel;
        }
        pixelcount--;
		
        if (c>0) {

 		  //index=LCDcolor_foreground;
          LCDcolor_foreground=pixelcolor;
          setPixel(x,y);
		  //LCDcolor_foreground=index;
          } else {
		   if(!transparent){
		    LCDcolor_foreground=LCDcolor_background;
            setPixel(x,y);
		   }
		  }
      }
    }
	LCDcolor_foreground=colmem;
	//sprintf(txt,"%d %d ",cursorx,width);
	//LCDyxtxt(0,2,2,txt);
	index=width; //konwersja typu
	cursorx=cursorx+index+space;
	//cursorx=stopX+10;

        if(spirequest>0){
          waitms(1);
          spi_request_handler();
        }
}


void LCDchr(char font, char c)
{
	switch(font){
        case 0: chargen(c);break;
	case 1: smchargen(c);break;
	case 2: smchargen(c);break;//chargen3(c);break;
	case 3: pchar(F_14X22,0,c,0);break;//f14x22
	case 4: pchar(F_DIGITS,0,c,0);break;//digits
        case 5: pchar(F_8X10,0,c,-1);break;//f7x10
        case 6: pchar(F_15X15,0,c,0);break;//f15x15
        case 7: pchar(F_8X10,0,c,0);break;//f7x10
    }

}

void LCDchrt(char font, char c)
{
	switch(font){
        case 0: chargen(c);break;
	case 1: smchargen(c);break;
	case 2: smchargen(c);break;//chargen3(c);break;
	case 3: pchar(F_14X22,1,c,0);break;//f14x22
	case 4: pchar(F_DIGITS,1,c,0);break;//digits
        case 5: pchar(F_8X10,1,c,-1);break;//f7x10
        case 6: pchar(F_15X15,1,c,0);break;//f15x15
        case 7: pchar(F_8X10,1,c,0);break;//f7x10
    }

}

void LCDyxtxt(char font, char y, char x, char *string)
{
  LCDyx(y,x);
  cursorx=x;
  cursory=y;
  while(*string){
    LCDchr(font,*string++);
  }
}

void LCDyxtxtt(char font, char y, char x, char *string)
{
  LCDyx(y,x);
  cursorx=x;
  cursory=y;
  while(*string){
    LCDchrt(font,*string++);
  }
}



void LCDtxt(char font,char *string)
{
   while((*string)&&(cursorx<176)){
    LCDchr(font,*string++);
	//chargen(*string++);
	//cursorx=cursorx+7;//CHAR_W;
  }
}

void LCDtxtt(char font,char *string)
{
   while((*string)&&(cursorx<176)){
    LCDchrt(font,*string++);
	//chargen(*string++);
	//cursorx=cursorx+7;//CHAR_W;
  }
}

void LCDfixed(char fixed)
{
 flagfixed=fixed;
}
