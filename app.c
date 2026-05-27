#include "includes.h"
#include <math.h>


char station_option=0;


void compass(char mode,int i)
//mode==1 means don't use colors
{
#define compass_offsx 	  5
#define compass_offsy	  11
int num;
float si,co,sy,ff;
int x1,y1,x2,y2,x3,y3;

  LCDselect(2);
  if(!mode){LCDcolor_foreground=rgb(31,0,0);}

  //romb
  ff=1.0/180.0*3.1415926;
  si=(float)i*ff;  	
	co=cos(si);	
	si=sin(si);
  x1=-45.0*si+121;
  y1=+45.0*co+67;
  si=(float)(i+5)*ff;  	
	co=cos(si);	
	si=sin(si);
  x2=-38.0*si+121;
  y2=+38.0*co+67;
  LCDline(x1,y1,x2,y2);
  si=(float)(i-5)*ff;  	
	co=cos(si);	
	si=sin(si);
  x3=-38.0*si+121;
  y3=+38.0*co+67;
  LCDline(x1,y1,x3,y3);
  LCDline(x3,y3,x2,y2);

  if(!mode){LCDcolor_foreground=(nightv ? GREEN : rgb(0,0,15));}
  for(num=30;num<360;num=num+30){
    si=(float)(num+i)*ff;  	
	co=cos(si);	
	si=sin(si);
	sy=41.0;
	if((num==90)||(num==180)||(num==270)){sy=36.0;}
	x1=-45.0*si+121;
	y1=+45.0*co+67;
	x2=-sy*si+121;
	y2=+sy*co+67;
	LCDline(x1,y1,x2,y2);

  }
  if(!mode){LCDcolor_foreground=(nightv ? GREEN : 0);}
  si=(float)(i)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x1=-26.0*si+121;
  y1=+26.0*co+67;
  x1=x1-compass_offsx;
  y1=y1-compass_offsy;
  LCDyx(y1,x1);
  pchar(F_14X22,1,'N',0);
  //LCDyxtxt(2,y1,x1,"N");


  si=(float)(i+90)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x1=-26.0*si+121;
  y1=+26.0*co+67;
  x1=x1-compass_offsx;
  y1=y1-compass_offsy;
  LCDyx(y1,x1);
  pchar(F_14X22,1,'W',0);
  //LCDyxtxt(2,y1,x1,"W");

  si=(float)(i+180)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x1=-26.0*si+121;
  y1=+26.0*co+67;
  x1=x1-compass_offsx;
  y1=y1-compass_offsy;
  LCDyx(y1,x1);
  pchar(F_14X22,1,'S',0);
  //LCDyxtxt(2,y1,x1,"S");

  si=(float)(i+270)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x1=-26.0*si+121;
  y1=+26.0*co+67;
  x1=x1-compass_offsx;
  y1=y1-compass_offsy;
  flagfixed=0;
  LCDyx(y1,x1);
  pchar(F_14X22,1,'E',0);//pchar(f15x22,1,'E');
  //LCDyxtxt(2,y1,x1,"E");

  LCDcolor_foreground=(nightv ? YELLOW : BLACK);
  LCDyx(56,115);
  pchar(F_SYM_MONO,1,0,0);//pchar(sym_mono,1,0); //arrow


}


void welcome()
{
  char c,i;
  unsigned long tt;
  char txmem;
  char ss[10];

  txmem=txenable;
  txenable=0;   //wylacz na razie nadajnik
  LCDcolor_background=(nightv ? BLACK : WHITE);
  LCDselect(3);
  LCDclear();
 LCDselect(1);
 LCDcolor_foreground=rgb(31,0,0);
 LCDyx(70,15);
 pchar(F_APRS,1,'A',0);//pchar(aprs,1,'A');
 pchar(F_APRS,1,'P',0);
 pchar(F_APRS,1,'R',0);
 pchar(F_APRS,1,'S',0);
 LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
 LCDyxtxt(3,45,20,"  NAVIGATOR   ");
 LCDcolor_foreground=(nightv ? BLUE : BLACK);
 LCDyxtxt(1,30,25,"Amateur position reporting system");
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 LCDrectangle(1,20,174,112);
 LCDselect(2);
 LCDcolor_foreground=GREEN;
 LCDyx(50,5);
 pchar(F_BIGSYM,1,0,0);//pchar(bigsym,1,0);
 LCDcolor_foreground=(nightv ? BURSZTYN : BLUE);
 LCDyxtxt(0,80,50," FHI NAVIGATOR");
 LCDyxtxt(0,70,50," VERSION 0.88b");
 LCDyxtxt(0,55,50,"(C) 2005, 2006");
 LCDyxtxt(0,45,50,"T.KUEHN SP3FHI");
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 sprintf(napis,"%s %s",__DATE__,__TIME__);
 LCDyxtxt(F_SMALL,21,5,napis);
 LCDrectangle(1,20,174,112);
 flash_read(napis,10,255);

 c=read_kbd();
 while(c>0){c=read_kbd();}
 c=0;
 tt=timeval+100;
 while(((c<1)||(c>9))&&(timeval<tt)){c=read_kbd();}

 sprintf(ss,"%08x",utimeval*utimeval);


 c=0;
 for(i=0;i<5;i++){c=c+napis[i+5];}
 ss[5]=c;
 for(i=0;i<5;i++){c=c+napis[i+15];}
 ss[6]=c;
//HACKERS CONTROL
 //printf("Hackers:\n");
 //for(i=0;i<8;i++){
 //  printf("%d\n",ss[i]);
 //}
 hackcheck=test_call(ss);
 //printf("Result:%d\n",hackcheck);
//HACKERS CONTROL
 LCDselect(3);

 LCDcolor_background=(nightv ? BLACK : WHITE);
 LCDclear();
 LCDselect(1);

 LCDcolor_foreground=WHITE;
 LCDrectangle(0,0,175,131);
 LCDcolor_foreground=rgb(31,0,0);
 LCDyx(80,15);
 c=system_userlevel;
 pchar(F_APRS,1,'A',0);//pchar(aprs,1,'A');
 pchar(F_APRS,1,'P',0);
 pchar(F_APRS,1,'R',0);
 pchar(F_APRS,1,'S',0);
 LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
 LCDyxtxt(3,55,25,"  NAVIGATOR   ");
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 switch(c){
 case 0: LCDyxtxt(3,34,25," Professional+");break;
 case 1: LCDyxtxt(3,34,25," Professional ");break;
 case 2: LCDyxtxt(3,34,25,"Navigator 800 ");break;
 case 3: LCDyxtxt(3,34,25,"Navigator 800S");break;
 case 4: LCDyxtxt(3,34,25,"Navigator DEMO");break;
 }
 LCDcolor_foreground=WHITE;
 LCDyx(10,40);
 for(i=0;i<6;i++){
   LCDchr(F_14X22,stations[0].sign[i]);
 }
 if(stations[0].ssid>0){
  sprintf(napis,"-%d",stations[0].ssid);
  LCDtxt(F_14X22,napis);
 }

 LCDselect(2);

 txenable=config_startup(txmem);
 LCDselect(3);
 LCDclear();

}



