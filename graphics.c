#include "includes.h"

#include "F15x22.h"
#include "aprs.h"
#include "bigsym.h"
#include "digits.h"
#include "f12x16.h"
#include "f15x15.h"
#include "f19x23.h"
#include "f20x23.h"
#include "f23x28.h"
#include "f7x10.h"
#include "f8x10.h"
#include "f8x11.h"
#include "f8x8.h"
#include "f9x14.h"

#include "sym_mono.h"

#include "znaki.h"


char nightv=1;
char disp1;
char disp2;
char scrolling_txt[80];
__scrolling_txt_status scrolling_txt_status;
char announce_status=0;


int istrlen(char *str)
{
 char i;

 i=0;
 while((*str>0)&&(i<54)){str++;i++;}
 return(i);
}


//wyswietla mini kompas z namiarem na stacji
void station_traker(char id,int course)
{
  int x,y,x1,y1,x2,y2,x3,y3;
  float si,co;

  x=cursorx+15;
  y=cursory+15;
  course=stations[id].namiar-mycourse;
  if(course<0){course=course+360;}
  course=-course;                   //clockwise option!
  LCDcircle(cursorx+15,cursory+15,15);
  si=(float)(course)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x1=-15.0*si+x;
  y1=+15.0*co+y;

  si=(float)(course+52)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x2=-7.0*si+x;
  y2=+7.0*co+y;
  si=(float)(course-52)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x3=-7.0*si+x;
  y3=+7.0*co+y;

  LCDline(x1,y1,x2,y2);
  LCDline(x1,y1,x3,y3);
  LCDline(x2,y2,x3,y3);
  si=(float)(course+180)/180.0*3.141592;  	
	co=cos(si);	
	si=sin(si);
  x2=-15.0*si+x;
  y2=+15.0*co+y;
  LCDline(x1,y1,x2,y2);
}

#define KEY_ESC   27
#define KEY_ENTER 13
#define KEY_BKSPC 12
#define KBD_LEFT  129
#define KBD_RIGHT 130
#define KEY_INS   131

char hold=0;  //cursors and bkspc hold
const char normalcharmap[]="QWERTYUIOPASDFGHJKLZXCVBNM0123456789 ,.?@*";
const char shiftcharmap []=":;=@/!#$%^&()-+{}<>_CVBNM0123456789 ,.?@* ";

char get1char(char cc,const char *ss)
{
  char c;
   LCDcolor_foreground=(nightv ? BLACK : WHITE);
   LCDcolor_background=(nightv ? BLACK : WHITE);
   LCDfillrectangle(1,1,174,30);
   LCDcolor_foreground=(nightv ? GREEN : BLACK);
   c=(cc-1)*6;
   LCDyx(20,9);LCDchr(0,ss[c++]);
   LCDyx(20,67);LCDchr(0,ss[c++]);
   LCDyx(20,125);LCDchr(0,ss[c++]);
   LCDyx(10,9);LCDchr(0,ss[c++]);
   LCDyx(10,67);LCDchr(0,ss[c++]);
   LCDyx(10,125);LCDchr(0,ss[c++]);
   c=0;
   while((c==0)||(c>6)){c=read_kbd();}
   c=c-1+(cc-1)*6;
   c=ss[c];
   return(c);
}





char getshiftchar(char cc)
{
  char c;
   LCDcolor_foreground=(nightv ? BLACK : WHITE);
   LCDcolor_background=(nightv ? BLACK : WHITE);
   LCDfillrectangle(1,1,174,30);
   LCDcolor_foreground=(nightv ? GREEN : BLACK);
   LCDyxtxt(0,21,9,":;=@/!");
   LCDyxtxt(0,21,67,"#$%^&(");
   LCDyxtxt(0,21,125,")-+{}<");
   LCDyxtxt(0,11,9,"");
   LCDyxtxt(0,11,67,"");
   LCDyxtxt(0,11,125,"");
   c=0;
   while((c<1)||(c>6)){c=read_kbd();}
   c=get1char(c,shiftcharmap);

   return(c);
}

char read_hex(char y,char x)
{
  char hh[3];
  char c,c1;

  LCDcolor_foreground=(nightv ? BLACK : WHITE);

   LCDcolor_foreground=(nightv ? RED : BLACK);
   LCDyx(y,x);
   LCDtxt(0," .. ");

   hh[0]=0;hh[1]=0;

   c=read_kbdstr(y,x+7,2,0,hh);
   c=hh[0]-'0';if(c>9){c=c-7;}
   c1=hh[1]-'0';if(c1>9){c1=c1-7;}
   c=c*16+c1;
   return(c);
}

char get2char(char y,char x,char opt,char c)  //control keys
{

   LCDcolor_foreground=(nightv ? BLACK : WHITE);
   LCDcolor_background=(nightv ? BLACK : WHITE);
   LCDfillrectangle(1,1,174,30);
   LCDcolor_foreground=(nightv ? GREEN : BLACK);
   LCDyx(21,9);LCDtxt(0,"ESC");
   LCDyx(21,67);LCDtxt(0," INS");
   LCDyx(21,125);LCDtxt(0,"BKSPCE");
   LCDyx(10,9);LCDtxt(0,"LEFT");
   LCDyx(10,67);LCDtxt(0,"ENTER");
   LCDyx(10,125);LCDtxt(0,"RIGHT");
   if(opt){LCDyx(1,9);LCDtxt(0,"HEX");}
   if(hold){LCDyxtxt(0,1,64,"RELEASE");}else{LCDyxtxt(0,1,67," HOLD");}
   //LCDyx(10,9);
   //LCDyx(10,67);
   //LCDyx(10,125);
   c=0;
   while((c<1)||(c>8)){c=read_kbd();}
   switch(c)
   {
   case 4 : c=KEY_LEFT;break;
   case 2 : c=KEY_INS;hold=0;break;
   case 6 : c=KEY_RIGHT;break;
   case 1 : c=KEY_ESC;hold=0;break;
   case 5 : c=KEY_ENTER;hold=0;break;
   case 3 : c=KEY_BKSPC;break;
   case 7 : hold=0;if(opt){c=read_hex(y,x);};break;
   case 8 : hold=1-hold;break;
   }
   return(c);
}


char getkchar(char y, char x,char opt)
{
  char c,c1;

  if(hold){
    c1=get2char(y,x,opt,8);
    if(c1==8){c1=0;};
  } else
  {
 LCDcolor_foreground=(nightv ? YELLOW : WHITE);
 LCDrectangle(0,0,175,31);

   LCDcolor_foreground=(nightv ? BLACK : WHITE);
   LCDcolor_background=(nightv ? BLACK : WHITE);
   LCDfillrectangle(1,1,174,30);

   LCDcolor_foreground=(nightv ? GREEN : WHITE);

   LCDyxtxt(0,21,9,"QWERTY");
   LCDyxtxt(0,21,67,"UIOPAS");
   LCDyxtxt(0,21,125,"DFGHJK");
   LCDyxtxt(0,11,9,"LZXCVB");
   LCDyxtxt(0,11,67,"NM0123");
   LCDyxtxt(0,11,125,"456789");
   LCDyxtxt(0,1,9," ,.?@*");
   LCDyxtxt(0,1,67," CTRL");
   LCDyxtxt(0,1,125," SHIFT");
   c=0;
   while(c==0){c=read_kbd();};


   c1=0;
   if(c==19){ c1=getshiftchar(c);}
   else
   {
    if(c==8){ c1=get2char(y,x,opt,c);if(c1==8){c1=0;};}
    else
    {
      if(c<9){ c1=get1char(c,normalcharmap);}
       else {c1=c;}
    }
   }
  }//if hold
   return(c1);
}

