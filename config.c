#include "includes.h"


void capture_position(char id)
{
char i;

aprs_data_flag=1;
for(i=0;i<7;i++){stations[0].position[i]=stations[id].position[i];}

  recalc_positions();
  //for(i=1;i<=num_stations;i++){stations[i].distchg=0;}
}


void config_messages_receive()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("MESSAGES OPTION");
   flagfixed=0;

   LCDyxtxt(6,80,10,"Read all");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(allmessages){pchar(F_ZNAKI,1,3,0);}
   /*
   LCDyxtxt(6,60,10,"TCP/IP");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(tcpipframes){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,40,10,"INVALID");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(!validonly){pchar(F_ZNAKI,1,3,0);}
*/
   LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,90,99-20*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     //case 2 : if(m>0){m=m-1;};break;
     //case 8 : if(m<2){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 : if(m==0){allmessages=1-allmessages;};
              //if(m==1){tcpipframes=1-tcpipframes;};
              //if(m==2){validonly=1-validonly;}
              break;

   }
 }
 update_config();
}


void config_messages_option()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("MSGS OPTIONS");
   flagfixed=1;
   sprintf(napis,"Retries :%3d",msg_retries); LCDyxtxt(F_8X10,91,10,napis);
   sprintf(napis,"Delay   :%3d",msg_delay); LCDyxtxt(F_8X10,76,10,napis);

   LCDrectangle(4,90-15*m,100,105-15*m);
   flagfixed=0;
   LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");

   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x10,2);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<1){m=m+1;};break;
     case 7 : exit=1;break;
     case 4 : chg=-1;break;
     case 6 : chg=1;break;
   }
   if(chg!=0){
    switch(m)
    {
    case 0 : msg_retries+=chg;break;
    case 1 : msg_delay+=(chg*5);break;
    }
    chg=0;
    if(msg_retries>10){msg_retries=10;}
    if(msg_retries<1){msg_retries=1;}
    if(msg_delay>60){msg_delay=60;}
    if(msg_delay<10){msg_delay=10;}
   }
 }

 update_config();
}






//encryption engine for static keys generation
static unsigned long kev[2];


void kcrypter(long N)
{
unsigned long ek[5];
unsigned long y=kev[0];
unsigned long z=kev[1];
unsigned long DELTA=0x9e3779b9;

//klucz
ek[2]=0x00000000;
ek[1]=0x43765123;
ek[3]=0x00000000;
ek[0]=utimeval;

unsigned long limit=DELTA*N;
unsigned long sum=0;
while(sum!=limit){
 y+=((z<<4 ^ z>>5) + z) ^ (sum + ek[sum&3]);
 sum+=DELTA;
 z+=((y<<4 ^ y>>5) + y) ^ (sum + ek[(sum>>11)&3]);
}


kev[0]=y;
kev[1]=z;

}


void remote_keys()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("KEYGEN");
   kev[0]=utimeval;kcrypter(5);rkey[0].key=kev[0];
   kev[0]=utimeval;kcrypter(9);rkey[1].key=kev[0];
   kev[0]=utimeval;kcrypter(11);rkey[2].key=kev[0];
   kev[0]=utimeval;kcrypter(15);rkey[3].key=kev[0];
   flagfixed=1;
   sprintf(napis,"%08X",rkey[0].key); LCDyxtxt(F_8X10,90,10,napis);
   sprintf(napis,"%08X",rkey[1].key); LCDyxtxt(F_8X10,75,10,napis);
   sprintf(napis,"%08X",rkey[2].key); LCDyxtxt(F_8X10,50,10,napis);
   sprintf(napis,"%08X",rkey[3].key); LCDyxtxt(F_8X10,35,10,napis);
   for(c=0;c<4;c++){rkey[c].enable=1;}
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   if(c==7){exit=1;}
 }

}



void config_remote()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("REMOTE CONTROL");
   flagfixed=0;
   LCDyxtxt(6,80,10,"Enable");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(1){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,60,10,"Key ");
   kev[0]=utimeval;kcrypter(5);
   sprintf(napis,"%08X",kev[0]);
   LCDtxt(6,napis);

   //LCDrectangle(4,90-15*m,100,105-15*m);
   flagfixed=0;


   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 5 : if(m>0){m=m-1;};break;
     case 7 : exit=1;break;
   }

 }

 update_config();
}