static int mi;
static int gper=0;

void compass_page_update()
{
  char i,j;
  int km;
  unsigned int cs;

  //course
  if(gpsfix){
    i=0;
    while((gpscourse[i]>'.')&&(i<3)){i++;}
    if(gpscourse[i]=='.')
    {
     i=0;cs=0;
     while(gpscourse[i]!='.'){
       cs=cs*10;
       cs=cs+gpscourse[i]-'0';
       i++;
     }
    } else
    {
     cs=mycourse;
    }
   } else
   {
     cs=mycourse;
   }



  flagfixed=0;
  LCDcolor_foreground=BLACK;
  LCDcolor_background=(nightv ? BLACK : WHITE);

  if(cs!=mi){
  compass(1,mi);
  mi=cs;
  compass(0,mi);
  }
  LCDcolor_foreground=(nightv ? GREEN : BLACK);

  flagfixed=1;

  sprintf(napis,"%003d",cs);
  LCDyxtxt(4,61,7,napis);

  //speed
  if(gpsfix){
    i=0;
    while((gpsspeed[i]>'.')&&(i<3)){i++;}
    if(gpsspeed[i]!='.') //no speed at all
    {
     km=0;
    }
    else{
      i=0;km=0;
      while(gpsspeed[i]!='.'){
        km=km*10;
        km=km+gpsspeed[i]-'0';
        i++;
      }
    }


    km=km*1.8532;

  } else
  {
    km=0;
  }
  sprintf(napis,"%003d",km);
  LCDyxtxt(4,90,7,napis);

    LCDyxtxt(0,41,2,gpslat);
    LCDyxtxt(0,32,2,gpslon);

  if(gpsfix){LCDyxtxt(0,120,1,"GPS FIX ");}
   else
   {
     if(gpsdata){LCDyxtxt(0,120,1,"GPS ON  ");}
     else{LCDyxtxt(0,120,1,"GPS OFF ");}

   }

  if(!gpsfix){gpssatnum=0;}
  for(i=0;i<12;i++){
    if(i<gpssatnum){
      LCDcolor_foreground=(nightv ? GREEN : BLACK);
    LCDfillrectangle(85+7*i,2,90+7*i,14);
    }
    else
    {
      LCDcolor_foreground=(nightv ? BLACK : WHITE);
    LCDfillrectangle(85+7*i,2,90+7*i,14);
      LCDcolor_foreground=(nightv ? GREEN : BLACK);
    LCDfillrectangle(85+7*i,2,90+7*i,2);
    }
  }
  /*
  if((gpssatnum!=0)&&(gpssatnum!=8)){
    gper++;
    sprintf(napis,"%d  ",gpssatnum);
    LCDyxtxt(0,130,80,napis);
  }
  sprintf(napis,"%d  ",gper);
  LCDyxtxt(0,100,80,napis);
  */
  flagfixed=0;
}