char read_kbdstr(char y,char x,char len,char opt,char *string)
//reads into 'napis' max len znakow
//wartosc funkcji to dlugosc lancucha
//jesli 255 to wyjscie bylo przez ESC
//opt wyswietla kod znaku pod kursorem
{
char c,i,cursor,exit,insmode;
char *copy,*druk;
char fnt=0;
unsigned int mc,mb;
char ss[6];
//char napis[20];

   mc=LCDcolor_foreground;
   mb=LCDcolor_background;
   copy=string;
   cursor=0;
   exit=0;
   //LCDyx(y,x);//LCDtxt(fnt,string);
   //druk=string;
   //while(*druk){LCDchr(fnt,*druk);if(cursorx>130){cursory-=10; LCDyx(cursory,x+5);};druk++;}
   //while((pos<len)&&(*copy)){LCDchr(0,*copy);napis[pos]=*copy++;pos=pos+1;}
   while((cursor<len)&&(*copy)){*copy++;cursor++;}
   *copy=0;   //string end
   insmode=0;
   for(i=cursor+1;i<=len;i++){string[i]=0;}
   //copy=string;
   //LCDchr(0,'_');
   while(!exit)
   {

          LCDyx(y,x);
          LCDcolor_foreground=mc;
          LCDcolor_background=mb;
          if(opt){
            sprintf(ss,"(%02X) ",string[cursor]);
            LCDyxtxt(fnt,y,x,ss);
            if(insmode){LCDtxt(fnt,"INS ");}else{LCDtxt(fnt,"    ");}

          }
			
          druk=string;
          c=0;
          while(*druk){
            if(c==cursor){
             LCDcolor_foreground=mb;
             LCDcolor_background=mc;
            }

            c++;
            LCDchr(fnt,*druk);
            LCDcolor_foreground=mc;
            LCDcolor_background=mb;
            if(cursorx>130){cursory-=10;LCDyx(cursory,x+5);};druk++;
          }
          if(c==cursor){
             LCDcolor_foreground=mb;
             LCDcolor_background=mc;
             LCDchr(fnt,' ');
            }
          LCDcolor_foreground=mc;
          LCDcolor_background=mb;
          LCDchr(fnt,' ');
          if(cursorx>130){cursory-=10;LCDyx(cursory,x+5);LCDchr(fnt,' ');};

	  c=getkchar(y,x,opt);
	
	   switch (c)
	   {
           case 0:break;
           case KEY_INS:insmode=1-insmode;break;
	   case KEY_ENTER : exit=1;break;	 					 //enter
	   case KEY_ESC : exit=2; break;	 					 //enter
           case KEY_BKSPC: if (cursor>0){
                             //copy--;*copy=0;
                             cursor--;
                             for(i=cursor;i<len-1;i++){string[i]=string[i+1];};
                             string[len-1]=0;
                            }//napis[pos]=0;};	//backspace
	
                        break;
           case KEY_LEFT: if(cursor>0){cursor--;}break;
           case KEY_RIGHT:if(string[cursor+1]>0){cursor++;};break;
           default:if(insmode){
                      if(string[len-1]==0){
                        for(i=len-1;i>cursor;i--){string[i]=string[i-1];}
                       string[cursor++]=c;
                      }
                    }
                   else
                   {
	           if((c>0)&&(cursor<len))
			   {
			    string[cursor++]=c;
				
			   }
                   };break;
	   }
	
	
	
   }
   LCDcolor_foreground=mc;
   LCDcolor_background=mb;
   //if(pos<255) //!ESC
   //{
    //LCDyx(y,x);
    //LCDtxt(0,napis);
    //LCDchr(0,' ');
   //}
   i=0;
   while(string[i]&&(i<255)){i++;}
   if(exit==2){i=255;}
   return i;
}

void menu_header(char *str)
{
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    flagfixed=0;

    LCDcolor_foreground=(nightv ? rgb(0,0,15) : BLACK);
    LCDfillrectangle(150,131,175,0);
    LCDcolor_foreground=(nightv ? WHITE : RED);
    LCDyx(95,151);pchar(F_20X23,1,'A',0);
    LCDyx(65,153);pchar(F_20X23,1,'P',0);
    LCDyx(35,153);pchar(F_20X23,1,'R',0);
    LCDyx(5,153);pchar(F_20X23,1,'S',0);
    LCDcolor_background=(nightv ? rgb(0,0,15) : BLACK);
    LCDyx(105,171);LCDchr(F_SMALL,'F');
    LCDyx(97,171);LCDchr(F_SMALL,'H');
    LCDyx(89,172);LCDchr(F_SMALL,'I');
    LCDyx(78,171);LCDchr(F_SMALL,'N');
    LCDyx(70,171);LCDchr(F_SMALL,'A');
    LCDyx(62,171);LCDchr(F_SMALL,'V');
    LCDyx(54,172);LCDchr(F_SMALL,'I');
    LCDyx(46,170);LCDchr(F_SMALL,'G');
    LCDyx(38,171);LCDchr(F_SMALL,'A');
    LCDyx(30,171);LCDchr(F_SMALL,'T');
    LCDyx(22,170);LCDchr(F_SMALL,'O');
    LCDyx(14,171);LCDchr(F_SMALL,'R');
      LCDcolor_foreground=(nightv ? WHITE : BLACK);
      LCDcolor_background=(nightv ? BLACK : WHITE);

    LCDline(0,119,5,119);
    LCDline(0,122,5,122);
    LCDline(0,125,5,125);
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    LCDyxtxt(6,116,7,str);
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDline(cursorx,119,175,119);
    LCDline(cursorx,122,175,122);
    LCDline(cursorx,125,175,125);
}

void packet_stats()
{
  unsigned long tt;

  char c,i,exit;
  int sum,max;

  LCDselect(2);
  exit=0;

    while(!exit)
    {
    tt=timeval;
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    flagfixed=0;
    LCDyxtxt(6,115,5,"Packet traffic stats ");
    LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);

    LCDrectangle(20,40,142,100);
    //LCDcolor_foreground=(nightv ? GREY : BLACK);
    //LCDrectangle(20,40,142,100);
    LCDline(20,60,142,60);
    LCDline(20,80,142,80);
    for(c=0;c<5;c++)LCDline(40+20*c,40,40+20*c,100);
    //LCDrectangle(20,10,140,50);
    //LCDline(20,30,140,30);
    //for(c=0;c<5;c++)LCDline(40+20*c,10,40+20*c,50);
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDyxtxt(F_SMALL,102,20,"LAST 60 MINUTES");

    //calculate max
    max=0;
    for(c=0;c<60;c++){if(traffic[c]>max){max=traffic[c];};}
    if(max>29){
    LCDyxtxt(F_SMALL,58,146,"20");
    LCDyxtxt(F_SMALL,78,146,"40");
    LCDyxtxt(F_SMALL,98,146,"60");
    } else {
    LCDyxtxt(F_SMALL,58,146,"10");
    LCDyxtxt(F_SMALL,78,146,"20");
    LCDyxtxt(F_SMALL,98,146,"30");
    }
    LCDyxtxt(F_SMALL,31,20,"TOTAL");
    LCDyxtxt(F_SMALL,31,70,"UNIQUE");
    LCDyxtxt(F_SMALL,31,120,"DIGI");

    sum=0;
    for(c=0;c<60;c++){sum=sum+traffic[c];}
    sprintf(napis,"%d",sum);
    LCDyxtxt(F_SMALL,23,20,napis);

    sum=0;
    for(c=0;c<60;c++){sum=sum+utraffic[c];}
    sprintf(napis,"%d",sum);
    LCDyxtxt(F_SMALL,23,70,napis);

    sum=0;
    for(c=0;c<60;c++){sum=sum+dtraffic[c];}
    sprintf(napis,"%d",sum);
    LCDyxtxt(F_SMALL,23,120,napis);

    LCDcolor_foreground=(nightv ? RED : BLACK);
    LCDfillrectangle(10,30,15,35);
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    LCDfillrectangle(60,30,65,35);
    LCDcolor_foreground=(nightv ? GREEN : BLACK);
    LCDfillrectangle(110,30,115,35);


    if(max>29){max=1;}else{max=2;}
    for(c=0;c<60;c++){
     LCDcolor_foreground=(nightv ? RED : BLACK);
     LCDrectangle(140-2*c,40,140-2*c,40+max*traffic[c]);
     LCDcolor_foreground=(nightv ? YELLOW : BLACK);
     LCDrectangle(140-2*c,40,140-2*c,40+max*utraffic[c]);
     LCDcolor_foreground=(nightv ? GREEN : BLACK);
     LCDrectangle(140-2*c,40,140-2*c,40+max*dtraffic[c]);
    }

    //wykres 20hr

    LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);

    LCDrectangle(21,0,142,20);

    LCDline(21,10,142,10);
    for(c=0;c<19;c++)LCDline(27+6*c,0,27+6*c,20);
    max=0;
    for(c=0;c<120;c++){if(htraffic[c]>max){max=htraffic[c];};}
    LCDcolor_foreground=(nightv ? LIGHT_BLUE : BLACK);
    for(c=0;c<120;c++){
      sum=htraffic[c]*20;
      if(max>0){sum=sum/max;}else{sum=0;}
      if(htraffic[c]>0){LCDline(141-c,0,141-c,sum);}
    }
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDyxtxt(F_SMALL,12,2,"LAST");
    LCDyxtxt(F_SMALL,4,2,"20HR");
    sprintf(napis,"%d",max);
    LCDyxtxt(F_SMALL,17,145,napis);
    c=0;
    while(((c<2)||(c>8))&&(timeval-tt<600)){c=read_kbd();}
    if(c==7){exit=1;}
    }
}

void send_stats()
{
#ifdef nouse
  int sum,i;
 U0LCR = 0x83;
 U0DLL = 64;                     // 57600
 U0DLM = 0;
 U0LCR = 0x03;
 printf("\nTraffic stats\n");
      printf("\nLast 60 minutes (tot,unique,digi)\n");
      for(i=0;i<60;i++){
        printf("%3d,%3d,%3d,\n",traffic[i],utraffic[i],dtraffic[i]);
      }
      printf("\nLast 20 hours (minutes,packets)\n");
      sum=0;
      for(i=0;i<120;i++){
        sum=i*10;
        printf("%d,%d,\n",sum,htraffic[i]);
      }

 //restore serial
 if(serial0_opt==0)
 {

  U0LCR = 0x83;
  U0DLM = 3;
  U0DLL = 0;                    // 4800
  U0LCR = 0x03;
 }
#endif 
}