void config_serialkbd()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("KEYBOARD");
   flagfixed=1;
   LCDyxtxt(6,80,10,"Enable");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(ser_kbden){pchar(F_ZNAKI,1,3,0);}

   flagfixed=0;


   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 5 : ser_kbden=1-ser_kbden;break;
     case 7 : exit=1;break;
   }

 }
 if(ser_kbden){
   gpsenable=0;

 }
}


void config_aliases()
{
char i,c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("ALIASES");
   flagfixed=1;
   for(i=0;i<4;i++){
     LCDyx(91-15*i,5);
     for(c=0;c<6;c++){
      LCDchr(F_8X10,aliases[i].alias[c]);
     }
     cursorx+=8;
     switch(aliases[i].aliastype){
     case 0 : LCDtxt(F_8X10,"simple");break;
     case 1 : LCDtxt(F_8X10,"flood ");break;
     case 2 : LCDtxt(F_8X10,"trace ");break;
     }
     cursorx+=2;
     LCDchr(F_8X10,aliases[i].hoplimit+'0');
     cursorx+=10;
     if(aliases[i].enable){pchar(F_ZNAKI,1,3,0);}
   }
   LCDrectangle(2,90-15*m,110,105-15*m);
   flagfixed=0;
   LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");

   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x10,2);LCDtxt(0,"EDIT ");
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<3){m=m+1;};break;
     case 5 : aliases[m].enable=1-aliases[m].enable;break;
     case 6 : aliases[m].aliastype++;
              if(aliases[m].aliastype>2){aliases[m].aliastype=0;}
              break;
     case 7 : exit=1;break;
     case 4 :
              menu_header("ALIAS DEFINE");
              LCDyxtxt(0,80,10,"Input alias");
              sprintf(napis,"");
              chg=read_kbdstr(66,10,6,0,napis);
              if(chg<255)
              {
                for(i=0;i<6;i++){aliases[m].alias[i]=napis[i];}
                LCDyxtxt(0,50,10,"Hoplimit:");
                sprintf(napis,"");
                chg=read_kbdstr(50,80,1,0,napis);
                if(chg<255){aliases[m].hoplimit=napis[0]-'0';}
              };


   }
 }

 update_config();
}




void config_trx()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("TRX TUNE");
   flagfixed=1;
   sprintf(napis,"TXDelay     :%3d",trx_txdelay); LCDyxtxt(F_8X10,97,10,napis);
   sprintf(napis,"TXHeader    :%3d",trx_txheader); LCDyxtxt(F_8X10,82,10,napis);
   sprintf(napis,"bitdelay    :%3d",trx_bitdelay); LCDyxtxt(F_8X10,67,10,napis);
   sprintf(napis,"RXtune      :%3d",trx_rxtune); LCDyxtxt(F_8X10,52,10,napis);
   sprintf(napis,"Use squelch :%3d",trx_squelch); LCDyxtxt(F_8X10,37,10,napis);   LCDrectangle(4,95-15*m,100,110-15*m);
   flagfixed=0;
   LCDyx(20,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");

   LCDyx(2,1);
   pchar(F_SYM_MONO,1,0x10,2);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<4){m=m+1;};break;
     case 7 : exit=1;break;
     case 4 : chg=-1;break;
     case 6 : chg=1;break;
   }
   if(chg!=0){
    switch(m)
    {
    case 0 : trx_txdelay+=chg;break;
    case 1 : trx_txheader+=chg;break;
    case 2 : trx_bitdelay+=chg;break;
    case 3 : trx_rxtune+=chg;break;
    case 4 : if(trx_squelch){trx_squelch=0;}else{trx_squelch=1;};break;
    }
    chg=0;
   }
 }

 update_config();
 update_aprs_config();
}




void config_frames()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("FRAMES OPTION");
   flagfixed=0;

   LCDyxtxt(6,80,10,"Unique only");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(!allframes){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,60,10,"TCP/IP");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(tcpipframes){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,40,10,"INVALID");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(!validonly){pchar(F_ZNAKI,1,3,0);}

   LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,90,99-20*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<2){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 : if(m==0){allframes=1-allframes;};
              if(m==1){tcpipframes=1-tcpipframes;};
              if(m==2){validonly=1-validonly;}
              break;

   }
 }
 update_config();
}