void station_show(char redraw, char id, char opt)
{
char i,c,c1;
unsigned long tt;
int y;
unsigned int lastfound;
char tx[20];

#ifdef rsdebug
    printf("app:station_show\n");
#endif

 LCDselect(2);
 flagfixed=0;
 if(redraw){
   LCDclear();

 LCDcolor_foreground=(nightv ? YELLOW : BLACK);

 LCDrectangle(0,91,175,131);
 LCDline(0,101,139,101);
 LCDline(139,91,139,131);
 }
 else
 {
  //wymaz pole pod wskaznikiem
   LCDcolor_foreground=(nightv ? BLACK : BLACK);
   LCDfillrectangle(140,92,174,130);
   LCDfillrectangle(1,92,138,100);
 }
 flagfixed=0;
 LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
 if(stations[id].dist<999999){
   tt=stations[id].dist/10;
   sprintf(napis,"DIST:%4ld.%d BRG:%3d",tt,stations[id].dist-10*tt,stations[id].namiar);
   LCDyxtxt(0,92,2,napis);
   LCDyx(96,142);
   station_traker(id,0);
 } else
 {
   LCDyxtxt(0,92,2,"NO POSITION YET");
 }
 LCDcolor_foreground=(nightv ? WHITE : BLACK);


 if(redraw)
 {
 LCDyx(105,3);
 for(i=0;i<6;i++)
 {if((stations[id].sign[i]>32)&&(stations[id].sign[i]<91)){pchar(F_20X23,1,stations[id].sign[i],0);};}
 if(stations[id].ssid>0){
   /*
   if(cursorx<106){
      sprintf(napis,"-%d",stations[id].ssid);
   }else
   {
      sprintf(napis,"%d",stations[id].ssid);
   }
   LCDyxtxt(F_SMALL,122,cursorx,napis);
   */
   LCDcolor_foreground=YELLOW;
   sprintf(napis,"%d",stations[id].ssid);
   i=6;
   if(stations[id].ssid>9){i=i+6;}
   if(cursorx+i<115){LCDyxtxt(7,118,cursorx,napis);}
   else {LCDyxtxt(7,118,115-i,napis);}
 }
 if(id==marked_station){LCDyx(112,2);pchar(F_ZNAKI,1,0x07,0);}
 LCDyx(105,115);
 switch(stations[id].symbol)
 {
 case '#' : pchar(F_ZNAKI,1,0x0A,0);break;
 case '_' : pchar(F_ZNAKI,1,0x0B,0);break;
 case '&' : pchar(F_ZNAKI,1,0x0C,0);break;
 case '-' : pchar(F_ZNAKI,1,0x0D,0);break;
 case 'K' : pchar(F_ZNAKI,1,0x0E,0);break;
 case '>' : pchar(F_ZNAKI,1,0x0F,0);break;

 }

 }
 flagfixed=0;
 if(opt){ //last packet


   if(opt==1){ //ostatnia ramka
     flash_read(napis,stations[id].slot,255);

   }
   else
   { //poprzednia ramka

     lastfound=stations[id].slot;
     c1=opt-1;
     y=stations[id].slot-1;
     if(y<1){if(eepromfull){y=MAXEEPROM;}else{y++;};}
     c=0;
     while((!c)&&(y!=stations[id].slot)){
        flash_read(napis,y,16);
        c=1;
        for(i=0;i<6;i++){if(stations[id].sign[i]!=napis[8+i]){c=0;};}
        if(c){ //sprawdz ktora ramka
          lastfound=y;
          c1=c1-1;
          if(c1>0){c=0;}
        }
        if(!c){y=y-1;}
        if(y<1){if(eepromfull){y=MAXEEPROM;}else{y=stations[id].slot;};}
     }
     if(c){ //previous found
       flash_read(napis,y,255);
     } else {
       flash_read(napis,lastfound,255);
       station_option--;
     }



   } //if opt

   LCDcolor_foreground=(nightv ? YELLOW : GREEN);
   if(station_option>1){sprintf(tx,"PREV:%d (%d) CRC:%d",station_option-1,napis[250],napis[251]);}
   else{sprintf(tx,"LAST (%d) CRC:%d",napis[250],napis[251]);}
   LCDyx(78,2);
   LCDtxt(7,tx);
   LCDcolor_foreground=(nightv ? GREEN : RED);

   LCDyx(67,2);
   y=67;
   //header
   i=1;
   while((napis[i]!=0x03)&&(y>0))
   {
    LCDchrt(7,napis[i]);
    if(cursorx>165){
      y=y-11;//10
      LCDyx(y,2);
    }
    i++;
   }
   //body
   LCDcolor_foreground=(nightv ? WHITE : BLACK);
   i++;
   i++;
   y=y-11;//10
   LCDyx(y,2);
   while((napis[i]!=0)&&(y>0))
   {
    LCDchrt(7,napis[i]);
    if(cursorx>165){
      y=y-10;//10
      LCDyx(y,2);
    }
    i++;
   }
 } else  //if opt
 { //info
#ifdef rsdebug
    printf("disp station info\n");
#endif
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 LCDfillrectangle(2,79,7,87);
 LCDyx(78,10);
 i=0;
 while(stations[id].info[i]!=0)
 {
  LCDchrt(7,stations[id].info[i]);
  if(cursorx>165){LCDyx(67,10);}
  i++;
 }
 if(i==0){
   LCDcolor_foreground=(nightv ? GREY : BLACK);
   LCDtxt(7,"No info!");
 }


 LCDcolor_foreground=(nightv ? WHITE : BLACK);

 LCDyxtxt(0,40,5,"FRAME:");
 LCDchr(0,stations[id].frame);
 LCDchr(0,32);
 switch(stations[id].frame)
 {
  case '=':;
  case '/':;
  case '!':;
  case '@':LCDtxt(0,"POSITION");break;
  case '`' :;
  case 0x27 :LCDtxt(0,"MIC-E ");break;
  case '>' :LCDtxt(0,"STATUS");break;
  case '}' :LCDtxt(0,"TCP/IP");break;
  case '_' :LCDtxt(0,"WEATHER");break;
  case ':' :LCDtxt(0,"MESSAGE");break;
  default:LCDtxt(0,"?");
 }
 if(stations[id].wxtemp!=NOTEMP){
  LCDcolor_foreground=(nightv ? WHITE : BLACK);
  y=stations[id].wxtemp;
  sprintf(napis,"%dC",y);
  LCDyx(40,130);
  pchar(F_SYM_MONO,1,0x2C,0);
  cursory=cursory+4;
  LCDtxt(7,napis);
  if(y<0)LCDcolor_foreground=LIGHT_BLUE;
  if((y>=0)&&(y<10))LCDcolor_foreground=BLUE;
  if((y>=10)&&(y<20))LCDcolor_foreground=YELLOW;
  if((y>=20)&&(y<30))LCDcolor_foreground=GREEN;
  if(y>=30)LCDcolor_foreground=RED;
  LCDrectangle(134,45,135,54);
 }
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 LCDyxtxt(0,50,5,"SYMBL:");
 LCDchr(0,stations[id].symbol);
 //sprintf(napis,"SLOT:%d",stations[id].slot);
 //LCDyxtxt(0,50,60,napis);

 //if(stations[id].opt&0x01){ //ui-view?
 //  LCDyx(45,120);
 //  pchar(F_ZNAKI,1,0x25,0);
 //}
 if((stations[id].type==1)||(stations[id].type==2))
 {
 LCDyxtxt(0,30,5,"via ");
 if(stations[id].pathvalid)
    { LCDcolor_foreground=(nightv ? YELLOW : BLACK);
      for(i=0;i<6;i++){LCDchr(0,stations[id].path[i]);};
    }
 }

 if(stations[id].dist<999999){

   //if(stations[id].distchg==1){LCDyxtxt(0,10,10,"zbliza sie");}
   //if(stations[id].distchg==2){LCDyxtxt(0,10,10,"oddala sie");}


 LCDyx(20,5);
 if(stations[id].position[6]&0xF0>0){c='S';}else{c='N';}  //S
 if(stations[id].position[6]&0x0F>0){c1='W';}else{c1='E';}
 sprintf(napis,"%02d %02d.%02d%c/%03d %02d.%02d%c",stations[id].position[0],\
   stations[id].position[1],stations[id].position[2],c,stations[id].position[3],\
     stations[id].position[4],stations[id].position[5],c1);
 LCDtxt(0,napis);
 if(stations[id].moving&0x80){pchar(F_SYM_MONO,1,0x29,0);}
   else {pchar(F_SYM_MONO,1,0x28,0);}

 }
 }//info
}