void digi_stats()
{
  char i,c,exit;
  char ss[6];
  unsigned long tt;

  LCDselect(2);

    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    exit=0;
    while(!exit){
    menu_header("My digi list");
    tt=memdigi[0].time;
    flagfixed=0;
    for(c=0;c<6;c++){
     LCDyx(90-10*c,5);
     if(memdigi[c].status){
       LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
       LCDtxt(0,"(");
       for(i=0;i<6;i++){
         LCDchr(0,aliases[memdigi[c].status-1].alias[i]);
       }
       LCDtxt(0,") ");


       for(i=0;i<6;i++){LCDchr(0,memdigi[c].alias[i]);}

       i=memdigi[c].alias[6];
       if(i>0){
         sprintf(ss,"-%d",i);
         LCDtxt(0,ss);
       }

     }
    }
    c=0;
    while((c!=7)&&(tt==memdigi[0].time)){c=read_kbd();}
    if(c==7){exit=1;}
     else {waitms(2);} //list change
  }
}

void frame_save()
{
  char c,i;

  LCDselect(2);

    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    flagfixed=0;
    menu_header("BUFFER EDIT");
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    slot_lock=stations[cstation].slot;
    flash_read(napis,stations[cstation].slot,255);
    for(i=0;i<250;i++){
      frame[i]=napis[i+1];
    }
    frame[249]=0;
    slot_lock=0;
    LCDyxtxt(7,85,5,"Frame saved");
    c=0;
    while(c==0){c=read_kbd();}
}


void edit_frame()
{
  char c,i,y,chg;
  LCDselect(2);

    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    flagfixed=0;
    menu_header("BUFFER EDIT");
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);



    for(i=0;i<250;i++){napis[i]=frame[i];}
    chg=read_kbdstr(108,5,240,1,napis);
    if(chg!=255){
      for(i=0;i<250;i++){frame[i]=napis[i];}
    }
    menu_header("FRAME SEND");
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    y=95;

   LCDyx(y,2);
   i=0;
   while((frame[i]>0)&&(y>0)&&(i<240))
   {
    LCDchrt(7,frame[i]);
    if(cursorx>145){
      y=y-10;//10
      LCDyx(y,2);
    }
    i++;
   }
   LCDcolor_foreground=(nightv ? WHITE : BLACK);
   LCDyx(5,1);
   pchar(F_SYM_MONO,1,ICONKEY6,2);LCDtxt(0,"SEND");

   if(system_userlevel>1){return;}
    c=0;
    while(c==0){c=read_kbd();}
    if(c==6){
     //send
      application_frame_send(frame);
    }


}

void my_frame()
{
  char c,i,y;
  LCDselect(2);

    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    flagfixed=0;
    LCDyxtxt(6,115,5,"Last beacon frame ");
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    y=78;
    if(stations[0].slot>0){
    flash_read(napis,stations[0].slot,255);
    //header
   LCDyx(y,2);
   i=0;
   while((napis[i]!='#')&&(y>0)&&(i<200))
   {
    LCDchrt(7,napis[i]);
    if(cursorx>145){
      y=y-10;//10
      LCDyx(y,2);
    }
    i++;
   }
   //body
   LCDcolor_foreground=(nightv ? WHITE : BLACK);
   i++;
   i++;
   y=y-10;//10
   LCDyx(y,2);
   while((napis[i]>0)&&(y>0)&&(i<250))
   {
    LCDchrt(7,napis[i]);
    if(cursorx>145){
      y=y-10;//10
      LCDyx(y,2);
    }
    i++;
   }
    } //slot>0
    else
    {
     LCDyxtxt(7,y,2,"No frame yet");
    }
    LCDyx(5,1);
    pchar(F_SYM_MONO,1,ICONKEY6,2);LCDtxt(0,"TO BUFFER");
    c=0;
    while(c!=7){c=read_kbd();}
    if(c==6){
      for(i=0;i<250;i++){buffer[i]=napis[i];}
    }
}


void show_digisens_list()
{
  char c,i,exit;
  unsigned long tt;
  char ss[5];

  exit=0;
  LCDselect(2);
  while(!exit){
    menu_header("Digisens list");
    tt=selfdigis[0].time;
    for(c=0;c<6;c++){
     LCDyx(90-10*c,5);
     if(selfdigis[c].status){
       if(selfdigis[c].status>1) //old?
       {LCDcolor_foreground=(nightv ? GREY : BLACK);}
       else
       {LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);};
       for(i=0;i<6;i++){LCDchr(0,selfdigis[c].alias[i]);}
       i=selfdigis[c].alias[6];
       if(i>0){
         sprintf(ss,"-%d",i);
         LCDtxt(0,ss);
       }
     }
    }
    c=0;
    while((c!=7)&&(tt==selfdigis[0].time)){c=read_kbd();}
    if(c==7){exit=1;}
     else {waitms(2);} //list change
  }
}

void show_station_status()
{
  char c;
  LCDselect(2);

    LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    LCDclear();
    flagfixed=0;
    LCDyxtxt(6,115,2,"Station status ");
    for(c=0;c<6;c++){LCDchr(6,stations[0].sign[c]);}
    if(stations[0].ssid>0){
     sprintf(napis,"-%d",stations[0].ssid);
     LCDtxt(6,napis);
    }
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    sprintf(napis,"Packets received:%d",packet_count);
    LCDyxtxt(0,100,5,napis);
    sprintf(napis,"Unique received:%d",unique_packet_count);
    LCDyxtxt(0,90,5,napis);
    sprintf(napis,"Digi in range:");
    LCDyxtxt(0,80,5,napis);
    if(self_digipeated){LCDtxt(0,"YES");}else{LCDtxt(0,"??");}
    sprintf(napis,"Beacons sent:%d",my_beacon_count);
    LCDyxtxt(0,70,5,napis);
    sprintf(napis,"Self received:%d",stations[0].count);
    LCDyxtxt(0,60,5,napis);
    sprintf(napis,"Messages received:%d",message_count);
    LCDyxtxt(0,50,5,napis);
    sprintf(napis,"Invalid received:%d",invalid_count);
    LCDyxtxt(0,40,5,napis);
    //pokaz godzine
    //(...)



    c=0;
    while(c!=7){c=read_kbd();}
}

void spos(char *str)
{
  char c,c1;
  if(str[6]&0xF0>0){c='S';}else{c='N';}  //S
  if(str[6]&0x0F>0){c1='W';}else{c1='E';}
  sprintf(napis,"%02d.%02d%02d%c %03d.%02d%02d%c",str[0],\
   str[1],str[2],c,str[3],\
     str[4],str[5],c1);
 LCDtxt(0,napis);
}

void position_select()
{
  char chg,c,i,cur,exit;
  char ss[8];

  LCDselect(2);
  exit=0;
  cur=0;
  chg=0;
  while(!exit)
  {
  menu_header("Static position");
  switch(cur)
  {

  case 0:LCDrectangle(0,79,145,89);break;
  case 1:LCDrectangle(0,69,145,79);break;
  case 2:LCDrectangle(0,59,145,69);break;

  }

 LCDyxtxt(0,103,3,"Current ");
 LCDcolor_foreground=(nightv ? RED : RED);
 if((position_mode&0xF0)==0){
    LCDtxt(0,"AUTO ");
    if(gpsfix){LCDtxt(0,"GPS");}
 } else{
   LCDtxt(0,"STATIC");
 }
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 LCDyx(93,5);
 spos(stations[0].position);

 LCDyx(80,5);spos(posbank1);
 LCDyx(70,5);spos(posbank2);
 LCDyx(60,5);spos(posbank3);


    LCDyx(80-10*posuse,130);
    pchar(F_ZNAKI,1,0x08,0);


/*
 switch(position_mode){
 case 0: LCDyxtxt(0,45,5,"Send: by GPS/static");break;
 case 1: LCDyxtxt(0,45,5,"Send: by GPS only  ");break;
 case 2: LCDyxtxt(0,45,5,"Send: static only  ");break;
 }
*/
 LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
 pchar(F_SYM_MONO,1,0x13,2);
 LCDtxt(0,"SELECT ");
 pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"USE ");

 LCDyx(5,1);
 pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"SET TO CURRENT");

  c=0;
    while((c<1)||(c>8)){c=read_kbd();}
  switch(c)
  {
  case 2: if(cur>0){cur--;}break;
  case 8: if(cur<2){cur++;}break;
  case 6: for(i=0;i<7;i++){

    if(cur==0)posbank1[i]=stations[0].position[i];
    if(cur==1)posbank2[i]=stations[0].position[i];
    if(cur==2)posbank3[i]=stations[0].position[i];

          }
    break;
  //case 5: exit=1;break;
  case 7: exit=1;break;
  case 5: if((position_mode&0xF0)>0)
        switch(cur){
  //case 1:break;//no change
        case 0:for(c=0;c<7;c++){stations[0].position[c]=posbank1[c];}break;
        case 1:for(c=0;c<7;c++){stations[0].position[c]=posbank2[c];}break;
        case 2:for(c=0;c<7;c++){stations[0].position[c]=posbank3[c];}break;
        };
        break;
  case 1:posuse=cur;break;
  case 4:menu_header("POSITION DEFINE");
              LCDyxtxt(0,90,10,"Input position");
              LCDyxtxt(0,80,10,"Format:");
              LCDyxtxt(0,70,10,"52.2581N 016.3215E");

              napis[0]=0;
              i=read_kbdstr(55,10,18,0,napis);
              if(i<255){
               c=10*(napis[0]-'0')+napis[1]-'0';
               ss[0]=c;
               c=10*(napis[3]-'0')+napis[4]-'0';
               ss[1]=c;
               c=10*(napis[5]-'0')+napis[6]-'0';
               ss[2]=c;
               c=100*(napis[9]-'0')+10*(napis[10]-'0')+napis[11]-'0';
               ss[3]=c;
               c=10*(napis[13]-'0')+napis[14]-'0';
               ss[4]=c;
               c=10*(napis[15]-'0')+napis[16]-'0';
               ss[5]=c;
               ss[6]=0;
               if(napis[7]=='S'){ss[6]=0x10;}
               if(napis[17]=='W'){ss[6]|=0x01;}
               for(i=0;i<7;i++){
                switch(cur){
                case 0:posbank1[i]=ss[i];break;
                case 1:posbank2[i]=ss[i];break;
                case 2:posbank3[i]=ss[i];break;
                }
               }
              };
              break;
  }
  } //while !exit


  aprs_data_flag=1;
  recalc_positions();
  update_config();

}