void config_txlock()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("TX OPTION");
   flagfixed=0;

   LCDyxtxt(6,80,10,"Disable TX");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(!txenable){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,60,10,"Disable query");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(!queryenable){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,40,10,"Disable digi");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(!repeater_enable){pchar(F_ZNAKI,1,3,0);}

   LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,90,99-20*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<2){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 : if(m==0){txenable=1-txenable;};
              if(m==1){queryenable=1-queryenable;};
              if(m==2){repeater_enable=1-repeater_enable;}
              break;

   }
 }
 update_config();
}




char config_startup(char txmem)
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("STARTUP CHECK");
   flagfixed=0;

   LCDyxtxt(6,80,10,"TX enabled");
   cursorx=120;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(txmem){pchar(F_ZNAKI,1,3,0);}

   LCDyxtxt(6,60,10,"Digi enabled");
   cursorx=120;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(repeater_enable){pchar(F_ZNAKI,1,3,0);}

   LCDyxtxt(6,40,10,"Beacon enabled");
   cursorx=120;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if((beacon_delay<99000000)||beacon_auto){pchar(F_ZNAKI,1,3,0);}

   LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,90,99-20*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<2){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 : if(m==0){txmem=1-txmem;};
              if(m==2){if((beacon_delay<99000000)||beacon_auto)
               {beacon_auto=0;beacon_delay=99000000;
                update_config();}
                      };
              if(m==1){repeater_enable=1-repeater_enable;}
              break;

   }
 }
 return(txmem);

}



void config_navmark()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("NAVIGATOR TAG");
   flagfixed=0;

   LCDyxtxt(6,80,10,"Enable ");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(navmark){pchar(F_ZNAKI,1,3,0);}

   LCDyxtxt(7,60,10,"In status: {NAV}");
   LCDyx(30,10);
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");




   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {

     case 7 : exit=1;break;
#ifndef DEMO1
     case 5 : navmark=1-navmark;break;
#endif
   }
 }
 update_config();
}




void config_announce()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("ANNOUNCE");
   flagfixed=0;

   LCDyxtxt(6,80,10,"Unique ");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(announce_filter&0x02){pchar(F_ZNAKI,1,3,0);}
   LCDyxtxt(6,60,10,"By filter");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(announce_filter&0x01){pchar(F_ZNAKI,1,3,0);}

   LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,90,99-20*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<1){m=m+1;};break;
     case 7 : exit=1;break;
   case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 update_config();
}




void parse_path(char pn)
{
char ss[27];
char p,i,j,k,n;

  printf("0");
  i=0;
  while(napis[i]>0){i++;}
  napis[i++]=0;
  napis[i++]=0;
  napis[i++]=0;
  napis[i++]=0;

  printf("1");
  p=0;n=0;
  while((n<5)&&(napis[p]>0))
  {
    k=0;
    while((k<6)&&(napis[p]>0)&&(napis[p]!=','))
    {
     paths[pn].spath[7*n+k]=napis[p];
     p++;
     k++;
    }

    while(k<6){paths[pn].spath[7*n+k]=' ';k++;}

    if((napis[p-1]<='9')&&(napis[p-1]>='0')){
     paths[pn].spath[7*n+6]=napis[p-1];
    }
    else
    {
      paths[pn].spath[7*n+6]='0';
    }
    n=n+1;
    paths[pn].plen=n;
    k=k+1;
    p=p+1;
  }

}

void config_path()
{
char i,j,c,m,exit;
int chg;

 m=0;
 exit=0;
 LCDcolor_foreground=WHITE;
 while(!exit)
 {
   menu_header("UNPROTO PATH");
   flagfixed=0;
   for(i=0;i<3;i++){
     LCDyx(91-15*i,5);
     for(j=1;j<=paths[i].plen;j++){
      c=0;
      while((c<6)&&(paths[i].spath[7*j-7+c]>' '))
        {LCDchr(7,paths[i].spath[7*j-7+c]);c++;}
      if(paths[i].spath[7*j-1]>'0'){
        sprintf(napis,"-%d",paths[i].spath[7*j-1]-'0');
        LCDtxt(7,napis);
      };
      if(j<paths[i].plen){LCDchr(7,',');}

     }
     cursorx=125;
      if(paths[i].msgenable){pchar(F_ZNAKI,1,6,0);}
     cursorx=135;
      if(paths[i].enable){pchar(F_ZNAKI,1,3,0);}
   }
   LCDrectangle(3,90-15*m,145,105-15*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<2){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 : paths[m].enable=1-paths[m].enable;break;
     case 4 : menu_header("PATH DEFINE");
              LCDyxtxt(0,80,10,"Input path");
              sprintf(napis,"");
              chg=read_kbdstr(60,10,27,0,napis);
              if(chg<255)parse_path(m);
              break;
     case 6 : for(i=0;i<3;i++){paths[i].msgenable=0;};
              paths[m].msgenable=1;
              break;

   }
 }
 update_config();
}