char count_messages()
{
  char i,mm;
  mm=0;
 for(i=0;i<MAX_MESSAGES;i++){
   if(messages[i].status>0){mm++;}

 }
 return mm;
}

void present_message(char mid)
{
  char i,c,y;
  char ss[4];

  if(messages[mid].status==1){messages[mid].status=2;}//mark as read
  LCDselect(2);
  LCDclear();
  LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
  LCDrectangle(1,110,175,131);
  LCDcolor_foreground=(nightv ? WHITE : BLACK);
     LCDfixed(1);
     LCDyx(114,5);
     for(c=0;c<6;c++)//znak stacji - a co jak zniknie z listy?
      {pchar(F_8X10,1,stations[messages[mid].stid].sign[c],-1);}
     i=stations[messages[mid].stid].ssid;
     if(i>0){
       sprintf(ss,"-%d",i);
       LCDtxt(F_8X10,ss);
      }
     LCDyx(115,77);
     pchar(F_ZNAKI,1,9,0);
     LCDyx(114,105);
     i=1;
     while(napis[i]!=':'){i++;}
     i++;
     for(c=0;c<9;c++)//znak stacji - a co jak zniknie z listy?
      {pchar(F_8X10,1,napis[c+i],-1);}

   LCDfixed(0);
   LCDyx(67,2);
   y=67;
   //header
   LCDcolor_foreground=(nightv ? YELLOW : BLACK);
   i=1;
   while((napis[i]!=0x03)&&(y>0))
   {
    LCDchrt(7,napis[i]);
    if(cursorx>165){
      y=y-11;//10
      LCDyx(y,2);
    }
    i++;
   }
   //body
   LCDcolor_foreground=(nightv ? WHITE : BLACK);
   i++;
   i++;
   y=y-11;//10
   LCDyx(y,2);
   while((napis[i]!=0)&&(y>0))
   {
    LCDchrt(7,napis[i]);
    if(cursorx>165){
      y=y-10;//10
      LCDyx(y,2);
    }
    i++;
   }
}