void delete_all_messages()
{
char i,c;

  menu_header("DELETE ALL");
  LCDyxtxt(7,50,10,"Are you sure?");
  LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x10,2);LCDtxt(0,"CHANGE");
  c=0;
   while((c<2)||(c>8)){c=read_kbd();}
  if(c==4)
  {
  aprs_data_flag=1;
  for(i=0;i<MAX_MESSAGES;i++){
   messages[i].status=0;
  }
  }
}

void delete_read_messages()
{
char i,c;
  menu_header("DELETE READ");
  LCDyxtxt(7,50,10,"Are you sure?");
  LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x10,2);LCDtxt(0,"CHANGE");
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   if(c==4){

  aprs_data_flag=1;
  for(i=0;i<MAX_MESSAGES;i++){
    if(messages[i].status==2){messages[i].status=0;}
  }
   }
}

void show_options_menu()
{
  char c;


  LCDselect(2);

    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    menu_header("ADD-ONS");


    LCDcolor_foreground=(nightv ? WHITE : BLACK);


    LCDyx(97,1);pchar(F_SYM_MONO,1,0x0D,3);LCDtxt(6,"Traffic");
    LCDyx(81,1);pchar(F_SYM_MONO,1,0x0E,3);LCDtxt(6,"Recent digi list");
    LCDyx(65,1);pchar(F_SYM_MONO,1,0x0F,3);LCDtxt(6,"My last beacon");
    LCDyx(49,1);pchar(F_SYM_MONO,1,0x10,3);LCDtxt(6,"Frame to buffer");
    LCDyx(33,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"Edit frame");
    //LCDyx(17,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"?");
    //LCDyx(0,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"?");


    c=0;
    while((c<1)||(c>8)){c=read_kbd();}


    switch(c){  //wspolne
    case 1:packet_stats();break;
    case 2:digi_stats();break;
    case 3:my_frame();break;
    case 4:frame_save();break;
    case 5:edit_frame();break;

    }

}


void send_message1()
{
  char predef,i,c,spos,exit,stp;
  char mst[52];
  char st[11];
  char ss[10];
  unsigned long told;


  LCDselect(2);
  exit=0;
  predef=0;
  while(!exit)
  {
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    menu_header("SEND MESSAGE");
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    LCDyxtxt(7,100,5,"TO: ");
    cursory-=4;
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    i=0;stp=0;
    while((i<6)&&(stations[cstation].sign[i]>' '))
    {
      LCDchr(6,stations[cstation].sign[i]);
      st[stp++]=stations[cstation].sign[i];
      i++;
    }
    if(stations[cstation].ssid>0){
     sprintf(napis,"-%d",stations[cstation].ssid);
     i=0;
     while(napis[i]>0){st[stp++]=napis[i++];}
     LCDtxt(6,napis);
    }
    while(stp<9){st[stp++]=' ';}
    st[stp]=0;
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    LCDyxtxt(7,80,5,"MSG:");
    switch(predef){
    case 0: sprintf(mst,"Select/enter text");break;
    //case 1: sprintf(mst,"Witam!");break;
    //case 2: sprintf(mst,"Jestem w drodze, odezwe sie pozniej!");break;
    //case 3: sprintf(mst,"Co slychac?");break;
    //case 4: sprintf(mst,"Pozdrawiam");break;
    //case 5: sprintf(mst,"Do widzenia 73!");break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:for(i=0;i<50;i++){mst[i]=msgpredef[predef-1].text[i];};mst[50]=0;
                   break;
    }
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    if(predef==0){LCDcolor_foreground=(nightv ? GREY : BLACK);}
    i=0;
    while(mst[i]>0){
      LCDchr(7,mst[i]);
      i++;
      if(cursorx>140){cursory-=10;cursorx=5;}
    }
    c=0;
    while((c<1)||(c>8)){c=read_kbd();}
    switch(c)
    {
    case 2:if(predef>0){predef--;};break;
    case 8:if(predef<5){predef++;}break;
    case 5:exit=1;break;
    case 7:return;
    case 4:if(predef==0){sprintf(mst,"");}
           LCDcolor_foreground=BLACK;
           LCDfillrectangle(5,90,145,40);
           LCDcolor_foreground=(nightv ? WHITE : BLACK);
           LCDyxtxt(7,80,5,"MSG:");

           i=read_kbdstr(80,40,50,0,mst);
           if(i<255)
              {
                predef=6;

              };
           break;
    }
  }
  LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
  if(predef==0){return;}

  LCDyxtxt(7,40,5,"Do you really want");
  LCDyxtxt(7,30,5,"send this message?");

  c=0;
    while((c<1)||(c>8)){c=read_kbd();}
    if(c!=5){return;}

    sprintf(napis,"xxxxxxxxxxxxxxRELAY 0WIDE  0# =");

  for(c=0;c<6;c++){napis[c]=udest[c];}
  napis[6]='0';
  for(c=0;c<6;c++){napis[7+c]=stations[0].sign[c];}
  napis[13]=stations[0].ssid+'0';
  spos=14;
  //select next valid path or no path
  c=msgpathuse+1;
  if(c>2){c=0;}
  while((c!=msgpathuse)&&(!paths[c].msgenable)){
    c++;
    if(c>2){c=0;}
  }
  if(paths[c].msgenable)
  {
    i=0;
    while(i<7*paths[c].plen)
    {
      napis[spos]=paths[c].spath[i];
      spos++;
      i++;
    }
    msgpathuse=c; //zapisz co bylo uzyte
  }
  napis[spos++]='#';
  napis[spos++]=' ';

  napis[spos++]=':';
  i=0;
  while(st[i]){napis[spos++]=st[i++];}
  napis[spos++]=':';
  i=0;
  while(mst[i]>0){napis[spos++]=mst[i++];}

  //msg id
  msgid++;
  sprintf(ss,"%d      ",msgid);
  napis[spos++]='{';
  i=0;
  while(ss[i]>0){napis[spos++]=ss[i++];}
  napis[spos++]=0;
  //szukaj pusty slot
  i=0;
  while((i<TX_MESSAGEBUF)&&(txmessages[i].status>0)){i++;}
  if(i==TX_MESSAGEBUF)
  {
    //szukaj wyslany i najstarszy
    i=0;c=TX_MESSAGEBUF;told=timeval+1000;
    while(i<TX_MESSAGEBUF){
      if((txmessages[i].status>1)&&(txmessages[i].time<told))
       {c=i;told=txmessages[i].time;}
      i++;
    }
    i=c;
  }
  if(i!=TX_MESSAGEBUF)
  {
    txmessages[i].slot=get_freepage();
    flash_write(napis,txmessages[i].slot,255);
    txmessages[i].ackstatus=0;
    txmessages[i].repcount=5;
    txmessages[i].time=timeval+10; //1sec
    txmessages[i].status=1;
    for(c=0;c<5;c++)
     {
       if(ss[c]>0){txmessages[i].ackid[c]=ss[c];}
        else{txmessages[i].ackid[c]=' ';};
     }
#ifdef msgdebug
    printf("ACK is:%s:\n",txmessages[i].ackid);
#endif
  }
  else
  {
    debugmsg(WARNLEVEL,"TX message buf full");
  }
  if(predef==6){
    menu_header("TEXT MANAGER");
    LCDyxtxt(7,100,5,"Save this text?");
    LCDyx(88,5);
    LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
    i=0;
    while(mst[i]>0){
      LCDchr(7,mst[i]);
      i++;
      if(cursorx>140){cursory-=10;cursorx=5;}
    }
    c=0;
    while((c<1)||(c>8)){c=read_kbd();}
    if(c==5){
      predef=0;
      exit=0;
      while(!exit){
      menu_header("TEXT MANAGER");
      LCDcolor_foreground=(nightv ? WHITE : BLACK);
      LCDyxtxt(7,100,5,"Save this text?");
      LCDyx(88,5);
      LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
      i=0;
      while(mst[i]>0){
        LCDchr(7,mst[i]);
        i++;
        if(cursorx>140){cursory-=10;cursorx=5;}
      }
      LCDcolor_foreground=(nightv ? WHITE : BLACK);
      LCDyxtxt(7,70,5,"Select memory");
      LCDyxtxt(7,60,5,"MSG:");
      LCDyx(45,5);
      i=0;
      while((msgpredef[predef].text[i]>0)&&(i<50)){
        LCDchr(7,msgpredef[predef].text[i]);
        i++;
        if(cursorx>140){cursory-=10;cursorx=5;}
      }
        c=0;
        while((c<1)||(c>8)){c=read_kbd();}
        switch(c)
        {
        case 7:exit=1;break;
        case 5:exit=1;for(i=0;i<50;i++){msgpredef[predef].text[i]=mst[i];};update_config();break;
        case 2:if(predef>0){predef--;}break;
        case 8:if(predef<4){predef++;}break;
        }
      }
    }//if c
  }//if predef

}