void config_status()
{
char i,c,m,exit;
int chg;

 m=0;
 exit=0;

 while(!exit)
 {
   menu_header("STATUS TEXT");
   LCDcolor_foreground=WHITE;
   flagfixed=0;
   for(i=0;i<3;i++){
     LCDyx(91-30*i,5);

      c=0;
      while((c<44)&&(statusy[i].text[c]>0))
      {
        LCDchr(7,statusy[i].text[c]);c++;
        if(cursorx>130){LCDyx(81-30*i,10);};
      }



     cursorx=125;
      if(statusy[i].enable){pchar(F_ZNAKI,1,3,0);}
   }
   LCDrectangle(3,75-30*m,135,105-30*m);
   //chg=read_kbdstr(60,10,44,napis);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
#ifdef DEMO1
     case 8 : if(m<1){m=m+1;};break;
#else
     case 8 : if(m<2){m=m+1;};break;
#endif
     case 7 : exit=1;break;
     case 5 : statusy[m].enable=1-statusy[m].enable;break;
     case 4 : menu_header("STATUS DEFINE");
              LCDyxtxt(0,80,10,"Input status");
              napis[0]=0; //empty
              chg=read_kbdstr(60,5,36,0,napis);
              if(chg<255){
               for(i=0;i<=chg;i++){statusy[m].text[i]=napis[i];}

              };
              break;
     case 6 : menu_header("STATUS DEFINE");
              LCDyxtxt(0,80,10,"Edit status");
              i=0;while(statusy[m].text[i]){napis[i]=statusy[m].text[i];i++;}
              napis[i]=0; //end string
              chg=read_kbdstr(60,5,36,0,napis);
              if(chg<255){
               for(i=0;i<=chg;i++){statusy[m].text[i]=napis[i];}

              };
              break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 update_config();
}



void config_dest()
{
char i,c,m,exit;
int chg;

 m=0;
 exit=0;
 while(!exit)
 {
   menu_header("UNPROTO DEST");
   flagfixed=0;

   LCDyxtxt(0,100,10,"Current");
   LCDyx(80,10);
   for(c=0;c<6;c++){LCDchr(7,udest[c]);}

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<1){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 :
              LCDyxtxt(0,60,10,"Edit dest:");
              for(i=0;i<6;i++){napis[i]=udest[i];}
              napis[6]=0; //end string
              chg=read_kbdstr(48,5,6,0,napis);
              if(chg<255){
                for(i=0;i<chg;i++){udest[i]=napis[i];}
                chg++;
                while(chg<6){udest[chg++]=' ';}
              };
              break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 update_config();
}


void config_digi_delay()
{
char c,m,exit;
int chg,mn;

   exit=0;
   m=0;
   while(!exit)
   {
   menu_header("DIGI TIMING");
   flagfixed=1;

   sprintf(napis,"Dupetime :%3d",digidupetime); LCDyxtxt(F_8X10,91,10,napis);
   sprintf(napis,"Dupedelay:%3d",digidelay); LCDyxtxt(F_8X10,76,10,napis);

   LCDrectangle(4,90-15*m,100,105-15*m);

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c){
     case 8:;
     case 2:m=1-m;break;
     case 7:exit=1;break;
     case 5:    sprintf(napis,"");
              chg=read_kbdstr(60,10,3,0,napis);
              if(chg<255)
              {
              switch(chg){
                case 3 :mn=100;break;
                case 2 :mn=10;break;
                case 1 :mn=1;
               }
              if(m==0){
                digidupetime=0;
                for(c=0;c<chg;c++){digidupetime+=(napis[c]-'0')*mn;mn=mn/10;}
              }
              if(m==1){
                digidelay=0;
                for(c=0;c<chg;c++){digidelay+=(napis[c]-'0')*mn;mn=mn/10;}
              }
              };break;


   }

  }
  update_config();
}

void config_beacon_delay()
{
char c,m,exit;
int chg,mn;

 m=0;
 exit=0;
 while(!exit)
 {
   menu_header("BEACON TIMING");
   flagfixed=0;

   LCDyxtxt(6,80,10,"DELAY");
   cursorx=110;
   LCDcolor_foreground=WHITE;
   if(beacon_delay>990000){sprintf(napis,"STOP");beacon_delay=99000000;}
   else
   {sprintf(napis,"%4ds",beacon_delay/10);}
   LCDtxt(0,napis);

   LCDyxtxt(6,60,10,"AUTO DELAY");
   cursorx=110;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(beacon_auto){pchar(F_ZNAKI,1,3,0);}

   LCDyxtxt(6,40,10,"DIGI SENS");
   cursorx=110;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(beacon_digi){pchar(F_ZNAKI,1,3,0);}

   LCDyx(15,1);
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,100,99-20*m);
/*
   LCDyxtxt(6,80,10,"Delay ");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   if(beacon_delay>990000){sprintf(napis,"STOP");beacon_delay=99000000;}
   else
   {sprintf(napis,"%4ds",beacon_delay/10);}
   LCDtxt(0,napis);

   LCDyxtxt(6,60,10,"Auto delay");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(announce_filter&0x01){pchar(F_ZNAKI,1,3,0);}

   flagfixed=0;
   LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");

   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   LCDrectangle(4,80-20*m,90,99-20*m);
*/
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<2){m=m+1;};break;
     case 4 : if(m==0){beacon_delay=99000000;
                       next_beacon_time=timeval+beacon_delay;
              };break;
     case 6 : if(m==0){
              menu_header("BEACON TIMING");
              LCDyxtxt(0,80,10,"Beacon delay");
              sprintf(napis,"");
              chg=read_kbdstr(60,10,4,0,napis);
              if(chg<255)
              {
              switch(chg){
              case 4 :mn=1000;break;
              case 3 :mn=100;break;
              case 2 :mn=10;break;
              case 1 :mn=1;
              }
              beacon_delay=0;
              for(c=0;c<chg;c++){beacon_delay+=(napis[c]-'0')*mn;mn=mn/10;}
              beacon_delay=beacon_delay*10;
              if((system_userlevel>2)&&(beacon_delay<300)){
                beacon_delay=300;
              }
              next_beacon_time=timeval+beacon_delay;
              }
              };
              break;
   case 5:
              if(m==1){beacon_auto=1-beacon_auto;}
              if(m==2){digisens_time=999000000;beacon_digi=1-beacon_digi;}


              break;
     case 7 : exit=1;break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 update_config();
}