void messages_show(char opt,char start,char cur)
{
char c,i,y,k;
char ur,mm;

#ifdef msgdebug
      printf("Messages show \n");
#endif


 if(opt){LCDselect(1);}else{LCDselect(2);}

 flagfixed=0;
 LCDcolor_background=(nightv ? BLACK : WHITE);
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 mm=0;
 ur=0;
 for(i=0;i<MAX_MESSAGES;i++){
   if(messages[i].status>0){mm++;}
   if(messages[i].status==1){ur++;}
 }


 sprintf(napis,"MSGS #%2d/UR:%2d",mm,ur);
 LCDyxtxt(0,124,1,napis);
 LCDline(0,123,176,123);
 i=0;
 y=105;
 if(mm==0){
  LCDyxtxt(7,105,7,"No messages received");
 }
 mm=0;
 while((y>0)&&(i<MAX_MESSAGES))
 {

   if(messages[i].status>0){
     mm++;
     if(mm>=start)
     {
     LCDyx(y,6);
     if(messages[i].status==1){LCDcolor_foreground=(nightv ? YELLOW : BLACK);}
     if(messages[i].status==2){LCDcolor_foreground=(nightv ? GREY : BLACK);}
     if((messages[i].status==1)&&(messages[i].option==0))
      {LCDcolor_foreground=(nightv ? RED : BLACK);}


     LCDfixed(1);
     for(c=0;c<6;c++)//znak stacji - a co jak zniknie z listy?
      {pchar(F_8X10,0,stations[messages[i].stid].sign[c],-1);}

     //retrieve from eeprom
     flash_read(napis,messages[i].slot,255);

     if(messages[i].option==1)//foreign
     {
       pchar(F_8X10,0,'>',-1);
       k=1;
       while((napis[k]!=0x03)&&(k<100)){k++;}
       k=k+3;
       c=0;
       while((c<6)&&(napis[k+c]!='-')&&(napis[k+c]>' '))
        {pchar(F_8X10,0,napis[k+c],-1);c++;}

     }



     pchar(F_8X10,0,':',-1);
     LCDcolor_foreground=BLACK;
     LCDfillrectangle(cursorx,cursory,175,cursory+9);
     LCDcolor_foreground=(nightv ? WHITE : BLACK);
     LCDfixed(0);
     //search for text
     c=1;
     while((napis[c]!=':')&&(c<250)){c++;}
     c++;
     while((napis[c]!=':')&&(c<250)){c++;}
     c++;
     while((napis[c]!='{')&&(c<250)&&(napis[c]>0)&&(cursorx<170))
     {
       LCDchrt(7,napis[c]);
       c++;
     }

     cur--;
     if((cur==0)&&(opt)){present_message(i);LCDselect(1);}


     y=y-10;
     }//if mm
   }
   i++;
 }//while

#ifdef msgdebug
      printf("Message show end\n");
#endif

}

static int pcmem=100;

void check_receiver()
{
  char c,pc;
  static unsigned long tt;

          //aprs_read(0x200,2,napis);

           LCDcolor_foreground=(nightv ? WHITE : BLACK);
           LCDselect(1);
           LCDyx(125,163);
           flagfixed=0;
	   if(aprs_squelch_flag){//squelch
             pchar(F_ZNAKI,1,0x04,0);
           } else {
             pchar(F_ZNAKI,1,0x05,0);
           }
 if(pcmem!=packet_count){
    pcmem=packet_count;
  LCDcolor_foreground=(nightv ? BLACK : BLUE);
  LCDfillrectangle(0,124,8,131);
  LCDcolor_foreground=(nightv ? WHITE : BLUE);
  LCDyx(125,0);
  pc=(char)(packet_count&0x07);
  pc=pc+0x30;
  pchar(F_SYM_MONO,1,pc,2);
  }
  //verify last digisens beacon status
  pc=0;
  for(c=0;c<6;c++){
     if(selfdigis[c].status==1){pc=1;}
    }
  if(pc){
    LCDcolor_foreground=(nightv ? GREEN : BLUE);
  } else
  {
    LCDcolor_foreground=(nightv ? RED : BLUE);
  }
  LCDyx(125,93);
  pchar(F_SYM_MONO,1,0x2F,2);
  if(gpsfix){
    LCDcolor_foreground=(nightv ? GREEN : BLUE);
  }
  else
  {
    LCDcolor_foreground=(nightv ? RED : BLUE);
  }
  LCDyx(125,108);
  pchar(F_SYM_MONO,1,0x2D,2);

  pc=0;
  for(c=0;c<MAX_MESSAGES;c++){
   if(messages[c].status==1){pc=1;}
  }

  LCDyx(125,78);
  LCDcolor_foreground=(nightv ? BURSZTYN : BLUE);
  if(timeval>(tt+10)){
    LCDcolor_foreground=(nightv ? BLACK : BLUE);
  }
  if(timeval>(tt+20)){
   tt=timeval;
  }
  if(!pc)
  {
    LCDcolor_foreground=(nightv ? BLACK : BLUE);
  }
  pchar(F_SYM_MONO,1,0x2E,2);

 if(aprs_received_flag>0){
   if(spirequest>0){spi_request_handler();}
   if(uniquemark||(!(announce_filter&0x02))){
   aprs_data_flag=1;
   receive_status=1;
   update_status|=UPDATE_RECEIVE;
   }
   aprs_received_flag=0;

 };
}



char gps_change()
{
char gps_bigchange;
int i;
static unsigned int cs;
static char memgpslat[10];
static char memgpslon[10];

  gps_bigchange=0;
     if((memgpslat[5]!=gpslat[5])||\
       (memgpslon[6]!=gpslon[6])){gps_bigchange=1;}
     i=memgpslat[6]-gpslat[6];
     if(i<0){i=-i;}
     if(i>5){gps_bigchange=1;}
     i=memgpslon[7]-gpslon[7];
     if(i<0){i=-i;}
     if(i>5){gps_bigchange=1;}

     i=0;
    while((gpscourse[i]>'.')&&(i<3)){i++;}
    if(gpscourse[i]=='.')
    {
     i=0;cs=0;
     while(gpscourse[i]!='.'){
       cs=cs*10;
       cs=cs+gpscourse[i]-'0';
       i++;
     }
     i=cs-mycourse;
     if(i<0){i=-i;}
     if(i>2){gps_bigchange=1;}
    }



     if(gps_bigchange){
     //save memories
     memgpslat[5]=gpslat[5];
     memgpslat[6]=gpslat[6];
     memgpslon[6]=gpslon[6];
     memgpslon[7]=gpslon[7];
     }
    return(gps_bigchange);
}