void txmessages_show()
{
char c,i,y,k,sorted;
char ur,mm;
unsigned long tm;
char msort[TX_MESSAGEBUF];

#ifdef msgdebug
      printf("Messages show \n");
#endif


 LCDselect(2);
 LCDclear();
 flagfixed=0;
 LCDcolor_background=(nightv ? BLACK : WHITE);
 LCDcolor_foreground=(nightv ? WHITE : BLACK);

 mm=0;
 ur=0;
 for(i=0;i<TX_MESSAGEBUF;i++){
   if(txmessages[i].status>0){mm++;}
   msort[i]=0;
   //if(messages[i].status==1){ur++;}
 }


 LCDyxtxt(0,124,1,"SENT MESSAGES");
 LCDline(0,123,176,123);
 i=0;
 y=105;
 if(mm==0){
  LCDyxtxt(7,105,7,"No messages sent");
 }
 mm=0;
 while(y>0)
 {

   //select newest
   tm=0;
   sorted=255;
   for(k=0;k<TX_MESSAGEBUF;k++){
     if((txmessages[k].status>0)&&(txmessages[k].time>tm)&&(msort[k]==0))
      {tm=txmessages[k].time;
       sorted=k;
      }
   }
   if(sorted<255){
     msort[sorted]=1;
     i=sorted;
     mm++;
     if(1)
     {
     LCDyx(y+1,0);
     if(txmessages[i].status==1){pchar(F_ZNAKI,1,0x24,0);};
     if((txmessages[i].status>1)&&(txmessages[i].ackstatus==0))
      {pchar(F_ZNAKI,1,0x23,0);};
     if((txmessages[i].status>1)&&(txmessages[i].ackstatus==1))
      {pchar(F_ZNAKI,1,0x22,0);};
     LCDyx(y,6);


     LCDcolor_foreground=(nightv ? YELLOW : BLACK);
     LCDfixed(1);
     LCDtxt(F_8X10,"TO:");


     //retrieve from eeprom
     flash_read(napis,txmessages[i].slot,255);


       //pchar(F_8X10,0,'>',-1);
       k=1;
       while((napis[k]!='#')&&(k<100)){k++;}
       k=k+3;
       c=0;
       while((c<6)&&(napis[k+c]!='-')&&(napis[k+c]>' '))
        {pchar(F_8X10,0,napis[k+c],-1);c++;}

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




     y=y-10;
     }//if mm
   }//if sorted
   else
   {
     y=0;
   }

 }//while
 c=0;
    while((c<1)||(c>8)){c=read_kbd();}

}

void serial_monitor()
{
char c;
  LCDselect(2);
 LCDclear();
 menu_header("SERIAL MONITOR");
 flagfixed=0;
 c=0;
 while(c!=7){
  c=0;
  while(c==0){c=read_kbd();}
  sprintf(napis,"#%03d  %c  ",c,c);
  LCDyxtxt(6,80,10,napis);
 }
}

void gps_monitor()
{
char c,exit;
  LCDselect(2);
 LCDclear();
 menu_header("GPS MONITOR");
 flagfixed=0;
 exit=0;
 while(!exit){
   if(gpsvalid){
    askgpsdata=1;while(askgpsdata==1){waitms(1);};
    gpsvalid=0;
    LCDyx(90,5);LCDtxt(0,"SPEED  ");
    c=0;
    while((c<6)&&(gpsspeed[c]>0)){
      LCDchr(0,gpsspeed[c]);
      c++;
    }
    LCDyx(80,5);LCDtxt(0,"COURSE ");
    c=0;
    while((c<6)&&(gpscourse[c]>0)){
      LCDchr(0,gpscourse[c]);
      c++;
    }
    LCDyx(70,5);LCDtxt(0,"LAT    ");
    for(c=0;c<8;c++){
      LCDchr(0,gpslat[c]);
    }
    LCDyx(60,5);LCDtxt(0,"LON    ");
    for(c=0;c<9;c++){
      LCDchr(0,gpslon[c]);
    }

   }
  c=read_kbd();
  if(c==7){exit=1;}
 }

}


void radar_setrange()
{
  if(stations[cstation].dist<999999){
    radar_range=(stations[cstation].dist+stations[cstation].dist/10)/10+1;
    if(radar_range>999){radar_range=999;}
  }
}


void quick_menu()
{
  char c;


  LCDselect(2);

    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDcolor_background=(nightv ? BLACK : WHITE);
    menu_header("QUICK MENU");


    LCDcolor_foreground=(nightv ? WHITE : BLACK);

    if(disp2==DISP_RADAR)
    {
    LCDyx(97,1);pchar(F_SYM_MONO,1,0x0D,3);LCDtxt(6,"Send beacon");
    LCDyx(81,1);pchar(F_SYM_MONO,1,0x0E,3);LCDtxt(6,"Capture position");
    LCDyx(65,1);pchar(F_SYM_MONO,1,0x0F,3);LCDtxt(6,"Station mark");
    LCDyx(49,1);pchar(F_SYM_MONO,1,0x10,3);LCDtxt(6,"Position select");
    LCDyx(33,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"Radar range set");
    //LCDyx(17,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"-");
    //LCDyx(0,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"-");
    }
    if(disp2==DISP_STATION)
    {
    LCDyx(95,1);pchar(F_SYM_MONO,1,0x0D,3);LCDtxt(6,"Send beacon");
    LCDyx(76,1);pchar(F_SYM_MONO,1,0x0E,3);LCDtxt(6,"Capture position");
    LCDyx(57,1);pchar(F_SYM_MONO,1,0x0F,3);LCDtxt(6,"Station mark");
    LCDyx(38,1);pchar(F_SYM_MONO,1,0x10,3);LCDtxt(6,"Position select");
    LCDyx(19,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"Station status");
    LCDyx(0,1); pchar(F_SYM_MONO,1,0x12,3);LCDtxt(6,"Digisens list");
    }
    if((disp2==DISP_MESSAGES)&&(disp1==DISP_MESSAGES))
    {
    LCDyx(95,1);pchar(F_SYM_MONO,1,0x0D,3);LCDtxt(6,"Send message");
    LCDyx(76,1);pchar(F_SYM_MONO,1,0x0E,3);LCDtxt(6,"Delete all");
    LCDyx(57,1);pchar(F_SYM_MONO,1,0x0F,3);LCDtxt(6,"Delete read");
    LCDyx(38,1);pchar(F_SYM_MONO,1,0x10,3);LCDtxt(6,"Delete message");
    LCDyx(19,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"Lock message");
    //LCDyx(0,1); pchar(F_SYM_MONO,1,0x12,3);LCDtxt(6,"Station status");
    }
    if((disp2==DISP_MESSAGES)&&(disp1!=DISP_MESSAGES))
    {
    LCDyx(95,1);pchar(F_SYM_MONO,1,0x0D,3);LCDtxt(6,"Send message");
    LCDyx(76,1);pchar(F_SYM_MONO,1,0x0E,3);LCDtxt(6,"Delete all");
    LCDyx(57,1);pchar(F_SYM_MONO,1,0x0F,3);LCDtxt(6,"Delete read");
    LCDyx(38,1);pchar(F_SYM_MONO,1,0x10,3);LCDtxt(6,"Show sent");
    //LCDyx(19,1);pchar(F_SYM_MONO,1,0x11,3);LCDtxt(6,"Lock message");
    //LCDyx(0,1); pchar(F_SYM_MONO,1,0x12,3);LCDtxt(6,"Station status");
    }
    if(disp2==DISP_GPS)
    {
    LCDyx(95,1);pchar(F_SYM_MONO,1,0x0D,3);LCDtxt(6,"GPS Monitor");

    }

    c=0;
    while((c<1)||(c>8)){c=read_kbd();}

    if(disp2==DISP_STATION)
      {
       switch(c){
        case 1:next_beacon_time=timeval;break;
        case 2:capture_position(cstation);break;
        case 3:if(marked_station==cstation){marked_station=0;}else
                {marked_station=cstation;};break;
        case 4:position_select();break;
        case 8:show_options_menu();break;

        case 5:show_station_status();break;
       case 6:show_digisens_list();break;
        }
      }
    if(disp2==DISP_RADAR)
      {
       switch(c){
        case 1:next_beacon_time=timeval;break;
        case 2:capture_position(cstation);break;
        case 3:if(marked_station==cstation){marked_station=0;}else
                {marked_station=cstation;};break;
        case 4:position_select();break;
       case 5:radar_setrange();break;
       }
      }
    if((disp2==DISP_MESSAGES)&&(disp1!=DISP_MESSAGES))
      {
       switch(c){
        case 1:send_message1();break;
        case 2:delete_all_messages();break;
        case 3:delete_read_messages();break;
        case 4:txmessages_show();break;
       }
      }

    if(disp2==DISP_GPS)
    {
      switch(c){
      case 1:gps_monitor();break;
      }
    }

}