void config_symbol()
{
char i,c,m,exit,tbs;
int chg;

 m=0;
 exit=0;
 tbs=0;
 while(!exit)
 {
   if(m>3){tbs=m-3;}else{tbs=0;}
   menu_header("STATION SYMBOL");
   for(i=0;i<4;i++){
    LCDyx(81-22*i,15);
    for(c=0;c<10;c++){LCDchr(7,symnames[(tbs+i)*10+c]);}
    cursorx=70;
    LCDchr(7,symtable[(tbs+i)*3]);
    LCDchr(7,symtable[(tbs+i)*3+1]);
    cursorx=100;
    if(symtable[(tbs+i)*3+2]>0)pchar(F_ZNAKI,1,symtable[(tbs+i)*3+2],0);
    if(stations[0].symbol==tbs+i){pchar(F_ZNAKI,1,3,0);}
   }

   LCDrectangle(10,79-22*(m-tbs),90,100-22*(m-tbs));

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<SYMCNT-1){m=m+1;};break;
     case 5 : stations[0].symbol=m;break;
     case 7 : exit=1;break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }

 }
 update_config();
}

void config_ranges()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 while(!exit)
 {
   menu_header("RADAR_RANGES");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<1){m=m+1;};break;
     case 7 : exit=1;break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 update_config();
}