void goapp()
{
//char napis[10];
int i;
unsigned long tt,tt1,gpst1,gpst2;
char memcstation;
unsigned int cs;

char c,key;
char curmem;    //cursor last position

unsigned int mm;
//char station_option=0;  /defined globally for use in station_show
char messages_list_start,messages_list_cur;

//gps memories
char memgpslat[10];
char memgpslon[10];
char gps_bigchange=0;

char station_update =0;
char radar_update   =0;
char gps_update     =0;
char message_update =0;
char list_update    =0;


 welcome();
 tt=utimeval;
 tt1=timeval;
 gpst1=0;
 gpst2=0;
 disp2=DISP_STATION;
 disp1=DISP_LIST;
 //list_info_header();
 list_header(station_list_col,station_list_sort);
 //station_list(1,0);

 messages_list_start=1;
 messages_list_cur=1;

 curmem=station_list_cur;
 update_status=UPDATE_REDRAW;
 cstation=1;
 memcstation=1;
 station_sort(0);
 list_update=UPDATE_REDRAW;
 station_update =UPDATE_REDRAW;

 //main loop
 while(1){

   //test only
   sprintf(napis,"%5d",timeval/10);
   LCDselect(1);
   LCDcolor_foreground=(nightv ? WHITE : BLUE);
   LCDyxtxt(0,124,120,napis);


   if(admin_code==1){
     LCDselect(2);
     LCDclear();
     LCDyxtxt(7,50,10,"Pozdrowienia!");
     admin_code=0;
     c=0;
     while((c<2)||(c>8)){c=read_kbd();}
     update_status|=UPDATE_REFRESH;
     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     gps_update     =UPDATE_REFRESH;
   }

  //cyclic operations




   //my "use" position
   if(((position_mode&0xF0)==0)&&gpsfix&&(timeval>=gpst1)){  //new gps data ready
     gpst1=timeval+50;




     askgpsdata=1;while(askgpsdata==1){waitms(1);};  //read gps buffers
     gps_update     =UPDATE_REFRESH;
     radar_update   =UPDATE_REFRESH;

     gps_bigchange=gps_change();
     if(gps_bigchange){


     aprs_data_flag=1;
     update_bygps();
     recalc_positions();

     update_status|=UPDATE_REFRESH;
     station_update =UPDATE_REFRESH;

     message_update|=0;
     aprs_data_flag=0;
     //odswiezam liste gdy widze odleglosci lub sortowanie wg odleglosci
     if(station_list_col>3){list_update=UPDATE_REFRESH;}
     if((station_list_sort==6)||(station_list_sort==7)){list_update=UPDATE_REFRESH;}
     }



   }
   else
   {
     if((disp2==DISP_GPS)&&(timeval-gpst2>7)){
     //jak updatowac info o GPS FIX jak gps zniknal
     gpst2=timeval;
     if(gpsfix){
      askgpsdata=1;while(askgpsdata==1){waitms(1);};  //read gps buffers
     }
     gps_update     =UPDATE_REFRESH;
     }

     if((disp2==DISP_RADAR)&&(timeval-gpst2>7)){
       gpst2=timeval;
       //sprawdz zmiane kursu
       if(gpsfix){
       askgpsdata=1;while(askgpsdata==1){waitms(1);};
       }
       i=0;
       while((gpscourse[i]>'.')&&(i<3)){i++;}
       if(gpscourse[i]=='.')
       {
         i=0;cs=0;
         while(gpscourse[i]!='.'){
           cs=cs*10;
           cs=cs+gpscourse[i]-'0';
           i++;
         }
         i=cs-mycourse;
         if(i<0){i=-i;}
         if(i>3){mycourse=cs;radar_update   =UPDATE_REFRESH;}
        }
     }
   }

   //check incoming packets
   //aprs_exclusive_flag=1;
   //tc1_poll();
   //aprs_exclusive_flag=0;
   check_receiver();

   if(receive_status){ //so there was a packet in buffer

    //aprs_exclusive_flag must be set here!
    //aprs_exclusive_flag=1;
    //tc1_poll();
    //aprs_exclusive_flag=0;

    check_receiver(); //for double_buffering receivers
#ifdef rsdebug
    printf("app:resort\n");
#endif

    if(((position_mode&0xF0)==0)&&gpsfix){  //new gps data ready

     askgpsdata=1;while(askgpsdata==1){waitms(1);};  //read gps buffers
     gps_bigchange=gps_change();




       aprs_data_flag=1;
       update_bygps();
       recalc_positions();
       gpst1=timeval+70;
       station_update =UPDATE_REFRESH;

       aprs_data_flag=0;



   }

    //resort list
    aprs_data_flag=1;
    station_sort(0);
    aprs_data_flag=0;
    if((station_list_start==1)&&(station_list_cur==1)){
      cstation=sort[1];
    } else
    {
    i=1;
    while((sort[i]!=cstation)&&(i<filtered_stations)){i++;}
    if(sort[i]==cstation){
      station_list_start=i-(station_list_cur-1);
      while(station_list_start<1){
        station_list_start++;
        station_list_cur--;
      }
    }
    else
    {
     station_list_start=1;
     station_list_cur=1;
     cstation=sort[1];
    }
    } //else
    key=0;

     radar_update   =UPDATE_REFRESH;
     if((cstation!=memcstation)||(cstation==lastreceived_num))
      {station_update =UPDATE_REDRAW;}
     gps_update|=0;
     message_update =UPDATE_REFRESH;
     list_update    =UPDATE_REFRESH;

   }//if(receive_status)
   else
   {
   key=read_kbd();
   }




  //check user action

   if(key==3){


     switch(disp2){
     case DISP_STATION :disp2=DISP_RADAR;break;
     case DISP_RADAR   :disp2=DISP_MESSAGES;break;
     case DISP_MESSAGES:disp1=DISP_LIST;disp2=DISP_GPS;list_update=UPDATE_REDRAW;break;
     case DISP_GPS     :disp2=DISP_STATION;break;

     }
     update_status|=UPDATE_REDRAW;

     radar_update   =UPDATE_REDRAW;
     station_update =UPDATE_REDRAW;
     gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;

   }

   if(key==7){

     quick_menu();
     aprs_data_flag=1;
     station_sort(0);
     station_list_start=1;
     station_list_cur=1;
     cstation=sort[1];
     update_status|=UPDATE_REDRAW;

     radar_update   =UPDATE_REDRAW;
     station_update =UPDATE_REDRAW;
     gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
   }

   if(key==17){
     menu();
     update_status|=UPDATE_REDRAW;

     radar_update   =UPDATE_REDRAW;
     station_update =UPDATE_REDRAW;
     gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
   }


   if(disp1==DISP_LIST){

   //goto top
   if(key==13){
     station_list_start=1;
     station_list_cur=1;
     cstation=sort[1];
     update_status|=UPDATE_REDRAW;
     if(station_option>0){station_option=1;}

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
   }

   //goto marked
   if((key==11)&&(marked_station>0)){
     cstation=marked_station;
     i=1;
     while((sort[i]!=cstation)&&(i<filtered_stations)){i++;}
     if(sort[i]==cstation){
      station_list_start=i-(station_list_cur-1);
      while(station_list_start<1){
        station_list_start++;
        station_list_cur--;
      }
    }
    else
    {
     station_list_start=1;
     station_list_cur=1;
     cstation=sort[1];
    }
    if(station_option>0){station_option=1;}
     update_status|=UPDATE_REDRAW;

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
   }

   //pgup
   if(key==12){

    if(station_list_start>6){station_list_start-=6;}
    else{
      if(station_list_start==1){station_list_cur=1;}
      else {station_list_start=1;}
    }
     cstation=sort[station_list_start+station_list_cur-1];
    if(station_option>0){station_option=1;}
     update_status|=UPDATE_REFRESH;

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
   }
   //pgdn
   if(key==18){

    if((station_list_cur+station_list_start)<=(filtered_stations-6))
    {station_list_start+=6;}

     cstation=sort[station_list_start+station_list_cur-1];
    if(station_option>0){station_option=1;}
     update_status|=UPDATE_REFRESH;

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
   }
    //down
    if(key==8){
     if((station_list_cur+station_list_start)<=filtered_stations){
       if(station_list_cur<6){station_list_cur++;}
        else
        {
          station_list_start++;
        }
     }
     cstation=sort[station_list_start+station_list_cur-1];
     if(station_option>0){station_option=1;}
     update_status|=UPDATE_REFRESH;

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
    }
    //up
    if(key==2){
     if(station_list_start>1){station_list_start--;}
     else{
       if(station_list_cur>1){station_list_cur--;}
     }

     cstation=sort[station_list_start+station_list_cur-1];
     if(station_option>0){station_option=1;}
     update_status|=UPDATE_REFRESH;

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
    }

    if(key==6){
     if(station_list_col<8){station_list_col++;}
     list_header(station_list_col,station_list_sort);
     update_status|=UPDATE_REFRESH;
     //radar_update   =UPDATE_REFRESH;
     //station_update =UPDATE_REFRESH;
     //gps_update     =UPDATE_REDRAW;
     //message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
    }

    if(key==4){
     if(station_list_col>0){station_list_col--;}
     list_header(station_list_col,station_list_sort);
     update_status|=UPDATE_REFRESH;
     //radar_update   =UPDATE_REFRESH;
     //station_update =UPDATE_REFRESH;
     //gps_update     =UPDATE_REDRAW;
     //message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
    }

    if(key==5){
     if(station_list_col==station_list_sort)
     {station_list_sort|=0x80;} else
     {station_list_sort=station_list_col;}
     list_header(station_list_col,station_list_sort);
     station_sort(0);

     station_list_start=1;
     station_list_cur=1;
     cstation=sort[1];
     if(station_option>0){station_option=1;}
     update_status|=UPDATE_REDRAW;

     radar_update   =UPDATE_REFRESH;
     station_update =UPDATE_REDRAW;
     //gps_update     =UPDATE_REDRAW;
     message_update =UPDATE_REDRAW;
     list_update    =UPDATE_REFRESH;
    }

   }//if(disp1==DISP_LIST)


   if(disp1==DISP_MESSAGES)
   {
     //down
    if(key==8){
     if((messages_list_cur+messages_list_start)<=count_messages()){
       if(messages_list_cur<6){messages_list_cur++;}
        else
        {
          messages_list_start++;
        }
     }

     message_update =UPDATE_REDRAW;

    }
    //up
    if(key==2){
     if(messages_list_start>1){messages_list_start--;}
     else{
       if(messages_list_cur>1){messages_list_cur--;}
     }
     message_update =UPDATE_REDRAW;

    }
   }

   if(disp2==DISP_MESSAGES)
   {
     if((key==1)&&(count_messages()>0)){
      disp1=DISP_MESSAGES;
      disp2=DISP_MESSAGES;
      message_update =UPDATE_REDRAW;
     }
   }

   if(disp2==DISP_RADAR)
   {

    if(key==1){
     currange++;
     if(currange>4){currange=0;}
     radar_range=rranges[currange];
     //radar(cstation,radar_range,-1);
     radar_update=UPDATE_REDRAW;

    }
   }

   if(disp2==DISP_STATION)
   {

    if(key==1){
      if(station_option){station_option=0;} //frame preview
      else {station_option=1;}
     update_status|=UPDATE_REDRAW;

     station_update =UPDATE_REDRAW;

    }
    if(key==14){
      if(station_option){
        if(station_option<6){
          station_option++;
          station_update =UPDATE_REDRAW;
        }
        update_status|=UPDATE_REDRAW;
      }
    }
    if(key==16){
      if(station_option){
        if(station_option>1){
          station_option--;
          station_update =UPDATE_REDRAW;
        }
        update_status|=UPDATE_REDRAW;
      }
    }
   }

   if(disp2==DISP_GPS)
   {
    if(key==1){
     mycourse=mycourse+10;
     if(mycourse>=360){mycourse=mycourse-360;}
     update_status|=UPDATE_REDRAW;
     gps_update     =UPDATE_REDRAW;
    }

   }

   aprs_data_flag=1;

      //verify announce action
   if((disp1==DISP_LIST)&&(list_update>0)){

     if(list_update==UPDATE_REDRAW){
       LCDselect(1);
       LCDcolor_background=(nightv ? BLACK : WHITE);
       LCDclear();
       list_header(station_list_col,station_list_sort);
     }
      i=1;
      while((i<filtered_stations)&&(sort[i]!=lastreceived_num)){i++;}
      if(((!receive_status)||((sort[i]!=lastreceived_num)&&(announce_filter&0x01)))\
        &&announce_status){station_announce_clear();}
      scrolling_txt_status.active=0;
      list_info_header();
      LCDcolor_foreground=(nightv ? BLACK : WHITE);
      LCDfillrectangle(0,112-10*curmem,4,121-10*curmem);
      LCDyx(112-10*station_list_cur,0);
      LCDcolor_foreground=(nightv ? YELLOW : BLUE);
      LCDchr(0,129);
      //LCDchr(F_SYM_MONO,0x0C);
      curmem=station_list_cur;
      i=station_list(station_list_start,station_list_col,station_list_cur);
   }

   if((disp1==DISP_MESSAGES)&&(message_update>0)){
     if(message_update==UPDATE_REDRAW){
       LCDselect(1);
       LCDclear();
     }
     scrolling_txt_status.active=0;
     messages_show(1,messages_list_start,messages_list_cur);
     LCDyx(116-10*messages_list_cur,0);
      LCDcolor_foreground=(nightv ? YELLOW : BLUE);
      LCDchr(0,129);
   } else
   {
    if((disp2==DISP_MESSAGES)&&(message_update>0)){
     if(message_update==UPDATE_REDRAW){
       LCDselect(2);
       LCDclear();
     }
     messages_show(0,messages_list_start,messages_list_cur);
    }
   }
  //check update action (receive)
   if(disp2==DISP_RADAR){
      LCDselect(2);
      if(radar_update==UPDATE_REDRAW){
        radar(cstation,radar_range,mycourse,1);}
      if(radar_update==UPDATE_REFRESH)
      {radar(cstation,radar_range,mycourse,0);}
    };



/*
    if(disp2==DISP_RADAR){
      LCDselect(2);
      if(update_status&UPDATE_REDRAW){
        radar(cstation,radar_range,mycourse,1);}
      else
      {radar(cstation,radar_range,mycourse,0);}
    };*/
    if(disp2==DISP_STATION){
#ifdef rsdebug
      printf("app:disp_station\n");
#endif
      LCDselect(2);
      if(station_update==UPDATE_REDRAW){station_show(1,cstation,station_option);}
      if(station_update==UPDATE_REFRESH){station_show(0,cstation,station_option);}
      /*
      if((update_status&UPDATE_RECEIVE)&&\
         (lastreceived_num==cstation))\
           {station_show(cstation,station_option);}
      */
    }
/*
    if(disp2==DISP_MESSAGES){
      LCDselect(2);
      if(message_update==UPDATE_REDRAW){messages_show();}
    }
  */
    if(disp2==DISP_GPS){
      LCDselect(2);
      if(gps_update==UPDATE_REDRAW){mi=370;compass_page();compass_page_update();}
      if(gps_update==UPDATE_REFRESH){compass_page_update();}

    }
    update_status=0;


  //announce
   if(receive_status){
#ifdef rsdebug
     printf("app:announce\n");
#endif
    receive_status=0;
    if(stations[lastreceived_num].frame==':')
      {message_announce(lastreceived_num);}
        else
         {station_announce(lastreceived_num);}
   }
  //check gps action



   aprs_data_flag=0;


   if((utimeval-tt)>380)//1/10s
   {
    tt=utimeval;

    if(scrolling_txt_status.active){
      LCDselect(1);
      LCDcolor_foreground=(nightv ? YELLOW : BLUE);
      LCDyx(scrolling_txt_status.y,scrolling_txt_status.x);
      if(scrolling_txt_status.pos>scrolling_txt_status.pose){
      text_scroll(scrolling_txt_status.y,scrolling_txt_status.x,scrolling_txt_status.x1,scrolling_txt,scrolling_txt_status.pose);
      } else {
      text_scroll(scrolling_txt_status.y,scrolling_txt_status.x,scrolling_txt_status.x1,scrolling_txt,scrolling_txt_status.pos);
       }
      scrolling_txt_status.pos++;
      if(scrolling_txt_status.pos-scrolling_txt_status.pose>50){scrolling_txt_status.pos=0;}
    };
   }




    update_status=0;
    radar_update=0;
    station_update=0;
    gps_update=0;
    message_update=0;
    list_update=0;
    memcstation=cstation;

 }  //while
}