void gomenu(char m1, char m2)
{
  int mm;

  flagfixed=0;
  mm=m1*10+m2;
  switch(mm){
  case 11 : config_announce();break;
  case 12 : config_position();break;
  case 13 : config_symbol();break;
  case 14 : config_suffix();break;
  case 15 : config_contacts();break;

  case 21 : config_path();break;
  case 22 : config_status();break;
  case 23 : config_beacon_delay();break;
  case 24 : config_navmark();break;
  case 25 : config_dest();break;

  case 31 : config_filter();break;
  case 41 : config_ranges();break;

  case 51 : config_messages_receive();break;
  case 52 : config_messages_option();break;

  case 61 : config_sounds();break;
  case 62 : config_txlock();break;
  case 63 : config_trx();break;
  case 64 : config_frames();break;
  case 65 : config_factory();break;

  case 71: config_aliases();break;
  case 72: config_digi_delay();break;

  case 81 : config_remote();break;
  case 82 : remote_keys();break;

  case 91 : config_serial0();break;
  case 92 : station_serial_list();break;
  case 93 : send_stats();break;
  case 94 : station_www_list();break;

  case 101 : config_serialkbd();break;
  case 102 : serial_monitor();break;
  }
}

#define MENULEN 10
  const char mposc[MENULEN]={5,5,1,2,2,5,3,2,4,2};
  char mpos[]="Announce!Position!Symbol!Callsign!Contacts!Path!Status!Delay!NAV tag!Dest!Config!Ranges!Options!Receive!Options!Sounds!TX lock!TRXTune\
!Frames!Factory!Aliases!Timing!Option!Config!Keys!Config!Stations!Stats!www!Keyboard!Monitor!";



void menu()
{

  char i,c,mc,x,y,f,p2max;
  char exit;
  char msp;   //menu start pos


  exit=0;
  LCDselect(2);
  while(!exit)
  {

  if(menup1>4){msp=menup1-3;}else{msp=1;}
  if(menup1==MENULEN){msp=MENULEN-4;}

  LCDcolor_foreground=(nightv ? GREEN : BLACK);
  LCDcolor_background=(nightv ? BLACK : WHITE);
  LCDclear();
  flagfixed=0;
  LCDcolor_foreground=(nightv ? WHITE : BLACK);
      LCDcolor_background=(nightv ? BLACK : WHITE);

    LCDline(0,119,5,119);
    LCDline(0,122,5,122);
    LCDline(0,125,5,125);
    LCDcolor_foreground=(nightv ? YELLOW : BLACK);
    LCDyxtxt(6,116,7,"MAIN MENU");
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDline(cursorx,119,175,119);
    LCDline(cursorx,122,175,122);
    LCDline(cursorx,125,175,125);

  for(i=0;i<5;i++){
    LCDcolor_foreground=(nightv ? WHITE : BLACK);
    LCDrectangle(2,89-18*i,73,107-18*i);

    if((msp+i)==menup1){
      LCDcolor_foreground=(nightv ? YELLOW : BLACK);
      LCDfillrectangle(3,90-18*i,72,106-18*i);
      LCDcolor_foreground=(nightv ? BLACK : BLUE);
    }
    LCDyx(90-18*i,5);
    switch(i+msp){
    case 1:LCDtxtt(6,"Station");break;
    case 2:LCDtxtt(6,"Beacon");break;
    case 3:LCDtxtt(6,"Filters");break;
    case 4:LCDtxtt(6,"Radar");break;
    case 5:LCDtxtt(6,"Messages");break;
    case 6:LCDtxtt(6,"Options");break;
    case 7:LCDtxtt(6,"Digi");break;
    case 8:LCDtxtt(6,"Remote");break;
    case 9:LCDtxtt(6,"Output");break;
    case 10:LCDtxtt(6,"Periph");break;
    }

  }


  LCDcolor_foreground=(nightv ? WHITE : BLACK);
  c=menup1-1;

  mc=mposc[c];
  //policz ile poprzedza
  i=0;
  while(c>0){c--;i=i+mposc[c];}
  //znajdz pierwszy
  c=0;
  while(i>0)
  {while(mpos[c]!='!'){c++;}
   c++;
   i=i-1;
  }
  //wyswietl
  y=85;
  p2max=0;
  while(mc>0)
  {
    LCDrectangle(102,y-1,170,y+17);
    LCDyx(y,105);
    while(mpos[c]!='!'){//font select
      if(mpos[c]=='$'){
       c++;
       f=F_SYM_MONO;
      } else {f=F_15X15;}
      pchar(f,1,mpos[c],0);c++;
    }
    c++;
    mc--;
    y=y-18;
    p2max++;
  }



  LCDcolor_foreground=(nightv ? YELLOW : BLACK);
  x=97-(menup1-msp)*18;
  y=94-(menup2-1)*18;
  LCDfillrectangle(75,x-1,85,x+1);
  LCDfillrectangle(85,y-1,98,y+1);
  LCDfillrectangle(84,x,86,y);
  LCDyx(y-5,95);
  pchar(F_SYM_MONO,1,0x0C,0);
  c=0;
  while((c<2)||(c>8)){c=read_kbd();}
  //sprintf(napis,"%d",c);
  //LCDyxtxt(0,1,1,napis);
  switch(c)
  {
  case 2:menup2--;break;
  case 4:menup1++;menup2=1;break;
  case 6:menup1--;menup2=1;break;
  case 8:menup2++;break;
  case 7:exit=1;break;
  case 5:
          gomenu(menup1,menup2);
  }
  //if(menup1<1){menup1=7;}
  if(menup2<1){menup2=p2max;}
  if(menup2>p2max){menup2=1;}
  //if(menup1>7){menup1=1;}
  if(menup1>MENULEN){menup1=1;}
  if(menup1<1){menup1=MENULEN;};


  }//while !exit
  printf("menu exit\n");
}





void station_announce_clear()
{
  LCDcolor_foreground=(nightv ? BLACK : WHITE);
  LCDselect(1);
 LCDfillrectangle(0,0,175,40);
 announce_status=0;
}





void message_announce(int id)
{
//char napis[8];
char i;

 LCDselect(1);
 scrolling_txt_status.active=0;
 announce_status=1;
 i=1;


 LCDcolor_foreground=(nightv ? rgb(0,0,0) : WHITE);
 LCDfillrectangle(1,1,174,40);
 LCDcolor_background=(nightv ? rgb(0,0,0) : WHITE);
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 LCDrectangle(0,0,175,40);
 LCDfixed(0);
 LCDyxtxt(0,30,5,"NEW MESSAGE FROM");
 LCDyx(4,4);
 for(i=0;i<6;i++)
 {if(stations[id].sign[i]>32){pchar(F_20X23,1,stations[id].sign[i],0);};}
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
   LCDcolor_foreground=LIGHT_BLUE;
   sprintf(napis,"%d",stations[id].ssid);
   i=6;
   if(stations[id].ssid>9){i=i+6;}
   if(cursorx+i<120){LCDyxtxt(7,28,cursorx,napis);}
   else {LCDyxtxt(7,28,120-i,napis);}
 }
 //for(i=0;i<6;i++){pchar(F_20X23,1,stations[id].sign[i],0);}//f20x23
 LCDfixed(1);
 LCDyx(31,110);
 LCDcolor_foreground=(nightv ? GREEN : BLACK);


 LCDyx(11,120);
 pchar(F_ZNAKI,1,6,0);//koperta

}