void config_serial0()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;

 while(!exit)
 {
   menu_header("SERIAL OUTPUT");
   flagfixed=1;

   switch(serial0_opt)
   {
   case 0: sprintf(napis,"NO OUTPUT");break;
   case 1: sprintf(napis,"PACKET MONITOR");break;
   case 2: sprintf(napis,"TV-GEN ANNOUNCE");break;
   case 3: sprintf(napis,"TV-GEN MONITOR");break;
   case 4: sprintf(napis,"DIGI MONITOR");break;
   case 5: sprintf(napis,"MARKED MONITOR");break;
   }
   LCDyxtxt(F_8X10,92,10,napis);
   LCDyx(5,1);
   //pchar(F_SYM_MONO,1,0x10,2);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
   case 2 : if(serial0_opt>0){serial0_opt--;}else{serial0_opt=5;};break;
     //case 8 : if(m<1){m=m+1;};break;
   case 6 :  serial0_opt++;if(serial0_opt>5){serial0_opt=0;};break;
     //case 4 :  if(chg>0){chg=chg-1;};break;
     case 7 : exit=1;break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 if(serial0_opt>0) //reconfigure serial port
 {
  //U0LCR = 0x83;
  //U0DLL = 64;                     // 57600
  //U0DLM = 0;
  //U0LCR = 0x03;

 }
 else //default serial port
 {
  //U0LCR = 0x83;
  //U0DLM = 3;
  //U0DLL = 0;                    // 4800
  //U0LCR = 0x03;
 }
}


void config_suffix()
{
char i,c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=stations[0].ssid;
 while(!exit)
 {
   menu_header("STATION OPTION");
   flagfixed=1;
   for(i=0;i<6;i++){napis[i]=stations[0].sign[i];}
   napis[6]=0;
   LCDyxtxt(F_8X10,92,10,napis);

   sprintf(napis,"Station SSID:%3d",chg);
   LCDyxtxt(F_8X10,72,10,napis);

   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x10,2);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     //case 2 : if(m>0){m=m-1;};break;
     //case 8 : if(m<1){m=m+1;};break;
     case 6 :  if(chg<15){chg=chg+1;};break;
     case 4 :  if(chg>0){chg=chg-1;};break;
     case 7 : exit=1;break;
     case 5 : if(system_userlevel<2){
              menu_header("CALL DEFINE");
              LCDyxtxt(0,80,10,"Input call");
              for(i=0;i<6;i++){napis[i]=stations[0].sign[i];}
              napis[6]=0;
              i=read_kbdstr(60,10,27,0,napis);
              if(i<255){
                for(i=0;i<6;i++){ stations[0].sign[i]=napis[i];}
              }
             }//if
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 stations[0].ssid=chg;
 update_config();
}

void config_contacts()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 while(!exit)
 {
   menu_header("CONTACT LIST");

   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<1){m=m+1;};break;
     case 7 : exit=1;break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
}


void config_factory()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;

   menu_header("FACTORY SETTING");

   LCDyxtxt(7,90,5,"Restore default settings?");
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   if(c==5){
     LCDyxtxt(7,75,5,"Are you sure?");
     c=0;
     while((c<2)||(c>8)){c=read_kbd();}
     if(c==5){
     //restore
     restore_factory();
     LCDyxtxt(7,60,5,"Restored");
     c=0;
     while((c!=7)){c=read_kbd();}
     }
   }

 update_config();
}


void config_sounds()
{
char c,m,exit;
int chg;

 m=1;
 exit=0;

 while(!exit)
 {
   setsound(soundtab[m]);
   menu_header("CONFIG SOUNDS");
   flagfixed=1;
   sprintf(napis,"Frame receive  :%3d",soundtab[1]); LCDyxtxt(F_8X10,104,10,napis);
   sprintf(napis,"Self receive   :%3d",soundtab[2]); LCDyxtxt(F_8X10,92,10,napis);
   sprintf(napis,"Contact receive:%3d",soundtab[3]); LCDyxtxt(F_8X10,80,10,napis);
   sprintf(napis,"Message receive:%3d",soundtab[4]); LCDyxtxt(F_8X10,68,10,napis);
   sprintf(napis,"Marked receive :%3d",soundtab[5]); LCDyxtxt(F_8X10,56,10,napis);
   sprintf(napis,"Frame sent     :%3d",soundtab[6]); LCDyxtxt(F_8X10,44,10,napis);
   LCDrectangle(4,114-12*m,130,126-12*m);
   flagfixed=0;
   LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");

   LCDyx(5,1);
   pchar(F_SYM_MONO,1,0x10,2);
   pchar(F_SYM_MONO,1,0x12,2);LCDtxt(0,"CHANGE");
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>1){m=m-1;};break;
     case 8 : if(m<6){m=m+1;};break;
     case 7 : exit=1;break;
     case 6 : soundtab[m]++;if(soundtab[m]>9){soundtab[m]=0;}break;
     case 4 : if(soundtab[m]>0){soundtab[m]--;}break;
   //case 5 : if(m==0){announce_filter^=0x02;}else{announce_filter^=0x01;};break;

   }
 }
 update_config();
}