void station_announce(int id)
{
//char napis[8];
char i,k,sl;
unsigned long tt;

 LCDselect(1);
 scrolling_txt_status.active=0;
 announce_status=1;
 i=1;
 while((i<filtered_stations)&&(sort[i]!=id)){i++;}
 if((!announce_filter)||(sort[i]==id))
 {

   if(serial0_opt==2){
     //U0LCR = 0x83;                   /* 8 bits, no Parity, 1 Stop bit            */
  		
     //U0DLM = 1;
     //U0DLL = 128;                    // 9600
     //U0LCR = 0x03;

     putchar(0x0C); //clear screen
     waitms(35);
     putchar(0x07); //color white
     printf("Received ");
     for(i=0;i<6;i++){napis[i]=stations[id].sign[i];}
     napis[6]=0;
     printf(napis);
     if(stations[id].ssid>0){
     sprintf(napis,"-%d",stations[id].ssid);
     printf(napis);
     }
     putchar(0x0A); //LF
     tt=stations[id].dist/10;
     if(stations[id].dist<999999){
      sprintf(napis,"%ld.%1dkm ",tt,stations[id].dist-10*tt);
      printf(napis);
     }
     if(stations[id].type!=1){ //direct
      printf("DIRECT ");
     }    
   }//tv gen

   if(serial0_opt==3){ //tvgen monitor
#ifdef nouse   
     U0LCR = 0x83;                   /* 8 bits, no Parity, 1 Stop bit            */
  		
     U0DLM = 1;
     U0DLL = 128;                    // 9600
     U0LCR = 0x03;

     putchar(0x0C); //clear screen
     waitms(35);
     putchar(0x07); //color white
     printf("APRS Navigator packet monitor");
     putchar(0x0A); //LF
     sprintf(napis," Stations:%3d  Frames:%5d",num_stations,packet_count);
     printf(napis);
     putchar(0x0A); //LF
     putchar(0x0A); //LF
     printf("Received ");
     for(i=0;i<6;i++){napis[i]=stations[id].sign[i];}
     napis[6]=0;
     printf(napis);
     if(stations[id].ssid>0){
     sprintf(napis,"-%d",stations[id].ssid);
     printf(napis);
     }
     printf(" :");

     tt=stations[id].dist/10;
     if(stations[id].dist<999999){
      sprintf(napis,"%ld.%1dkm ",tt,stations[id].dist-10*tt);
      printf(napis);
     }
     if(stations[id].type!=1){ //direct
      printf("DIRECT ");
     }
     putchar(0x0A); //LF
     putchar(0x0A); //LF
     flash_read(napis,stations[id].slot,255);
     i=1;
     while(napis[i]!=0x03){
       if((napis[i]<32)||(napis[i]>126)){putchar('.');}
        else {putchar(napis[i]);}
       i++;
     }
     while(napis[i]>0){
       if((napis[i]<32)||(napis[i]>126)){putchar('.');}
        else {putchar(napis[i]);}
       i++;
     }
#endif
   }//tv gen

 LCDcolor_foreground=(nightv ? rgb(0,0,0) : WHITE);
 LCDfillrectangle(1,1,174,40);
 LCDcolor_background=(nightv ? rgb(0,0,0) : WHITE);
 LCDcolor_foreground=(nightv ? GREEN : BLACK);
 LCDrectangle(0,0,175,40);
 LCDfixed(0);
 LCDyx(15,4);
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 for(i=0;i<6;i++)
 {if(stations[id].sign[i]>32){pchar(F_20X23,1,stations[id].sign[i],0);};}
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
   LCDcolor_foreground=BURSZTYN;
   sprintf(napis,"%d",stations[id].ssid);
   i=6;
   if(stations[id].ssid>9){i=i+6;}
   if(cursorx+i<110){LCDyxtxt(7,28,cursorx,napis);}
   else {LCDyxtxt(7,28,110-i,napis);}
 }
 //for(i=0;i<6;i++)
 //  {if(stations[id].sign[i]>32){pchar(F_20X23,1,stations[id].sign[i],0);};}//f20x23
 LCDfixed(1);
 LCDyx(31,110);
 LCDcolor_foreground=(nightv ? GREEN : BLACK);
 if(stations[id].type==0){ //direct
    LCDtxt(0,"DIRECT");
  }
 if(stations[id].type==1){ //direct
    if(stations[id].pathvalid){
      LCDtxt(F_SMALL,"via-");
      for(i=0;i<6;i++){LCDchr(0,stations[id].path[i]);}
    }		
  }
 if(stations[id].type==2){ //direct
    LCDtxt(0,"(DIRECT)");
  }
 LCDyx(21,110);
 tt=stations[id].dist/10;
    if(stations[id].dist<999999){
      sprintf(napis,"%ld.%1dkm",tt,stations[id].dist-10*tt);
      LCDtxt(0,napis);
    }
    sprintf(napis,"FRAME %d",stations[id].count);
    if(stations[id].count==1){LCDcolor_foreground=(nightv ? WHITE : BLACK);}
    LCDyxtxt(0,11,110,napis);
    sl=istrlen(stations[id].info);
    if(sl>23){
    LCDcolor_foreground=(nightv ? YELLOW : BLUE);
 sprintf(scrolling_txt,"    %s           ",stations[id].info);
 //LCDyxtxt(5,2,4,stations[id].info);
 scrolling_txt_status.x=4;
 scrolling_txt_status.y=2;
 scrolling_txt_status.x1=174;
 scrolling_txt_status.active=1;
 scrolling_txt_status.pos=0;
 scrolling_txt_status.pose=7*(sl+9)-170;//7*(sl+9);
    } else
    { //static text
      LCDcolor_foreground=(nightv ? YELLOW : BLUE);
      LCDyxtxt(0,2,4,stations[id].info);
    }
 }
}




void list_info_header()
{
  //char napis[25];

  LCDselect(1);


  sprintf(napis,"ST%3d/%3d",filtered_stations,num_stations);
  LCDcolor_foreground=(nightv ? WHITE : BLUE);
  LCDcolor_background=BLACK;
  LCDyxtxt(0,124,10,napis);


  //LCDline(0,124,175,124);
}




void list_header(char startpos, char markpos)
{
//callsign sym info time path dist bearing frames-count comment
char x,xi,i;
//char napis[10];
 LCDselect(1);
 LCDcolor_foreground=(nightv ? BLACK : BLUE);
 if(!nightv){LCDfillrectangle(0,114,175,123);}
 //LCDcolor_foreground=(nightv ? BURSZTYN : WHITE);
 LCDcolor_background=(nightv ? BLACK : BLUE);
 //LCDyxtxt(0,123,0,"CALLSIGN ");
 x=2;
 //delete previous graphics
    LCDcolor_foreground=(nightv ? BLACK : BLUE);
    LCDline(0,114,175,114);
    LCDrectangle(0,115,10,123);
    LCDcolor_foreground=(nightv ? YELLOW : WHITE);
    LCDline(0,123,175,123);

 for(i=0;i<9;i++){
  if(((i==0)||(i>=startpos))&&(x<164))
  {
   xi=x;
   switch(i)
   {
   case 0 : sprintf(napis,"STATION  ");x=x+56; break;
   case 1 : sprintf(napis,"S "); x=x+14; break;
   case 2 : sprintf(napis,"INF "); x=x+28;break;
   case 3 : sprintf(napis,"TIME  "); x=x+42;break;
   case 4 : sprintf(napis,"PATH   "); x=x+49;break;
   case 5 : sprintf(napis,"DIST "); x=x+35;break;
   case 6 : sprintf(napis,"BRG "); x=x+28;break;
   case 7 : sprintf(napis,"CNT "); x=x+28;break;
   case 8 : sprintf(napis,"COMMENT    "); x=x+56; break;
   }
   if(x>175){x=175;}

   LCDyxtxt(0,115,xi,napis);
   if(i==(markpos&0x7F)){
    LCDyx(117,x-8);
    LCDfixed(0);
    if(station_list_sort&0x80)
    {pchar(F_SYM_MONO,1,2,0);}
    else
    {pchar(F_SYM_MONO,1,3,0);}
    LCDrectangle(xi-2,115,x-2,123);
   }
   if(i==0){x=x+7;}
  }
 } //for
 LCDcolor_foreground=(nightv ? WHITE : YELLOW);
 if(startpos==0){LCDyx(116,58);pchar(F_SYM_MONO,1,4,0);}
  LCDcolor_foreground=(nightv ? BLACK : BLUE);
  LCDfillrectangle(x,122,175,131);


 //LCDtxt(0,"TIME ");
 //LCDrectangle(90,122,120,131);
 //LCDtxt(0,"DIST");

}

void compass_page()
{
  int i;
  char sats[]={1,5,15,8,2,9,11,3,4,4,2,9};

  LCDcolor_foreground=(nightv ? GREEN : BLACK);
  LCDcolor_background=(nightv ? BLACK : GREY);
  LCDselect(2);
  LCDclear();
  //LCDfillrectangle(0,0,175,119);
  if(nightv){
   LCDrectangle(0,1,65,28);
   LCDrectangle(0,30,65,57);
   LCDrectangle(0,59,65,86);
   LCDrectangle(0,88,65,115);
  } else {
  LCDcolor_foreground=WHITE;
  LCDfillrectangle(1,1,65,31);
  LCDfillrectangle(1,33,65,63);
  LCDfillrectangle(1,65,65,95);
  LCDfillrectangle(1,97,65,127);

  LCDfillrectangle(68,1,174,21);
  }//nightv
  LCDcolor_foreground=(nightv ? GREEN : BLACK);
  LCDcolor_background=(nightv ? BLACK : WHITE);
  LCDyxtxt(1,20,2,"GPS TIME");
  LCDyxtxt(1,49,2,"POSITION");
  LCDyxtxt(1,78,2,"TRACK");
  LCDyxtxt(1,107,2,"SPEED");




  LCDyxtxt(2,5,10,"15:36");


  //flagfixed=1;
  //LCDyxtxt(4,90,7,"121");
  //sprintf(napis,"%003d",mycourse);
  //LCDyxtxt(4,61,7,napis);
  //flagfixed=0;
  //LCDyxtxt(2,100,10,"121");
  //pchar(F_SYM_MONO,0,1,0);//pchar(sym_mono,0,1);
  //LCDtxt(1," km/h");

  //LCDrectangle(68,14,174,119);

  //LCDcolor_foreground=rgb(16,31,31);
  //LCDfillrectangle(69,25,173,129);
  //LCDcolor_foreground=WHITE;
  //LCDellipse(121,77,50,50);
  //LCDcolor_foreground=BLACK;
  LCDcircle(121,67,47);
  //LCDcircle(121,67,35);
  LCDyxtxt(F_SMALL,1,68,"SAT");



}


void text_scroll(int y, int x, int x2, char *str,int pos)
{
int x1,p1,i;

x1 = 8 - (pos % 7);
p1 = pos / 7;
for(i=0;i<p1;i++){str++;}
LCDyx(y,x+x1);
//LCDcolor_foreground=(nightv ? BURSZTYN : BLACK);
//LCDcolor_background=(nightv ? BLACK : WHITE);
i=0;
x1=x+x1;
while((*str)&&(x1<(x2-7))){
 LCDchr(0,*str++);
 x1=x1+7;
 i++;
 if(i==1){LCDfillrectangle(x,y,x+6,y+8);}
}

}



struct {
  char x,y;

} rstmem[MAX_STATIONS];




void radar_station(int x, int y, char r, char g, char b)
{
char i;
 LCDcolor_foreground=rgb(r-r/3,g-g/3,b);
 LCDellipse(x,y,2,2);
 LCDcolor_foreground=rgb(r,g,b);
 LCDellipse(x,y,1,1);
 /*
 for(i=0;i<5;i++)setPixel(x-2+i,y);
 for(i=0;i<5;i++)setPixel(x,y-2+i);
 setPixel(x-1,y-1);
 setPixel(x-1,y+1);
 setPixel(x+1,y-1);
 setPixel(x+1,y+1);
 */
}



void radar(char cstation,int range,int ang, char rdrw)
{
static int memi;
static char memx, memy; //line coord memo
static char cntr;
int i,n;
int x1,y1;
char cntd;
float ff,si,co;
unsigned long tt;
//char napis[20];

 LCDselect(2);
 LCDfixed(0);
 LCDcolor_background=BLACK;
 LCDcolor_foreground=BLACK;
 if(rdrw>0){
 LCDclear();
 //LCDcolor_foreground=rgb(0,15,0);
 //LCDfillrectangle(1,1,59,130);
 LCDcolor_foreground=rgb(0,63,0);
 LCDrectangle(0,0,175,131);
 LCDline(50,0,50,115);
 LCDline(0,115,50,115);

 LCDcolor_background=rgb(0,0,0);
 LCDyxtxt(F_SMALL,105,3,"Range");
 sprintf(napis,"%d",radar_range);
 LCDyxtxt(4,85,2,napis);
 LCDline(0,83,50,83);
 LCDyxtxt(F_SMALL,73,3,"Stations");
 LCDyxtxt(F_SMALL,65,3,"in range");
 //LCDyxtxt(F_22,43,40,"12");
 LCDline(0,41,50,41);
 LCDyxtxt(F_SMALL,31,3,"DIRECT");
 LCDyxtxt(F_SMALL,23,3,"in range");
 //LCDyxtxt(F_22,3,40,"2");

 LCDcolor_foreground=rgb(0,0,0);
 LCDellipse(112,68,60,60);
 LCDcolor_foreground=rgb(0,30,0);
 LCDcircle(112,68,61);
 LCDcolor_foreground=GREEN;
 LCDrectangle(60,0,164,9);

 i=0;
 memi=0;
 memx=80;
 memy=120;
 cntr=0;

 }



//wipe stations //a co jak stacja w miedzyczasie zniknela z listy?
 LCDcolor_foreground=rgb(0,0,0);
 for(n=1;n<=cntr;n++){

    x1=rstmem[n].x;
    y1=rstmem[n].y;
    LCDellipse(x1,y1,2,2);

 }
 //wipe red line
 LCDline(memx,memy,80,120);
 //wipe previous
 i=memi;
 LCDcolor_foreground=rgb(0,0,0);
 for(i=0;i<720;i=i+45)
 {
  ff=1.0/180.0*3.1415926;
  si=((float)(i+memi)/2.0)*ff;  	
	co=cos(si);	
	si=sin(si);
  x1=-59.0*si+112;
  y1=+59.0*co+68;

  LCDline(112,68,x1,y1);
 }
 //wipe red line
 //LCDline(memx,memy,102,105);




 //new
 cntr=0;
 cntd=0;
 memi=2*ang;
 for(i=0;i<720;i=i+45)
 {
  ff=1.0/180.0*3.1415926;
  si=((float)(i+memi)/2.0)*ff;  	
	co=cos(si);	
	si=sin(si);
  x1=-59.0*si+112;
  y1=+59.0*co+68;
  if(i==0){LCDcolor_foreground=rgb(25,63,0);}
   else{LCDcolor_foreground=rgb(0,63,0);}

  LCDline(112,68,x1,y1);
 }
 LCDcolor_foreground=rgb(0,30,0);
 LCDcircle(112,68,59);
 LCDcolor_foreground=rgb(0,63,0);
 LCDcircle(112,68,60);
 LCDcircle(112,68,40);
 LCDcircle(112,68,20);
 LCDline(80,9,144,9);
 LCDcolor_foreground=YELLOW;
 x1=range/3;
 sprintf(napis,"%d",x1);
 LCDyxtxt(F_SMALL,69,130,napis);
 x1=x1+x1;
 sprintf(napis,"%d",x1);
 LCDyxtxt(F_SMALL,69,150,napis);

 for(n=1;n<=num_stations;n++){

  if((stations[n].dist<999999)&&(stations[n].dist<(10*range))){
    cntr++;
    if((stations[n].type&(~0x02))==0){cntd++;}
    tt=stations[n].dist/10;
    tt=(tt*60)/range;
    if(tt>57){tt=57;}  //problem brzegu
    ff=-1.0/180.0*3.1415926;
    si=((float)(stations[n].namiar-ang))*ff;  	
    co=cos(si);	
    si=sin(si);
    x1=-(float)tt*si+112;
    y1=+(float)tt*co+68;

    radar_station(x1,y1,31,63,5);
    //test-option?
    //LCDcolor_foreground=rgb(0,63,0);
    //LCDline(x1,y1,112,68);
    //memorize displayed station
    rstmem[cntr].x=x1;
    rstmem[cntr].y=y1;
  }
 }
 //show marked
 if(marked_station>0)
 {
   n=marked_station;
 if((stations[n].dist<999999)&&(stations[n].dist<(10*range))){
    tt=stations[n].dist/10;
    tt=(tt*60)/range;
    if(tt>57){tt=57;}  //problem brzegu
    ff=-1.0/180.0*3.1415926;
    si=((float)(stations[n].namiar-ang))*ff;  	
    co=cos(si);	
    si=sin(si);
    x1=-(float)tt*si+112;
    y1=+(float)tt*co+68;

      radar_station(x1,y1,10,63,31);

  }
 }
 //show current
 n=cstation;
 if(stations[n].dist<999999){
    tt=stations[n].dist/10;
    if(tt<range)
    {
     tt=(tt*60)/range;
     if(tt>57){tt=57;}
     ff=-1.0/180.0*3.1415926;
     si=((float)(stations[n].namiar-ang))*ff;  	
     co=cos(si);	
     si=sin(si);
     x1=-(float)tt*si+112;
     y1=+(float)tt*co+68;
     radar_station(x1,y1,31,0,0);
     LCDcolor_foreground=RED;
     memx=x1;
     memy=y1+3;
     LCDline(memx,memy,80,120);
     tt=stations[n].dist/10;
     sprintf(napis,"DIST:%4ld/%003d",tt,stations[n].namiar);
     LCDyxtxt(0,1,65,napis);
//HACKERS CONTROL
     hackcheck&=test_crypt(napis);
//HACKERS CONTROL
    }
    else
    {
      LCDcolor_foreground=GREEN;
      LCDyxtxt(0,1,65," NOT IN RANGE ");
    }
  }
 else{
   LCDcolor_foreground=GREEN;
   LCDyxtxt(0,1,65," NO POSITION  ");
 }
  //liczby
  LCDcolor_background=rgb(0,0,0);
  LCDcolor_foreground=GREEN;
  sprintf(napis,"%3d",cntr);
  LCDyxtxt(4,44,2,napis);
  sprintf(napis,"%3d",cntd);
  LCDyxtxt(4,3,2,napis);
  //stacja wyrozniona
  LCDcolor_foreground=RED;
  for(i=0;i<6;i++){napis[i]=stations[n].sign[i];}
  napis[6]=0;
  LCDyxtxt(0,121,15,napis);
  if(stations[n].ssid>0){
   sprintf(napis,"-%d",stations[n].ssid);
  } else {
   sprintf(napis,"   ");
  }
  LCDtxt(0,napis);
  LCDline(15,120,80,120);



}