void config_position()
{
char c,m,exit;
int chg;

 m=0;
 exit=0;
 chg=0;
 while(!exit)
 {
   menu_header("POSITION MODE");
   flagfixed=0;

   LCDyxtxt(6,80,10,"USE   ");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   switch(position_mode&0xF0){
     case 0x00 : LCDtxt(6,"AUTO");break;
     case 0x20 : LCDtxt(6,"STATIC");break;
   }

   LCDyxtxt(6,60,10,"REPORT");
   cursorx=100;
   LCDcolor_foreground=WHITE;
   switch(position_mode&0x0F){
     case 0x00 : LCDtxt(6,"AUTO");break;
     case 0x01 : LCDtxt(6,"GPS");break;
     case 0x02 : LCDtxt(6,"STATIC");break;
   }



   LCDyx(25,1);pchar(F_SYM_MONO,1,0x0E,2);
   pchar(F_SYM_MONO,1,0x13,2);
   LCDtxt(0,"SELECT ");
   pchar(F_SYM_MONO,1,0x11,2);LCDtxt(0,"CHANGE");



   LCDrectangle(4,80-20*m,90,99-20*m);
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   switch(c)
   {
     case 2 : if(m>0){m=m-1;};break;
     case 8 : if(m<1){m=m+1;};break;
     case 7 : exit=1;break;
     case 5 : if(m==0){
                position_mode=(position_mode&0x0F)|(0x20-(position_mode&0xF0)); }
              else {
                chg=position_mode&0x0F;
                chg++;
                if(chg>2){chg=0;}
                position_mode=(position_mode&0xF0)+chg;
              }
       break;

   }
 }
 update_config();

}





void config_filter()
{
  //char napis[11];
  char c,cc,f,s,i,j,curs,exit;
  char scheme;

 scheme=0;
 LCDselect(2);
 LCDclear();
 LCDcolor_foreground=WHITE;
 LCDcolor_background=BLACK;
 flagfixed=0;
 LCDyxtxt(6,116,5,"Display  filter  setting");
 curs=1;
 exit=0;
 while(!exit)
 {
 LCDcolor_foreground=WHITE;
 LCDrectangle(2,96,173,112);
 switch(scheme){
 case 0:sprintf(napis,"USER              ");break;
 case 1:sprintf(napis,"DIRECT ONLY       ");break;
 case 2:sprintf(napis,"HIDE DIRECT       ");break;
 case 3:sprintf(napis,"SHOW LOCAL DIGIS  ");break;
 case 4:sprintf(napis,"ALL               ");break;
 }
 LCDyxtxt(5,98,5,"Filter setting : ");
 LCDtxt(5,napis);
 i=1;j=0;
 for(s=1;s<9;s++)
 {
   j=j+1;
   if(j>3){j=1;i++;}

     cc=(i-1)*3+j;
     switch(cc){
     case 1: f=FILTER_ALL;break;
     case 2: f=FILTER_DIRECT;break;
     case 3: f=FILTER_POSITION;break;
     case 4: f=FILTER_NOPOSITION;break;
     case 5: f=FILTER_VIADIGI;break;
     case 6: f=FILTER_WX;break;
     case 7: f=FILTER_DIGI;break;
     case 8: f=FILTER_GATE;break;
     }



     if(cc==curs)
     {
       LCDcolor_foreground=YELLOW;
       LCDcolor_background=YELLOW;
     }
     else
     {
       LCDcolor_foreground=WHITE;
       LCDcolor_background=WHITE;
     }
     LCDfillrectangle(60*(j-1),90-28*(i-1),60*(j-1)+55,70-28*(i-1));
     LCDcolor_foreground=BLACK;
     LCDrectangle(60*(j-1)+2,90-28*(i-1)-2,60*(j-1)+53,72-28*(i-1));



     LCDcolor_foreground=BLACK;
     //if(cc==curs){LCDrectangle(60*(j-1)+2,100-30*(i-1)-2,60*(j-1)+53,82-30*(i-1));}
     LCDyx(77-28*(i-1),60*(j-1)+4);
     switch(cc)
     {
     case 1: LCDtxt(5,"ALL");break;
     case 2: LCDtxt(5,"DIRECT");break;
     case 3: LCDtxt(5,"POS");break;
     case 4: LCDtxt(5,"NO POS");break;
     case 5: LCDtxt(5,"VIA");break;
     case 6: LCDtxt(5,"WEATHER");break;
     case 7: LCDtxt(5,"DIGI");break;
     case 8: LCDtxt(5,"GATE");break;
     }
     LCDyx(71-28*(i-1),60*(j-1)+36);
     if((filter_opt_exclude&f)>0){
       pchar(F_ZNAKI,1,1,0);
     }
     if((filter_opt_include&f)>0){
       pchar(F_ZNAKI,2,2,0);
     }

 }
   switch(curs){
     case 1: f=FILTER_ALL;break;
     case 2: f=FILTER_DIRECT;break;
     case 3: f=FILTER_POSITION;break;
     case 4: f=FILTER_NOPOSITION;break;
     case 5: f=FILTER_VIADIGI;break;
     case 6: f=FILTER_WX;break;
     case 7: f=FILTER_DIGI;break;
     case 8: f=FILTER_GATE;break;
     }
   LCDcolor_background=BLACK;
   if(curs==9){LCDcolor_foreground=YELLOW;}else{LCDcolor_foreground=WHITE;}
   LCDyxtxt(0,3,40,"Announce filter ");
   LCDcolor_foreground=BLACK;
   LCDfillrectangle(cursorx,cursory,cursorx+20,cursory+20);
   LCDcolor_foreground=WHITE;
   LCDrectangle(cursorx+1,cursory+1,cursorx+11,cursory+11);
   if(announce_filter&0x01){pchar(F_ZNAKI,1,3,0);}else{pchar(F_ZNAKI,1,0,0);}
   c=0;
   while((c<2)||(c>8)){c=read_kbd();}
   //sprintf(napis,"%d",c);
   //LCDyxtxt(0,10,10,napis);
   switch(c){
   case 4: curs--;if(curs<1){curs=9;}break;
   case 6: curs++;if(curs>9){curs=1;}break;
   case 5: if((curs>0)&&(curs<9)){
           if(filter_opt_include&f){
            filter_opt_include&=~f;filter_opt_exclude|=f;
            } else
           if(filter_opt_exclude&f){
            filter_opt_include&=~f;filter_opt_exclude&=~f;
            } else
            {
              filter_opt_include|=f;
            };
            scheme=0;
           }
           if(curs==9){announce_filter^=0x01;}
           break;
   case 7: exit=1;break;
   case 2: scheme=scheme+2;
   case 8: scheme--;
           if(scheme>4){scheme=1;}
           if(scheme<1){scheme=4;}
           switch(scheme)
           {
           case 1:  filter_opt_include=FILTER_DIRECT;filter_opt_exclude=0x00;break;
           case 2:  filter_opt_include=FILTER_VIADIGI;filter_opt_exclude=0x00;break;
           case 3:  filter_opt_include=FILTER_DIGI;filter_opt_exclude=FILTER_VIADIGI;break;
           case 4:  filter_opt_include=FILTER_ALL;filter_opt_exclude=0x00;break;
           }
           break;

   }
 }//while

 /*
 LCDcolor_foreground=(nightv ? WHITE : BLACK);
 LCDcolor_background=(nightv ? WHITE : BLACK);

 LCDcolor_foreground=(nightv ? BLACK : BLACK);
 LCDyxtxt(0,90,2,"Direct");
 LCDcolor_foreground=(nightv ? RED : BLACK);
 LCDfillrectangle(60,100,115,80);
 LCDcolor_foreground=(nightv ? GREEN : BLACK);
 LCDfillrectangle(120,100,175,80);
 LCDrectangle(0,70,55,50);
 LCDrectangle(60,70,115,50);
 LCDrectangle(120,70,175,50);
 */

 /*
 if(filter_opt_include==FILTER_DIRECT)
 {
   filter_opt_include=0xFF;
 }else
 {
  filter_opt_include=FILTER_DIRECT;
 };
 sprintf(napis,"%s","edit me");
 read_kbdstr(50,10,10,napis);
 LCDyxtxt(0,60,10,napis);
 c=0;
 while(c!=5)
 {
  c=read_kbd();
  //sprintf(napis,"%3d",c);
  //LCDyxtxt(2,50,5,napis);
 }*/
 aprs_data_flag=1;
 station_sort(0);

}

