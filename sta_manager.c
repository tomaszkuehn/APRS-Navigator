#include "includes.h"


void print_symbol(char sym)
{

  switch(sym)
  {
   case '='	:printf("=");break;
   case '_'	:printf("W");break;
   case 'K'	:printf("K");break;
   case '#'	:printf("*");break;
   case '>'	:printf("C");break;
   case '-'	:printf("H");break;
   case '}'	:printf("G");break;
   default: printf(" ");
  }

}


void show_symbol(char sym)
{

  switch(sym)
  {
   case '='	:LCDtxt(0,"=");break;
   case '_'	:LCDtxt(0,"W");break;
   case 'K'	:LCDtxt(0,"K");break;
   case '#'	:LCDtxt(0,"*");break;
   case '>'	:LCDtxt(0,"C");break;
   case '-'	:LCDtxt(0,"H");break;
   case '}'	:LCDtxt(0,"G");break;
   default: LCDtxt(0," ");
  }

}

void print_frametype(char sym)
{

  switch(sym)
  {
   case '='	:printf("=");break;
   case '_'	:printf("W");break;
   case '#'	:printf("*");break;
   case '>'	:printf("C");break;
   case '-'	:printf("H");break;
   case '}'	:printf("G");break;
   case '`'	:printf("M");break;
   default: printf(" ");
  }

}

void show_frametype(char sym)
{

  switch(sym)
  {
   case '='	:LCDtxt(0,"Po");break;
   case '_'	:LCDtxt(0,"Wx");break;
   case '#'	:LCDtxt(0,"Wx");break;
   case '*'	:LCDtxt(0,"Wx");break;
   case '>'	:LCDtxt(0,"St");break;
   case '!'	:LCDtxt(0,"Po");break;
   case '}'	:LCDtxt(0,"IT");break;
   case '`'	:LCDtxt(0,"Mc");break;
   case 0x27	:LCDtxt(0,"Mc");break;
   case '@'	:LCDtxt(0,"Po");break;
   case '/'	:LCDtxt(0,"Po");break;
   case ':'	:LCDtxt(0,"Ms");break;
   case 'T'	:LCDtxt(0,"Tm");break;
   case ','	:LCDtxt(0,"??");break;
   case '?'	:LCDtxt(0,"Qr");break;
   case 'x'     :LCDtxt(0,"!!");break;

   default: LCDtxt(0,"  ");
  }

}

//sort and filter stations
//output list is in sort[] table
//output number is in filtered_stations
void station_sort(char opt)
{
static char sort1[MAX_STATIONS];
  char n,ts,swaps,sc;
  char include,exclude;
  int i;

  for(n=0;n<MAX_STATIONS;n++){sort1[n]=n;}
  swaps=1;
  while(swaps>0)
  {
   swaps=0;
   n=1;
   while(n<num_stations){

    switch(station_list_sort)
    {
    case 0x01:sc=(stations[sort1[n]].symbol<stations[sort1[n+1]].symbol);break;
    case 0x81:sc=(stations[sort1[n]].symbol>stations[sort1[n+1]].symbol);break;
    case 0x02:sc=(stations[sort1[n]].type<stations[sort1[n+1]].type);break;
    case 0x82:sc=(stations[sort1[n]].type>stations[sort1[n+1]].type);break;
    case 0x03:sc=(stations[sort1[n]].minutes<stations[sort1[n+1]].minutes);break;
    case 0x83:sc=(stations[sort1[n]].minutes>stations[sort1[n+1]].minutes);break;
    case 0x00:i=strncmp(stations[sort1[n]].sign,stations[sort1[n+1]].sign,6);
              if(i>0){sc=1;}else{sc=0;}
              break;
    case 0x80:i=strncmp(stations[sort1[n]].sign,stations[sort1[n+1]].sign,6);
              if(i<0){sc=1;}else{sc=0;}
              break;
    case 0x05:sc=(stations[sort1[n]].dist<stations[sort1[n+1]].dist);break;
    case 0x85:sc=(stations[sort1[n]].dist>stations[sort1[n+1]].dist);break;
    case 0x06:sc=(stations[sort1[n]].namiar<stations[sort1[n+1]].namiar);break;
    case 0x86:sc=(stations[sort1[n]].namiar>stations[sort1[n+1]].namiar);break;
    case 0x07:sc=(stations[sort1[n]].count<stations[sort1[n+1]].count);break;
    case 0x87:sc=(stations[sort1[n]].count>stations[sort1[n+1]].count);break;
    default:sc=(stations[sort1[n]].minutes<stations[sort1[n+1]].minutes);break;
    }

    if(sc)
    {//swap
      ts=sort1[n];
      sort1[n]=sort1[n+1];
      sort1[n+1]=ts;
      swaps=1;
    }
    n=n+1;
   }
  } //while swaps
  //now filter

  for(n=1;n<=num_stations;n++){
    exclude=0;
    if((filter_opt_exclude&FILTER_DIRECT)&&\
      ((stations[sort1[n]].type==0)||(stations[sort1[n]].type==2)))\
        {exclude=1;}
    if((!exclude)&&(filter_opt_exclude&FILTER_POSITION)&&\
      (stations[sort1[n]].dist<999999))\
        {exclude=1;}
    if((!exclude)&&(filter_opt_exclude&FILTER_NOPOSITION)&&\
      (stations[sort1[n]].dist==999999))\
        {exclude=1;}
    if((!exclude)&&(filter_opt_exclude&FILTER_VIADIGI)&&\
      (stations[sort1[n]].type==1))\
        {exclude=1;}
    if((!exclude)&&(filter_opt_exclude&FILTER_DIGI)&&\
      (stations[sort1[n]].symbol=='#'))\
        {exclude=1;}
    if((!exclude)&&(filter_opt_exclude&FILTER_WX)&&\
      (stations[sort1[n]].symbol=='_'))\
        {exclude=1;}
    if((!exclude)&&(filter_opt_exclude&FILTER_GATE)&&\
      (stations[sort1[n]].symbol=='}'))\
        {exclude=1;}
    if(exclude){sort1[n]=255;}
   }
  filtered_stations=0;
  for(n=1;n<=num_stations;n++){
    include=0;
    if(sort1[n]!=255) //not excluded
    {
      if(filter_opt_include&FILTER_ALL){include=1;}
    if((!include)&&(filter_opt_include&FILTER_DIRECT)&&\
      ((stations[sort1[n]].type==0)||(stations[sort1[n]].type==2)))\
        {include=1;}
    if((!include)&&(filter_opt_include&FILTER_POSITION)&&\
      (stations[sort1[n]].dist<999999))\
        {include=1;}
    if((!include)&&(filter_opt_include&FILTER_NOPOSITION)&&\
      (stations[sort1[n]].dist==999999))\
        {include=1;}
    if((!include)&&(filter_opt_include&FILTER_VIADIGI)&&\
      (stations[sort1[n]].type==1))\
        {include=1;}
    if((!include)&&(filter_opt_include&FILTER_DIGI)&&\
      (stations[sort1[n]].symbol=='#'))\
        {include=1;}
    if((!include)&&(filter_opt_include&FILTER_WX)&&\
      (stations[sort1[n]].symbol=='_'))\
        {include=1;}
      if((!include)&&(filter_opt_include&FILTER_GATE)&&\
        (stations[sort1[n]].symbol=='}'))\
        {include=1;}
    }
    if(include)
    {
      filtered_stations++;
      sort[filtered_stations]=sort1[n];
    }
   }


}



void station_www_list()
{
int i,j,n,max,adm,y;
char sort[MAX_STATIONS];
float si,co,ff;
int x1,y1;
int range;
//char napis[20];

unsigned long tt;

adm=aprs_data_flag;
aprs_data_flag=1;
//U0LCR = 0x83;
//U0DLL = 64;                     // 57600
//U0DLM = 0;
//U0LCR = 0x03;

y=0;
range=15;

printf("\n\n<?\n");
printf("$img=ImageCreate(800,682);\n");
printf("$black=ImageColorAllocate($img,0,0,0);\n");
printf("imagecolortransparent($img,$black);\n");

printf("$white=ImageColorAllocate($img,250,255,235);\n");
printf("$yellow=ImageColorAllocate($img,234,220,14);\n");
printf("$green=ImageColorAllocate($img,0,220,50);\n");
printf("$green1=ImageColorAllocate($img,0,160,30);\n");
printf("$red=ImageColorAllocate($img,250,0,0);\n");


//last heard
for(i=0;i<MAX_STATIONS;i++){sort[i]=1;}
n=1;
while((n<=18)&&(n<=num_stations)){
  max=0;
  tt=0;
  for(j=0;j<MAX_STATIONS;j++){
   if(sort[j]&&(stations[j].minutes>tt)){max=j;tt=stations[j].minutes;}
  }
  sort[max]=0;

  for(i=0;i<6;i++){napis[i]=stations[max].sign[i];}
  napis[6]=0;
  printf("ImageString($img,5,690,");
  printf("%d,'",n*20+240);
  printf(napis);


  if(stations[max].ssid>0){
	printf("-%d",stations[max].ssid);
  }
  printf("',$green1);\n");

  n=n+1;
}


//radar
range=15;
n=1;
while(n<=num_stations){

  if(stations[n].dist<150){

    tt=stations[n].dist;
    tt=(tt*30)/range;
    ff=-1.0/180.0*3.1415926;
    si=((float)(stations[n].namiar))*ff;  	
    co=cos(si);	
    si=sin(si);
    x1=-(float)tt*si+340;
    y1=340-(float)tt*co;

    max=n;

  printf("ImageArc($img,%d,%d,6,6,0,360,$red);\n",x1,y1);
  for(i=0;i<6;i++){napis[i]=stations[max].sign[i];}
  napis[6]=0;
  printf("ImageString($img,3,%d,%d,'",x1+5,y1-8);
  printf(napis);


  if(stations[max].ssid>0){
	printf("-%d",stations[max].ssid);
  }
  printf("',$yellow);\n");
   tt=stations[max].dist/10;
   if(stations[max].dist<999999)
    { printf("ImageString($img,1,%d,%d,'",x1+5,y1+5);
      printf("%d.%1dkm/%003d ",tt,stations[max].dist-10*tt,stations[max].namiar);
      printf("',$white);\n");
    }


  }
  n=n+1;
}
printf("header('content-type: image/png');\n");
printf("ImagePng($img);\n");
printf("ImageDestroy($img);\n");
printf("\n?>\n\n\n");

//clickable map
//last heard
for(i=0;i<MAX_STATIONS;i++){sort[i]=1;}
n=1;
while((n<=18)&&(n<=num_stations)){
  max=0;
  tt=0;
  for(j=0;j<MAX_STATIONS;j++){
   if(sort[j]&&(stations[j].minutes>tt)){max=j;tt=stations[j].minutes;}
  }
  sort[max]=0;

  printf("<area shape='rect' alt='' coords='690,%d,790,%d' href=' ' ",20*n+240,20*n+260);
  printf("onmouseover=\"gettip('<b>");
  for(i=0;i<6;i++){napis[i]=stations[max].sign[i];}
  napis[6]=0;
  printf(napis);
  if(stations[max].ssid>0){
	printf("-%d",stations[max].ssid);
  }
  printf("</b> &nbsp;&nbsp;");
  tt=stations[max].dist/10;
   if(stations[max].dist<999999)
    {
      printf("%d.%1dkm/%003d ",tt,stations[max].dist-10*tt,stations[max].namiar);

    }
  printf("<br>");
  tt=stations[max].minutes;
  tt=(timeval-tt)/600;
  printf("Last heard %3d min. ago  ",tt);
  printf("<br>");
  printf("Status:");
  printf(stations[max].info);

  printf("')\" />\n");


  n=n+1;
}
//radar
range=15;
n=1;
while(n<=num_stations){

  if(stations[n].dist<150){

    tt=stations[n].dist;
    tt=(tt*30)/range;
    ff=-1.0/180.0*3.1415926;
    si=((float)(stations[n].namiar))*ff;  	
    co=cos(si);	
    si=sin(si);
    x1=-(float)tt*si+340;
    y1=340-(float)tt*co;

    printf("<area shape='circle' alt='' coords='%d,%d,10' href=' ' onmouseover=\"gettip('<b>",x1,y1);

  for(i=0;i<6;i++){napis[i]=stations[n].sign[i];}
  napis[6]=0;
  printf(napis);
  if(stations[n].ssid>0){
	printf("-%d",stations[n].ssid);
  }
  printf("</b> &nbsp;&nbsp;");
  printf("<br>");
  tt=stations[n].minutes;
  tt=(timeval-tt)/600;
  printf("Last heard %3d min. ago  ",tt);
  printf("<br>");
  printf("Status:");
  printf(stations[n].info);

  printf("')\" />\n");
  }
  n=n+1;
}
printf("\n\n");

aprs_data_flag=adm;

//restore serial
 if(serial0_opt==0)
 {

  //U0LCR = 0x83;
  //U0DLM = 3;
  //U0DLL = 0;                    // 4800
  //U0LCR = 0x03;
 }

}//www



void station_serial_list()
{
int i,j,n,max,adm;
char sort[MAX_STATIONS];
//char napis[20];

unsigned long tt;

adm=aprs_data_flag;
aprs_data_flag=1;
//U0LCR = 0x83;
//U0DLL = 64;                     // 57600
//U0DLM = 0;
//U0LCR = 0x03;
printf("\nAPRS Navigator (C) T.Kuehn 2005-2006\n");
printf("Station list (%d)\n",num_stations);

for(i=0;i<MAX_STATIONS;i++){sort[i]=1;}
n=1;
while(n<=num_stations){
  max=0;
  tt=0;
  for(j=0;j<MAX_STATIONS;j++){
   if(sort[j]&&(stations[j].minutes>tt)){max=j;tt=stations[j].minutes;}
  }
  sort[max]=0;

  for(i=0;i<6;i++){napis[i]=stations[max].sign[i];}
  napis[6]=0;
  printf(napis);
  if(stations[max].ssid>0){
	printf("-%2d ",stations[max].ssid);
  } else {
    printf("    ");
  }
  switch(stations[max].type)
  {
   case 0 :	printf("DIRECT    ");break;
   case 1 :
   			if(stations[max].pathvalid){printf("via-");for(i=0;i<6;i++){printf("%c",stations[max].path[i]);};}
			 else {printf("          ");}
			break;
   case 2 : printf(" (");
   			if(stations[max].pathvalid){for(i=0;i<6;i++){printf("%c",stations[max].path[i]);};}
			 else {printf(" ??   ");}
   			printf(") ");break;
  }

  printf("  %3d  ",stations[max].count);
  print_symbol(stations[max].symbol); printf(" ");
  print_frametype(stations[max].frame);
  tt=(timeval-tt)/600;
  printf(" %3d min. ago  ",tt);
  tt=stations[max].dist/10;
  if(stations[max].dist<999999){printf("%5ld.%1dkm/%003d ",tt,stations[max].dist-10*tt,stations[max].namiar);}
   else {printf("              ");}
  //opcje testowe
  printf("slot:%4d ",stations[max].slot);

  //
  printf(stations[max].info);
  printf("\n");
  n=n+1;
}
printf("\n");

//opcje testowe
printf("UNSORTED\n");
for(max=0;max<num_stations;max++){
  printf("%4d ",max);
  for(i=0;i<6;i++){napis[i]=stations[max].sign[i];}
  napis[6]=0;
  printf(napis);
  printf("\n");
}
//

aprs_data_flag=adm;

//restore serial
 if(serial0_opt==0)
 {

  //U0LCR = 0x83;
  //U0DLM = 3;
  //U0DLL = 0;                    // 4800
  //U0LCR = 0x03;
 }

}







/* zwraca numer stacji w bazie na pozycji kursora */
char station_list(char startrow,char startcol,char cursor)
{
int i,j,n,max,sy;
char napis[42];
unsigned long tt;
char retn;

#define SLFNT 5
#define SLHGHT 10

LCDselect(1);
LCDfixed(1);
aprs_data_flag=1;
//for(i=0;i<MAX_STATIONS;i++){sort[i]=i;}
//station_sort(0);
n=1;
while(n<=filtered_stations){
  /*
  max=0;
  tt=0;
  for(j=0;j<MAX_STATIONS;j++){
   if(sort[j]&&(stations[j].minutes>tt)){max=j;tt=stations[j].minutes;}
  }
  sort[max]=0;*/
  max=sort[n];
  sy=112-SLHGHT*(n-startrow+1);
  if((n>=startrow)&&(sy>=0)){

   if(cursor>0){
      cursor--;
      if(cursor==0){retn=max;};
   }
   LCDcolor_background=(nightv ? BLACK : WHITE);
   LCDcolor_foreground=(nightv ? GREEN : BLACK);

   if(stations[max].type==0){ //direct
     LCDcolor_foreground=WHITE;
   }
   if(stations[max].type==2){ //direct&rep
     LCDcolor_foreground=YELLOW;
   }
   if((stations[max].opt&0x02)==2){ //object
     LCDcolor_foreground=BLUE;
   }
   LCDfixed(1);
   LCDyx(sy,6);
  for(i=0;i<6;i++){pchar(F_8X10,0,stations[max].sign[i],-1);}
  //napis[6]=0;
  //LCDyxtxt(SLFNT,sy,6,napis);
  LCDcolor_foreground=(nightv ? WHITE : BLACK);
  cursorx++;
  cursory=cursory+1;
  if(stations[max].ssid>0){
	sprintf(napis,"-%2d ",stations[max].ssid);

    LCDtxt(F_SMALL,napis);
  } else {
    LCDtxt(SLFNT,"  ");
  }
  cursory=cursory-1;
  LCDcolor_foreground=(nightv ? GREEN : BLACK);
  cursorx=64;
  if(startcol<2){
    show_symbol(stations[max].symbol);
    LCDtxt(SLFNT," ");
  }
  if(startcol<3){
   LCDchr(SLFNT,' ');
   show_frametype(stations[max].frame);
   LCDtxt(SLFNT,"  ");
  }
  LCDfixed(1);
  if(startcol<4){
   LCDtxt(SLFNT,"-     ");
  }
  if(startcol<5){

   if(stations[max].pathvalid)
    { LCDcolor_foreground=(nightv ? (rgb(0,50,0)) : BLACK);
      for(i=0;i<6;i++){LCDchr(0,stations[max].path[i]);};
      LCDchr(SLFNT,' ');
    } else {
     LCDtxt(SLFNT,"-      ");
    }
			
  }
  if((startcol<6)&&(cursorx<170)){
    tt=stations[max].dist/10;
    if(stations[max].dist<999999){
      sprintf(napis,"%4ld",tt);
      LCDtxt(SLFNT,napis);
      //LCDcolor_foreground=GREY;
      j=cursorx;
      LCDchr(0,' ');  //kasowanie poprzedniego napisu
      cursorx=j;
      sprintf(napis,".%1d",stations[max].dist-10*tt);
      LCDtxt(F_SMALL,napis);
      cursorx=j+7;
    }
     else {LCDtxt(SLFNT,"     ");}
  }
  LCDcolor_foreground=(nightv ? GREEN : BLACK);
  if((startcol<7)&&(cursorx<170)){
    if(stations[max].dist<999999){
      sprintf(napis,"%3d ",stations[max].namiar);
      LCDtxt(SLFNT,napis);
    }
     else {LCDtxt(SLFNT,"    ");}
  }
  if((startcol<8)&&(cursorx<170)){
      sprintf(napis,"%3d ",stations[max].count);
      LCDtxt(SLFNT,napis);
  }

  //wipe rest of line
  LCDcolor_foreground=BLACK;
  if(cursorx<175)LCDfillrectangle(cursorx,cursory,175,cursory+SLHGHT);
  LCDcolor_foreground=WHITE;

  if((startcol<9)&&(cursorx<170)){
    i=0;
    while((stations[max].info[i]>=32)&&(i<MAX_INFO)&&(cursorx<174))
    {
      LCDchr(F_SMALL,stations[max].info[i]);
      i++;
    }
    //sprintf(napis,"%s",stations[max].info);
    //LCDtxt(F_SMALL,napis);
  }


  }//if n
  n=n+1;
}//while
 LCDcolor_foreground=(nightv ? BLACK : WHITE);
 if(cursory>0)LCDfillrectangle(0,cursory,175,0);
 return(retn);
}


//this function returns free position in txqueue
//or 255 if there is no place
char gettxqueuepos()
{
char i;

  i=0;
  while((txqueue[i].status>0)&&(i<TXQUEUELEN)){i++;}
  if(i==TXQUEUELEN){i=255;}
  return(i);
}


void send_txqueue()
{
char i;
char ss[2];

  aprs_write(0x100,200,txqueue[0].data);

  ss[0]=1;
  aprs_write(0x204,1,ss);//pttmode
  ss[0]=trx_txheader;
  aprs_write(0x209,1,ss);//txheader
  ss[0]=trx_bitdelay;
  aprs_write(0x20A,1,ss);//bitdelay
  ss[0]=trx_txdelay;
  aprs_write(0x208,1,ss);
  ss[0]=trx_rxtune;
  aprs_write(0x207,1,ss);
  ss[0]=1-trx_squelch;
  aprs_write(0x20B,1,ss);

  if(txenable){
   ss[0]=1;
   aprs_write(0x202,1,ss);

   setsound(soundtab[SND_TRANSMIT]);
  }
  //resort txqueue
  for(i=0;i<TXQUEUELEN-1;i++){
    txqueue[i]=txqueue[i+1];
  }
  txqueue[TXQUEUELEN-1].status=0;
}

char *inject;

//this function runs on app level
void application_frame_send(char *ss)
{
#ifdef debug
    printf("application_frame_send\n");
#endif
 inject=ss;
 frame_injection=1;
 while(frame_injection){waitms(1);}
#ifdef debug
    printf("application_frame_send:done\n");
#endif
}

//this function runs on int level
void application_frame_get()
{
#ifdef debug
    printf("application_frame_get\n");
#endif
  char n,i;
  n=gettxqueuepos();
  if(n==255){return;}
#ifdef debug
    printf("application_frame_get...\n");
#endif

  for(i=0;i<250;i++){
    txqueue[n].data[i]=*inject++;
  }
  txqueue[n].status=1;
  frame_injection=0;
}





void send_beacon()
{
unsigned char ss[9];
unsigned char str[9];
char c,c1,i,n,spos,sym1,sym2;

  n=gettxqueuepos();
  if(n==255){return;}

  sprintf(txqueue[n].data,"xxxxxxxxxxxxxxRELAY 0WIDE  0# =5225.75N/01656.20E-FHI-TRAK v0.72");

  for(c=0;c<6;c++){txqueue[n].data[c]=udest[c];}
  txqueue[n].data[6]='0';
  for(c=0;c<6;c++){txqueue[n].data[7+c]=stations[0].sign[c];}
  txqueue[n].data[13]=stations[0].ssid+'0';
  spos=14;
  //select next valid path or no path
  c=pathuse+1;
  if(c>2){c=0;}
  while((c!=pathuse)&&(!paths[c].enable)){
    c++;
    if(c>2){c=0;}
  }
  if(paths[c].enable)
  {
    i=0;
    while(i<7*paths[c].plen)
    {
      txqueue[n].data[spos]=paths[c].spath[i];
      spos++;
      i++;
    }
    pathuse=c; //zapisz co bylo uzyte
  }
  txqueue[n].data[spos++]='#';
  txqueue[n].data[spos++]=' ';
  sym1=symtable[stations[0].symbol*3];
  sym2=symtable[stations[0].symbol*3+1];
  //pozycja lub brak
  //askgpsdata=1;while(askgpsdata==1){};  //read gps buffers

  switch(position_mode&0x0F){
  case 0x00:if(gpsfix||gpspresent){
              txqueue[n].data[spos++]='=';
              for(i=0;i<8;i++){txqueue[n].data[spos++]=gpslat[i];};
              txqueue[n].data[spos++]=sym1;
              for(i=0;i<9;i++){txqueue[n].data[spos++]=gpslon[i];};
              txqueue[n].data[spos++]=sym2;
             } else
             {  //use static
               txqueue[n].data[spos++]='=';
               for(i=0;i<7;i++){
                 switch(posuse){
                 case 0:str[i]=posbank1[i];break;
                 case 1:str[i]=posbank2[i];break;
                 case 2:str[i]=posbank3[i];break;
                 };
               };
               if(str[6]&0xF0>0){c='S';}else{c='N';}  //S
               sprintf(ss,"%02d%02d.%02d%c",str[0],str[1],str[2],c);
               for(i=0;i<8;i++){txqueue[n].data[spos++]=ss[i];}
               txqueue[n].data[spos++]=sym1;
               if(str[6]&0x0F>0){c1='W';}else{c1='E';}
               sprintf(ss,"%03d%02d.%02d%c",str[3],str[4],str[5],c1);
               for(c=0;c<9;c++){txqueue[n].data[spos++]=ss[c];}
               txqueue[n].data[spos++]=sym2;
             };
            break;
  case 0x01:if(gpsfix){
              txqueue[n].data[spos++]='=';
              for(i=0;i<8;i++){txqueue[n].data[spos++]=gpslat[i];};
              txqueue[n].data[spos++]=sym1;
              for(i=0;i<9;i++){txqueue[n].data[spos++]=gpslon[i];};
              txqueue[n].data[spos++]=sym2;
             } else
             {  //status only
               txqueue[n].data[spos++]='>';
             };
            break;
  case 0x02://static;
             txqueue[n].data[spos++]='=';
               for(i=0;i<7;i++){
                 switch(posuse){
                 case 0:str[i]=posbank1[i];break;
                 case 1:str[i]=posbank2[i];break;
                 case 2:str[i]=posbank3[i];break;
                 };
               };
               if(str[6]&0xF0>0){c='S';}else{c='N';}  //S
               sprintf(ss,"%02d%02d.%02d%c",str[0],str[1],str[2],c);
               for(i=0;i<8;i++){txqueue[n].data[spos++]=ss[i];}
               txqueue[n].data[spos++]=sym1;
               if(str[6]&0x0F>0){c1='W';}else{c1='E';}
               sprintf(ss,"%03d%02d.%02d%c",str[3],str[4],str[5],c1);
               for(c=0;c<9;c++){txqueue[n].data[spos++]=ss[c];}
               txqueue[n].data[spos++]=sym2;
               break;

  }
  //gpslock=0;


  //status
  c=statuse+1;
  if(c>2){c=0;}
  while((c!=statuse)&&(!statusy[c].enable)){
    c++;
    if(c>2){c=0;}
  }
  if(statusy[c].enable)
  {
   i=0;
   while(statusy[c].text[i]>0){txqueue[n].data[spos++]=statusy[c].text[i++];}
   statuse=c;
  }
#ifdef DEMO1
  if(1){
#else
  if(navmark){
#endif
  //navigator mark
  txqueue[n].data[spos++]=' ';
  txqueue[n].data[spos++]='{';
  txqueue[n].data[spos++]='N';
  txqueue[n].data[spos++]='A';
  txqueue[n].data[spos++]='V';
  txqueue[n].data[spos++]='}';
  }
  txqueue[n].data[spos++]=0;


  txqueue[n].status=1;

  for(i=0;i<255;i++){txbuf[i]=txqueue[n].data[i];}
  /*
  aprs_write(0x100,200,txbuf);
  ss[0]=1;
  aprs_write(0x204,1,ss);//pttmode
  ss[0]=trx_txheader;
  aprs_write(0x209,1,ss);//txheader
  ss[0]=trx_bitdelay;
  aprs_write(0x20A,1,ss);//bitdelay

  ss[0]=1;
  aprs_write(0x202,1,ss);
  */
  my_beacon_count++;
  self_digipeated=0;
  for(i=0;i<6;i++){
    if(selfdigis[i].status==1){selfdigis[i].status=2;}
  }

  stations[0].slot=get_freepage();          //save to exteral flash

  spirequest=1;
  spi_txbuf_to_flash_rq=stations[0].slot;
}





void transmit_message()
{
char ri,n,i;
unsigned int sp;

  sp=spi_flash_to_msbuf_rq-10000;
  i=0;
  while((i<TX_MESSAGEBUF)&&(txmessages[i].slot!=sp)){i++;}
  if(i<TX_MESSAGEBUF)
  {
    ri=i;
    n=gettxqueuepos();
    if(n==255){return;}
   //flash_read(txqueue[n].data,txmessages[id].slot,254);
    //sprintf(txqueue[n].data,"xxxxxxxxxxxxxxRELAY 0WIDE  0# =5225.75N/01656.20E-FHI-TRAK v0.72");
    for(i=0;i<255;i++){txqueue[n].data[i]=msbuf[i];}
    txqueue[n].status=1;
    txmessages[ri].repcount--;
    if(txmessages[ri].repcount==0){
      txmessages[ri].status=2;
    }
    else
    {
      txmessages[ri].time=timeval+100;
    }
  }
  spi_flash_to_msbuf_rq=0;
}



void do_repeat()
{
#ifdef digidebug
char ss[50];
#endif
unsigned int sp;
char ri,i,k,n;


  sp=spi_flash_to_rpbuf_rq-10000;
  i=0;
  while((i<DIGIQUEUELEN)&&((rpque[i].slot!=sp)||(rpque[i].rpflag!=1))){i++;}
  if(i<DIGIQUEUELEN) //found the right frame
  {
#ifdef digidebug
       n=rpque[i].id;
       sprintf(ss,"REPEATING:(%d):",i);
       printf("%s",ss);
       for(k=0;k<6;k++){putchar(stations[n].sign[k]);}
       printf("\n");
#endif
    ri=i;
    switch(aliases[rpque[ri].alias].aliastype){
    case 0 :for(k=0;k<6;k++){rpbuf[rpque[ri].aliaspos+k]=stations[0].sign[k];}
            rpbuf[rpque[ri].aliaspos+6]=0x70+stations[0].ssid;
            break;
    case 1 :rpbuf[rpque[ri].aliaspos+6]--;
            if((rpbuf[rpque[ri].aliaspos+6]&0x0F)==0)
              {rpbuf[rpque[ri].aliaspos+6]=0x70+stations[0].ssid;}
            break;
    case 2 :rpbuf[rpque[ri].aliaspos+6]--;
            if((rpbuf[rpque[ri].aliaspos+6]&0x0F)==0)
              {rpbuf[rpque[ri].aliaspos+6]=0x70+stations[0].ssid;}
            for(k=255;k>rpque[ri].aliaspos+6;k--){rpbuf[k]=rpbuf[k-7];}
            for(k=0;k<6;k++){rpbuf[rpque[ri].aliaspos+k]=stations[0].sign[k];}
            rpbuf[rpque[ri].aliaspos+6]=0x70+stations[0].ssid;
            break;
    }

    //for(k=0;k<250;k++){rpbuf[k]=rpbuf[k+1];}

     //k=0;while((rpbuf[k]!=0)&&(k<210)){putchar(rpbuf[k]);k++;}
      //printf("\n");
    rpbuf[249]=0;

    n=gettxqueuepos();
    if(n<255){
      for(i=0;i<250;i++){txqueue[n].data[i]=rpbuf[i];}
      if(repeater_enable){txqueue[n].status=1;}
    }
#ifdef digidebug
    printf("Placed in queue: %d\n",n);
#endif


    rpque[ri].rpflag=2;
    dtraffic[0]++;

    //uzupelnij liste memdigi
    for(i=5;i>0;i--){
      memdigi[i]=memdigi[i-1];
    }
    memdigi[0].status=rpque[ri].alias+1;  //0 - to pusty slot a aliasy sa od zera
    memdigi[0].time=timeval;
    n=rpque[ri].id;
    for(k=0;k<6;k++){memdigi[0].alias[k]=stations[n].sign[k];}
    memdigi[0].alias[6]=stations[n].ssid;

    if(serial0_opt==4)  //digi monitor
    {
      sprintf(napis,"(%d) ",aliases[rpque[ri].alias].aliastype);
      printf(napis);
       i=1;while((i<250)&&(rpbuf[i]!=0x03)){
      if((rpbuf[i]<32)||(rpbuf[i]>126)){putchar('.');}
      else{putchar(rpbuf[i]);}
      i++;
      }
      while((i<250)&&(rpbuf[i]>0)){
      if((rpbuf[i]<32)||(rpbuf[i]>126)){putchar('.');}
      else{putchar(rpbuf[i]);}
      i++;
      }
      printf("\n");
    }


  }
  spi_flash_to_rpbuf_rq=0;
}


void send_ans(char *emem)
{


char c,i,n,spos;

  n=gettxqueuepos();
  if(n<255){
  sprintf(txqueue[n].data,"xxxxxxxxxxxxxxRELAY 0WIDE  0# =5225.75N/01656.20E-FHI-TRAK v0.72");

  for(c=0;c<6;c++){txqueue[n].data[c]=udest[c];}
  txqueue[n].data[6]='0';
  for(c=0;c<6;c++){txqueue[n].data[7+c]=stations[0].sign[c];}
  txqueue[n].data[13]=stations[0].ssid+'0';
  spos=14;
  //select next valid path or no path
  c=pathuse+1;
  if(c>2){c=0;}
  while((c!=pathuse)&&(!paths[c].enable)){
    c++;
    if(c>2){c=0;}
  }
  if(paths[c].enable)
  {
    i=0;
    while(i<7*paths[c].plen)
    {
      txqueue[n].data[spos]=paths[c].spath[i];
      spos++;
      i++;
    }
    pathuse=c; //zapisz co bylo uzyte
  }
  txqueue[n].data[spos++]='#';
  txqueue[n].data[spos++]=' ';

  txqueue[n].data[spos++]='>';
  for(i=0;i<8;i++){txqueue[n].data[spos++]=*(emem++);}

  txqueue[n].data[spos++]=0;
  if(queryenable){txqueue[n].status=1;}

  }//if c
  else
  {
   debugmsg(WARNLEVEL,"Tx queue full");
  }

}


void send_ack(char m, char *ss)
{
char txt[4];
char c,c1,i,n,spos;

  n=gettxqueuepos();
  if(n<255){
  sprintf(txqueue[n].data,"xxxxxxxxxxxxxxRELAY 0WIDE  0# =5225.75N/01656.20E-FHI-TRAK v0.72");
  for(c=0;c<6;c++){txqueue[n].data[c]=udest[c];}
  txqueue[n].data[6]='0';
  for(c=0;c<6;c++){txqueue[n].data[7+c]=stations[0].sign[c];}
  txqueue[n].data[13]=stations[0].ssid+'0';
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
      txqueue[n].data[spos]=paths[c].spath[i];
      spos++;
      i++;
    }
    msgpathuse=c; //zapisz co bylo uzyte
  }
  txqueue[n].data[spos++]='#';
  txqueue[n].data[spos++]=' ';

  txqueue[n].data[spos++]=':';
  i=0;c1=0;
  while((stations[m].sign[i]>' ')&&(i<6)){
    txqueue[n].data[spos++]=stations[m].sign[i];
    i++;
    c1++;
  }
  if(stations[m].ssid>0){
    sprintf(txt,"-%d",stations[m].ssid);
    i=0;
    while(txt[i]>0){
      txqueue[n].data[spos++]=txt[i];
      i++;
      c1++;
    }
  }
  while(c1<9){
    txqueue[n].data[spos++]=' ';
    c1++;
  }
  txqueue[n].data[spos++]=':';
  txqueue[n].data[spos++]='a';
  txqueue[n].data[spos++]='c';
  txqueue[n].data[spos++]='k';
  for(i=0;i<5;i++){
   txqueue[n].data[spos++]=ss[i];
  }

  txqueue[n].data[spos++]=0;
  txqueue[n].status=1;

  }//if c
  else
  {
   debugmsg(WARNLEVEL,"Tx queue full");
  }

}


char header_decode(char limit, unsigned char *gdzie)
//zwraca 1 jesli ramka byla repeatowana
{
return 0;
}

static char emem[8];
static char ememvalid=0;

static unsigned long ev[2];
static unsigned long ek[4];



void xtea(long N)
{
unsigned long y=ev[0];
unsigned long z=ev[1];
unsigned long DELTA=0x9e3779b9;

//klucz
//ek[0]=0x00000000;
//ek[1]=0x00000000;
//ek[2]=0x00000000;
//ek[3]=0x553E1234;

if(N>0){
unsigned long limit=DELTA*N;
unsigned long sum=0;
while(sum!=limit){
 y+=((z<<4 ^ z>>5) + z) ^ (sum + ek[sum&3]);
 sum+=DELTA;
 z+=((y<<4 ^ y>>5) + y) ^ (sum + ek[(sum>>11)&3]);
}
}
else
{
unsigned long sum=DELTA*(-N);
while(sum){
 z-=((y<<4 ^ y>>5) +y ) ^ (sum + ek[(sum>>11)&3]);
 sum-=DELTA;
 y-=((z<<4 ^ z>>5) +z ) ^ (sum + ek[sum&3]);
}
}
ev[0]=y;
ev[1]=z;

}


void process_query(char n, char msg)
{
const char q1[]="APRSS";
const char q2[]="APRSP";
const char q3[]="STATG";
const char q4[]="STATP";
const char q5[]="EXEC";
const char c1[]="BC"; //send beacon
const char c2[]="DT"; //disable tx
const char c3[]="AD";
char ss[9],sc[9];
char i;
unsigned long tt;

  for(i=0;i<5;i++){ss[i]=buffer[msg+1+i];}
  if((!memcmp(ss,q1,5))||(!memcmp(ss,q2,5)))
  {//general query recognized
    next_beacon_time=timeval+20;
  }

  if(!memcmp(ss,q3,5))
  {
   //odpowiedz zawiera czas i 4-cyfrowy losowy kod
   //na tej bazie wysylany jest zaszyfrowany kod wraz
   //z poleceniem zdalnego sterowania
   tt=utimeval;
   tt=tt&0xFFF;
   sprintf(ss,"%04d",tt);
   //zapisz wygenerowany kod i czas do pozniejszego sprawdzenia
   for(i=0;i<4;i++){emem[i+4]=ss[i];}
   sprintf(ss,"1234");
   for(i=0;i<4;i++){emem[i]=ss[i];}
   send_ans(emem);
   //printf("emem:");for(i=0;i<8;i++){putchar(emem[i]);}printf("\n");
   ememvalid=1;
  }
  if((!memcmp(ss,q4,5))&&(ememvalid))
  { //polecenie z kodem
    //format:2litery polecenia+8znakow kodu
    //utworz ciag do deszyfrowania z pamieci
    ememvalid=0;
    ev[0]=0;
    ev[1]=0;
    for(i=0;i<8;i++){
      ev[0]=ev[0]<<4;
      ev[0]+=emem[i]-'0';
    }
    //encrypt
    //klucz
    ek[0]=0x00000000;
    ek[1]=0x00000000;
    ek[2]=0x00000000;
    ek[3]=0x553E1234;
    xtea(4);
    sprintf(sc,"%08X",ev[0]);
    for(i=0;i<8;i++){ss[i]=buffer[msg+8+i];}
    //printf("sc:");for(i=0;i<8;i++){putchar(sc[i]);}printf("\n");
    //printf("ss:");for(i=0;i<8;i++){putchar(ss[i]);}printf("\n");
    //porownaj
    if(!memcmp(sc,ss,8))
    { //kod zgodny, wykonuje polecenie
      //polecenia:
      //- wlacz/wylacz digi
      //- wlacz/wylacz beacon
      send_ans(ss);
      //printf("Zgodny\n");
    }
  }
  if(!memcmp(ss,q5,4))
    {
      //printf("??\n");
      //sprintf(sc,"%08X",rkey[0].key);
      //for(i=0;i<8;i++){
      // printf("%c %c\n",ss[i],sc[i]);
      //}
      for(i=0;i<8;i++){ss[i]=buffer[msg+7+i];}
     //moze to byc jeden z jednorazowych kodow
      i=255;
      sprintf(sc,"%08X",rkey[0].key);
      if((!memcmp(ss,sc,8))&&(rkey[0].enable)){
        i=0;
      }
      sprintf(sc,"%08X",rkey[1].key);
      if((!memcmp(ss,sc,8))&&(rkey[1].enable)){
        i=1;
      }
      sprintf(sc,"%08X",rkey[2].key);
      if((!memcmp(ss,sc,8))&&(rkey[2].enable)){
        i=2;
      }
      sprintf(sc,"%08X",rkey[3].key);
      if((!memcmp(ss,sc,8))&&(rkey[3].enable)){
        i=3;
      }

      if(i<255){
       rkey[i].enable=0;
       send_ans(ss);
       ss[0]=buffer[msg+5];
       ss[1]=buffer[msg+6];
       if(!memcmp(ss,c1,2)){next_beacon_time=timeval+10;}
       if(!memcmp(ss,c2,2)){txenable=0;}
       if(!memcmp(ss,c3,2)){admin_code=1;}
      }

      //albo kod administratora (czyli z innym kluczem)
      //polecenia administratora:
      //- podaj swoj prawdziwy znak
      //- blokuj traker
      //- wyswietl reklame
      /*
      ev[0]=0;ev[1]=0;
      for(i=0;i<8;i++){
        ev[0]=ev[0]*10;
        ev[0]=ev[0]+buffer[msg+8+i];
      }
      //klucz
      ek[0]=0x00000000;
      ek[1]=0x00000000;
      ek[2]=0x0ABC34F0;
      ek[3]=0x84BC091A;
      xtea(29);
      sprintf(sc,"%08X",ev[0]);
      for(i=0;i<8;i++){
        ss[i]=buffer[msg+16+i];
      }
      if(!memcmp(ss,sc,8)){
       //admin mode
       ss[0]=buffer[msg+6];
       ss[1]=buffer[msg+7];
       if(!memcmp(ss,c1,2)){next_beacon_time=timeval+10;}
       if(!memcmp(ss,c2,2)){txenable=0;}
       if(!memcmp(ss,c3,2)){
         admin_code=1;
       }
      }
      */
    }


}


void add_message(char m, char msg_start)
//m - direct pointer to stations table
{
char ss[7];
char c,i,j,ack,msn;
char forme;

  stations[m].frame=':';
  message_count++;
  forme=0;
#ifdef msgdebug
      printf("Message \n");
#endif
  i=0;
  while((i<6)&&\
       (buffer[msg_start+i+1]!=' ')&&\
       (buffer[msg_start+i+1]!='-'))
  {
    ss[i]=buffer[msg_start+i+1];
    i++;
  }
  j=i;
  while(j<6){ss[j]=' ';j++;}
  //printf("Message to:");
  //for(j=0;j<6;j++){putchar(ss[j]);}
  //printf("=\n");
  if(memcmp(stations[0].sign,ss,6)==0) //for me?
  {
    //now verify ssid
    c=0;
    if(buffer[msg_start+i+1]=='-')
    {
      c=buffer[msg_start+i+2]-'0';
      if(buffer[msg_start+i+3]>' '){
        c=10*c+buffer[msg_start+i+3]-'0';
      }
    }
#ifdef msgdebug
    printf(" ssid:%d\n",c);
#endif
    if(c==stations[0].ssid){ //for me
#ifdef msgdebug
      printf("Message to me -%d\n",c);
#endif
      forme=1;
      //is this ack or rej?
      i=msg_start+11;
      ack=0;
      if(((buffer[i]=='a')&&(buffer[i+1]=='c')&&(buffer[i+2]=='k'))||\
         ((buffer[i]=='r')&&(buffer[i+1]=='e')&&(buffer[i+2]=='j')))
	 {
           ack=1;
#ifdef msgdebug
           printf("ACK received:");
#endif
          for(c=0;c<5;c++){
            if(buffer[i+3+c]>' '){ss[c]=buffer[i+3+c];}
             else {ss[c]=' ';}
#ifdef msgdebug
      printf("%c",ss[c]);
#endif
          }
#ifdef msgdebug
          printf(":\n");
#endif
          for(c=0;c<TX_MESSAGEBUF;c++){
#ifdef msgdebug
            printf("Searching %d :",c);
          for(j=0;j<5;j++){putchar(ss[j]);}
          printf(":");
          for(j=0;j<5;j++){putchar(txmessages[c].ackid[j]);}
          printf(":\n");
#endif
            if(!memcmp(txmessages[c].ackid,ss,5)){
              txmessages[c].ackstatus=1;
#ifdef msgdebug
      printf("ACKked\n");
#endif
              if(buffer[i]=='r'){txmessages[c].ackstatus=2;}
              txmessages[c].status=3;
            }
          }
         }
      //query?
      if(buffer[i]=='?'){
        process_query(m,i);
        ack=1;          //ack tutaj jako flaga blokuje kolejne opcje
      }
      if(!ack){
        //verify message id
        j=i;
        while((buffer[j]!='{')&&(buffer[j]>0)){j++;}
        if(buffer[j]=='{'){
           //ack needed
           c=0;
           while((buffer[j+c+1]>0)&&\
                 (buffer[j+c+1]>' ')&&\
                   (c<5)){
                     ss[c]=buffer[j+c+1];
                     c++;
                   }
           while(c<5){ss[c++]=' ';}
           ack=1;
           send_ack(m,ss);
#ifdef msgdebug
           printf(" ack:");
           for(c=0;c<5;c++){putchar(ss[c]);};
           printf(":\n");
#endif
        } //if {
        //add it to my messages list
        //search empty or oldest or same
        c=0;
        msn=0;
        while(c<MAX_MESSAGES){
          if(messages[c].status==0){msn=c+1;}
          if((messages[c].stid==m)&&\
             (memcmp(ss,messages[c].ackid,5)==0))
          {
#ifdef msgdebug
      printf("Duplicate\n");
#endif
            msn=c+1;
            c=MAX_MESSAGES;
          }
          c++;
        }//while
        if(msn>0){ //no free, remove unread
          while(c<MAX_MESSAGES){
          if(messages[c].status==2){msn=c+1;}
          c++;
          }//while
        }
        if(msn==0){
          //send reject
#ifdef msgdebug
      printf("Reject - table full\n");
#endif
        }
        else
        { //message accepted
          setsound(soundtab[SND_MESSAGE]);
          msn=msn-1;
          messages[msn].status=1;
          messages[msn].stid=m;
          messages[msn].slot=stations[m].slot;
          messages[msn].ackstatus=ack;
          for(i=0;i<5;i++){messages[msn].ackid[i]=ss[i];}
          messages[msn].time=timeval;
          messages[msn].option=0;
#ifdef msgdebug
      printf("Store message\n");
#endif
        }

      }
    }
  } //if memcmp


 //
 //this part receives all messages regardless of adresee
 //no acking
 //


    if((!forme)&&allmessages){

      //is this ack or rej?
      i=msg_start+11;
      ack=0;
      if(((buffer[i]=='a')&&(buffer[i+1]=='c')&&(buffer[i+2]=='k'))||\
         ((buffer[i]=='r')&&(buffer[i+1]=='e')&&(buffer[i+2]=='j')))
	 {
           ack=1;
#ifdef msgdebug
      printf("Foreign ACK received\n");
#endif
         }
      if(!ack){
        //verify message id
        j=i;
        while((buffer[j]!='{')&&(buffer[j]>0)){j++;}
        if(buffer[j]=='{'){
           //ack needed
           c=0;
           while((buffer[j+c+1]>0)&&\
                 (buffer[j+c+1]>' ')&&\
                   (c<5)){
                     ss[c]=buffer[j+c+1];
                     c++;
                   }
           while(c<5){ss[c++]=' ';}
           ack=1;
#ifdef msgdebug
           printf(" foreign ack:");
           for(c=0;c<5;c++){putchar(ss[c]);};
           printf(":\n");
#endif
        } //if {
        //search empty or oldest or same
        c=0;
        msn=0;
        while(c<MAX_MESSAGES){
          if(messages[c].status==0){msn=c+1;}
          if((messages[c].stid==m)&&\
             (memcmp(ss,messages[c].ackid,5)==0))
          {
#ifdef msgdebug
      printf("Foreign Duplicate\n");
#endif
            msn=c+1;
            c=MAX_MESSAGES;
          }
          c++;
        }//while
        if(msn==0){
          //send reject
#ifdef msgdebug
      printf("Reject foreign- table full\n");
#endif
        }
        else
        { //message accepted
          setsound(soundtab[SND_MESSAGE]);
          msn=msn-1;
          messages[msn].status=1;
          messages[msn].stid=m;
          messages[msn].slot=stations[m].slot;
          messages[msn].ackstatus=0;
          for(i=0;i<5;i++){messages[msn].ackid[i]=ss[i];}
          messages[msn].time=timeval;
          messages[msn].option=1;
#ifdef msgdebug
      printf("Store foreign message\n");
#endif
        }

      }//if !ack
    } //if allmessages

#ifdef msgdebug
      printf("Message end\n");
#endif

}





float convert_position(char latlon, char m)
{
char c1,c2;
long l,l1;
float f;
#define LAT	  0
#define LON	  1

 if(latlon==LAT){
  c1=stations[m].position[1];
  c2=stations[m].position[2];
  l=c1*100+c2;
  l1=(l)/6;
  l=stations[m].position[0];
  f=(float)l1/1000.0+(float)l;
  if(stations[m].position[6]&0xF0>0)  //S
  {
    f=-f;
  }
 } else {
  c1=stations[m].position[4];
  c2=stations[m].position[5];
  l=c1*100+c2;
  l1=(l)/6;
  l=stations[m].position[3];
  f=(float)l1/1000.0+(float)l;
  if(stations[m].position[6]&0x0F>0)  //W
  {
    f=-f;
  }

 }
 return f;
}


void parse_comment(char n,char start)
{
char c1;

#ifdef rsdebug
printf("procedure start: parse_comment\n");
#endif

  while(buffer[start]==' '){start++;}
  c1=0;
  while((buffer[c1+start]>=' ')&&(c1<(MAX_INFO-1))){
     stations[n].info[c1]=buffer[c1+start];
	 c1=c1+1;
  }
  if(c1>0) //preserve previous comment
  stations[n].info[c1]=0;
}

char stringcmp(char *s1,char *s2,char n)
{
char i;
char eq=1;
  for(i=0;i<n;i++){
   if((*s1)!=(*s2)){eq=0;}
   s1++;
   s2++;
   }
  return eq;
}



void decode_wx(char n, char msg)
{
short int tt;
  if(stations[n].symbol==32){stations[n].symbol='_';}
  while((buffer[msg]!='t')&&(msg<249)){msg++;}
  if(buffer[msg]=='t'){
   //verify no dots
    msg++;
    if((buffer[msg]!='.')&&(buffer[msg]!=' ')){
      tt=(buffer[msg+1]-'0')*10+(buffer[msg+2]-'0');
      if(buffer[msg]=='-'){tt=-tt;}
      else {tt=tt+100*(buffer[msg]-'0');}
      //convert to celsius
      tt=tt-32;
      tt=tt/1.8;
    }
    else
    {
      tt=NOTEMP;
    }
    stations[n].wxtemp=tt;
  }
}


void decode_query(char n, char msg)
{
const char q1[]="APRS?";
char ss[7];
char i;
unsigned long tt;

  for(i=0;i<5;i++){ss[i]=buffer[msg+1+i];}
  if(!memcmp(ss,q1,5)&&(buffer[msg+6]==0)) //general query only
  {//general query recognized
    tt=timeval;
    tt=tt&0xF;
    tt=tt*5;
    next_beacon_time=timeval+tt;
  }

}


void pos_extension(char n, char start)
{
char ss[3];
char phg[]="PHG";
char i;

    if((buffer[start]=='_')&&(buffer[start+4]=='/')){decode_wx(n,start);}
      else
      {
        start++;
        for(i=0;i<3;i++){ss[i]=buffer[start+i];}
        if((buffer[start+3]=='/')||(stringcmp(phg,ss,3))){start=start+7;}
        parse_comment(n,start);
      }
}



void dist_calc(char m,float lat2, float lon2, char recalc)
{
static float lat1,lon1;
float d;
unsigned long mdist;
char i,c;
double dd,de;

#ifdef rsdebug
printf("procedure start: dist_calc\n");
#endif

 //check moving
 c=0;
 for(i=0;i<7;i++){c=c+stations[m].position[i];}
 c=c&0x7F;
 if((c==(stations[m].moving&0x7F))||(stations[m].dist==999999))
 {
   stations[m].moving=0;
 } else
 {
   stations[m].moving=0x80;
 }
 stations[m].moving+=c;

 mdist=stations[m].dist;

 //mla=lat2;      //zapamietaj lat,lon
 //mlo=lon2;
 //mylat=52.4308;
 //mylon=16.7551;

 //konwertuj na radiany
 lon2=lon2/57.295779;
 lat2=lat2/57.295779;

 if(recalc){
 lon1=mylon/57.295779;
 lat1=mylat/57.295779;
 }
 //d=acos(cos(lat1)*cos(lat2)*cos(lon1-lon2)+sin(lat1)*sin(lat2));
 //d=cos(lat1)*cos(lat2);
 //d=d*cos(lon1-lon2);
 //d=d+(sin(lat1)*sin(lat2));
 //d=acos(d);
 //d=acos(cos(lat1-lat2)-(1-cos(lon1-lon2))*cos(lat1)*cos(lat2));

 d=(1.0-cos(lon1-lon2));
 //d=d*cos(lat1)*cos(lat2);
 //dd=cos(lat1-lat2)-d;

 dd=cos(lat1)*cos(lat2)+sin(lat1)*sin(lat2)-d;
 dd=acos(dd);


 stations[m].dist=dd*63667.22;//63780.0;
 lon2=lon1-lon2;	  //dlon
 //stations[m].dist=mlo*10000.0;

 //namiar
 lat2=sin(lat2)-cos(dd+lat1-1.5708);
 lat2=lat2/cos(lat1);
 lat2=lat2/sin(dd)+1.0;
 //de=sin(lat2)-cos(dd+lat1-1.5708);
 //de=de/cos(lat1);
 //de=de/sin(dd)+1.0;
 if((lat2>1)||(lat2<-1)){d=0;} else
 {
 d=acos(lat2)*57.295779;
 //lon2=lon1-lon2;	  //dlon
 if (lon2<0){lat2=-lon2;}else{lat2=lon2;}		//|dlon|
 if((lon2>0)&&(lat2<3.141592653)){d=360-d;}
 if((lat2>3.141592653)&&(lon2<0)){d=360-d;}
 }
 stations[m].namiar=(int)d;//floor(acos((sin(lat2)-cos(d+lat1-1.5708))/cos(lat1)/sin(d)+1));

 if(mdist<999999){  //eliminuje pierwszy pomiar
   if(mdist>stations[m].dist){stations[m].distchg=1;}
   if(mdist==stations[m].dist){stations[m].distchg=0;}
   if(mdist<stations[m].dist){stations[m].distchg=2;}
 }

}



void decode_position(char m,char msg_start)
//m - direct addr in station table
{
float lat2,lon2;
//unsigned int c;
long l;
char i,c1,c2,c3,c4;
int ii;

#ifdef rsdebug
printf("procedure start: decode_position\n");
#endif

 if ((stations[m].frame=='!')||(stations[m].frame=='='))					//bez czasu
 {
  stations[m].symbol=buffer[msg_start+19];
  pos_extension(m,msg_start+19);
 }
 else																	//z czasem
 {
  stations[m].symbol=buffer[msg_start+26];

  //zapisz comment
  pos_extension(m,msg_start+26);


  msg_start=msg_start+7;		//omin czas
 }
 //oblicz odleglosc

 //lat1=mylat;
 //lon1=mylon;
 //lat1=38.8449;
 //lon1=94.8679;
 //compressed position?
 if((buffer[msg_start+1]=='/')||\
    (buffer[msg_start+1]=='\\'))   //error?compressed position
 {
  /*
  c1='>';
  c2='*';
  c3='4';
  c4='>';
  */
  c1=buffer[msg_start+2];
  c2=buffer[msg_start+3];
  c3=buffer[msg_start+4];
  c4=buffer[msg_start+5];
  lat2=((float)c1-33.0)*1.97826; //57.3695
  l=((long)c2-33)*8281+((long)c3-33)*91+((long)c4-33); //9,19,29
  l=(l*100)/380926;
  lat2=90.0-(lat2+((float)l/100.0));

  c1=buffer[msg_start+6];
  c2=buffer[msg_start+7];
  c3=buffer[msg_start+8];
  c4=buffer[msg_start+9];
  /*
  c1='e';
  c2='c';
  c3='V';
  c4=';';
  */
  lon2=((float)c1-33.0)*3.95652;
  l=((long)c2-33)*8281+((long)c3-33)*91+((long)c4-33);
  l=(l*100)/190463;
  lon2=-180.0+(lon2+((float)l/100.0));
  //lon2=-180.0+lon2;
  stations[m].symbol=buffer[msg_start+10];
  ii=1; //do sparawdzenia !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 }
 else
 {
 stations[m].position[0]=((buffer[msg_start+1]-48)*10+buffer[msg_start+2]-48);
 stations[m].position[1]=((buffer[msg_start+3]-48)*10+buffer[msg_start+4]-48);
 stations[m].position[2]=((buffer[msg_start+6]-48)*10+buffer[msg_start+7]-48);
 stations[m].position[6]=0;
 if (buffer[msg_start+8]=='S'){stations[m].position[6]=0x10;}
 lat2=convert_position(0,m);
 /*
 c1=buffer[msg_start+3]-48;
 c2=buffer[msg_start+4]-48;
 c3=buffer[msg_start+6]-48;
 //l=(buffer[msg_start+3]-48)*1000+(buffer[msg_start+4]-48)*100;	//minuty
 //l=l+(buffer[msg_start+6]-48)*10;//+(buffer[msg_start+7]-48);								//1/10 min
 l=c1*1000+c2*100+c3*10;
 l1=(l)/6;
 l=((buffer[msg_start+1]-48)*10+buffer[msg_start+2]-48);	//stopnie
 lat2=(float)l1/1000.0+(float)l;
 if (buffer[msg_start+8]=='S'){lat2=-lat2;}	
 */
 stations[m].position[3]=((buffer[msg_start+10]-48)*100+(buffer[msg_start+11]-48)*10+buffer[msg_start+12]-48);
 stations[m].position[4]=((buffer[msg_start+13]-48)*10+buffer[msg_start+14]-48);
 stations[m].position[5]=((buffer[msg_start+16]-48)*10+buffer[msg_start+17]-48);
 if (buffer[msg_start+18]=='W'){stations[m].position[6]|=0x01;}
 lon2=convert_position(1,m);
 /*
 c1=buffer[msg_start+13]-48;
 c2=buffer[msg_start+14]-48;
 c3=buffer[msg_start+16]-48;
 //l=(buffer[msg_start+13]-48)*1000+(buffer[msg_start+14]-48)*100;	//minuty
 //l=l+(buffer[msg_start+16]-48)*10;//+(buffer[msg_start+17]-48);							//1/10 min
 l=c1*1000+c2*100+c3*10;
 l1=(l)/6;

 l=((buffer[msg_start+10]-48)*100+(buffer[msg_start+11]-48)*10+buffer[msg_start+12]-48);	//stopnie
 lon2=(float)l1/1000.0+(float)l;
 if (buffer[msg_start+8]=='W'){lon2=-lon2;}
 */
 ii=0;
 for(i=0;i<6;i++){ii=ii+stations[m].position[i];}

 }
 if(ii>0) //eliminuje pozycje 000.00
  dist_calc(m,lat2,lon2,1);

}



char header_match(char *header, char hid)
{
char i,c,match;
char *hh;

 match=0;
 i=0;
 while ((i<MAX_STATIONS)&&(!match))
 {
   if (hid==stations[i].ssid)
   {
     match=1;
	 hh=header;
     for (c=0;c<6;c++)
	  {if((*hh)!=(stations[i].sign[c])){match=0;};
	   hh=hh+1;
	  }
	 if(!match){i=i+1;}
   }
   else
   {
    i=i+1;
   }
 }
 if(!match){i=255;}
 return(i);
}



void decode_status(char n, char msg)
{

 if(buffer[msg+7]=='z'){msg=msg+7;}
 parse_comment(n,msg+1);
 /*
 c1=0;
  while((buffer[c1+msg+1]!=0)&&(c1<(MAX_INFO-1))){
     stations[n].info[c1]=buffer[c1+msg+1];
	 c1=c1+1;
  }
  stations[n].info[c1]=0;
*/
}



void decode_mic_e(char n,char tp,char msg_start)
{
char ms,c1,i,tc;
char ns,loffs,we;
int ii;
float lat2,lon2;
//char latstr[7];
//char ss[5];

#ifdef rsdebug
printf("procedure start: decode_mic_e\n");
#endif

 //decode latitude
 loffs=0;
 tc=0;
 for(i=1;i<7;i++){
    c1=buffer[i];
    //decode digit
    if((c1=='Z')||(c1=='K')||(c1=='L')){c1=0;}
    if(c1>='P'){c1=c1-'P';}
    if(c1>='A'){c1=c1-'A';}
    if(c1>='0'){c1=c1-'0';}
    //latstr[i-1]=c1+'0';
    tc=10*tc+c1;
    if(i==2){stations[n].position[0]=tc;tc=0;}
    if(i==4){stations[n].position[1]=tc;tc=0;}
    if(i==6){stations[n].position[2]=tc;}
    c1=buffer[i];
    if(i==4){ //n/s coding
      if(c1<'P'){ns='S';}else{ns='N';}
    }
    if(i==5){ //offset coding
      if(c1<'P'){loffs=0;}else{loffs=100;}
    }
    if(i==6){ //w/e coding
      if(c1<'P'){we='E';}else{we='W';}
    }

 }//for

 //decode longitude
 c1=buffer[msg_start+1];
 if(loffs==0){
  c1=c1+99-127;
 } else{ //loffs==100
   if(c1>=118){c1=c1+9-127;}
    else
   if(c1>=108){c1=c1+109-117;}
    else
    {c1=c1+179-107;}
 }
 stations[n].position[3]=c1;
 //sprintf(ss,"%003d",c1);
 //for(i=0;i<3;i++){latstr[i]=ss[i];}
 //minutes
 c1=buffer[msg_start+2];
 c1=c1-28;
 if(c1>=60){c1=c1-60;}
 stations[n].position[4]=c1;
 //sprintf(ss,"%02d",c1);
 //for(i=0;i<2;i++){latstr[i+3]=ss[i];}
 //1/100 minutes
 c1=buffer[msg_start+3];
 c1=c1-28;
 stations[n].position[5]=c1;
 //sprintf(ss,"%02d",c1);
 //for(i=0;i<2;i++){latstr[i+5]=ss[i];}
 stations[n].position[6]=0;
 if (ns=='S'){stations[n].position[6]=0x10;}
 if (we=='W'){stations[n].position[6]|=0x01;}
 lat2=convert_position(0,n);
 lon2=convert_position(1,n);
 ii=0;
 for(i=0;i<6;i++){ii=ii+stations[n].position[i];}
 if(ii>0) //eliminuje pozycje 000.00
  dist_calc(n,lat2,lon2,1);

 stations[n].symbol=buffer[msg_start+7];
 c1=msg_start+9;
 if((buffer[c1]=='`')||(buffer[c1]==0x27)||(buffer[c1]==0x1d))
 {	//telemetry
  sprintf(stations[n].info,"telemetry");
 } else
 {
 ms=msg_start+9;
 if((buffer[ms]=='>')||(buffer[ms]==']')){ms=ms+1;}	//kenwood marks
 if(buffer[ms+3]=='}'){ms=ms+4;}//altitude
 if(buffer[ms+6]=='/'){ms=ms+8;}
 parse_comment(n,ms);
 /*
 c1=0;
  while((buffer[c1+ms]!=0)&&(c1<(MAX_INFO-1))){
     stations[n].info[c1]=buffer[c1+ms];
	 c1=c1+1;
  }
  stations[n].info[c1]=0;
 */
 }

}

char it_parse_(char tp,char ms,char limit)
{
  char i,c;

  i=0;
  while((buffer[ms+i]!=limit)&&(buffer[ms+i]!='-')&&(i<6)){
    tcpbuf[tp+i]=buffer[ms+i];
    i++;
  }
  c='0';
  if(buffer[ms+i]=='-'){
    i++;
    if(buffer[ms+i+1]=='>'){c=buffer[ms+i]-'0';}
     else //dwucyfrowy
     {
       c=(buffer[ms+i]-'0')*10+buffer[ms+i+1]-'0';
       i++;
     }
    if(c>9){c=c-10+'A';}else{c=c+'0';}
    i++;
  }
  i++;
  tp=tp+6;
  tcpbuf[tp++]=c;
  return(ms+i);
}


void it_packet(char n,char msg_start)
{
char i,tp,ms;

  if(stations[n].symbol==32){stations[n].symbol='&';}	
  //parse tcpip packet and put into tcpip buf
  if(!tcpipframes){return;}
  tcpflag=1;
  tp=1;
  ms=msg_start+1;
  for(i=0;i<16;i++){tcpbuf[i]=' ';}
  ms=it_parse_(tp+7,ms,'>');
  ms=it_parse_(tp,ms,',');
  tp=tp+14;
  tcpbuf[tp++]='T';
  tcpbuf[tp++]='C';
  tcpbuf[tp++]='P';
  tcpbuf[tp++]='I';
  tcpbuf[tp++]='P';
  tcpbuf[tp++]=' ';
  tcpbuf[tp++]='p';
  tcpbuf[tp++]=0x03;
  tcpbuf[tp++]=0xFF;
  while((ms<230)&&(buffer[ms]!=':')){ms++;}
  if(ms==230) //parse error
  {
    tcpbuf[tp++]='>';
    i=0;
    while(buffer[i]>0){tcpbuf[tp++]=buffer[i+1];i++;}
  }
  else
  {
    ms++;
    while(buffer[ms]>0){tcpbuf[tp++]=buffer[ms++];}
  }

  tcpbuf[tp]=0;


}//it packet

void decode_item(char n,char msg_start)
{
}

void decode_object(char n,char msg_start)
{
char i,tp,ms;

  	
  //parse object packet and put into tcpip buf
  tcpflag=1;
  tp=1;
  ms=msg_start+1;
  tcpbuf[tp++]='O';
  tcpbuf[tp++]='B';
  tcpbuf[tp++]='J';
  tcpbuf[tp++]='E';
  tcpbuf[tp++]='C';
  tcpbuf[tp++]='T';
  tp=7;
  tcpbuf[tp++]='0';
  for(i=0;i<6;i++){tcpbuf[tp++]=buffer[ms+i];}
  tcpbuf[tp++]='0';
  for(i=0;i<6;i++){tcpbuf[tp++]=stations[n].sign[i];}
  tcpbuf[tp++]='p';
  tcpbuf[tp++]=0x03;
  tcpbuf[tp++]=0xFF;
  ms=ms+9;
  if((buffer[ms]=='*')||(buffer[ms]=='_'))

  {
    tcpbuf[tp++]='@';
    ms++;
    while(buffer[ms]>0){tcpbuf[tp++]=buffer[ms++];}
  }
  else
  {//parse error
    tcpflag=0;

  }

  tcpbuf[tp]=0;


}//it packet



//****************************
//char check_digis(char start)
//****************************
//sprawdza pole sciezki
//zwraca 1 jesli stwierdzi repetowanie
//zwraca adres poczatku pola jesli stwierdzi poprawny znak repeatera
char check_digis(char start)
{
char i,j,j1,j2,j3,j4;
char napis[5];
char rp;
#ifdef rsdebug
printf("procedure start: check_digis\n");
#endif
	   rp=0;
	   //printf("CD:%d",start);for(i=0;i<6;i++){putchar(buffer[start+i]);}
	
	   sprintf(napis,"WIDE");
	   j=1;for(i=0;i<4;i++){if(napis[i]!=buffer[start+i]){j=0;};};
	   if(j&&(buffer[start+4]==' ')){
             j=0;
             if((buffer[start+6]&0x40)==0x40){rp=1;}
           }			//sprawdz czy WIDEn

	   if(j&&(buffer[start+4]!=' ')){
		 if((buffer[start+6])!=(buffer[start+4])){rp=1;}
	   }
	   sprintf(napis,"SP2 ");
	   j1=1;for(i=0;i<4;i++){if(napis[i]!=buffer[start+i]){j1=0;};};
	   sprintf(napis,"SP3 ");
	   j2=1;for(i=0;i<4;i++){if(napis[i]!=buffer[start+i]){j2=0;};};
	   sprintf(napis,"SP4 ");
	   j3=1;for(i=0;i<4;i++){if(napis[i]!=buffer[start+i]){j3=0;};};
	   sprintf(napis,"SP5 ");
	   j4=1;for(i=0;i<4;i++){if(napis[i]!=buffer[start+i]){j4=0;};};
	   if(j1+j2+j3+j4){
	     //printf(" %3d  %3d  ",buffer[start+2],buffer[start+6]);
		 if((buffer[start+6])!=(buffer[start+2])){rp=1;}
	   }
	
	
	
	   if(((buffer[start+6]&0x40)==0x40)&&(rp==0)){
	    rp=start;
		}
	   //printf("\n");
	   return(rp);
}

char check_valid(char start)
{
 char c,i,vv;
 vv=0;
 for(i=0;i<6;i++){
   c=buffer[start+i];
   if(!((c==0x20)||((c>=0x30)&&(c<=0x39))||((c>=0x41)&&(c<=0x5A)))){vv=1;}
 }

 return(vv);
}

//*********************************
// analiza sciezki pakietu
// zwraca 0 jesli w direkcie
// zwraca adres poczatku znaku ostatniego repeatera
// w minusie jesli blad
int analyze_path(int digi_start)
{
char c,i,vv;
int rp,rpa;

	rp=0;
	rpa=0;
        vv=0;
	digi_start=digi_start+13;		//omit source&dest
	i=0;
	while((buffer[digi_start+i+1]!=0x03)&&(i<200)){
	 c=check_digis(digi_start+i+1);
	 //printf("=%d",c);
	 if(c==1){rp=1;rpa=0;}
	 if(c>1){rp=1;rpa=c;}
         vv=vv+check_valid(digi_start+i+1);
	 i=i+7;
	}

	if(rpa>0){rp=rpa;}else{rp=-rp;}
        if(vv){rp=255;}
	return(rp);
}


//digimeter ananlizuje wlasny pakiet czy wrocil z przemiennika
//aktualizuje informacje o tym jakie przemienniki mnie powtarzaja
void digimeter(int digi_start)
{
  char i,j,dup;
  char ss[7];

  digi_start+=13;
  if((buffer[digi_start+1]!=0x03)&&((buffer[digi_start+7]&0x40)==0x40)){

    for(i=0;i<7;i++){ss[i]=buffer[digi_start+1+i];}
    ss[6]=ss[6]&0x0F;
    dup=0;
    for(i=0;i<6;i++){
      if((!memcmp(ss,selfdigis[i].alias,7))&&\
         (selfdigis[i].status==1))
      {dup=1;}
    }
    if(!dup){
    for(i=5;i>0;i--){
     selfdigis[i].status=selfdigis[i-1].status;
     selfdigis[i].time=selfdigis[i-1].time;
     for(j=0;j<6;j++){
       selfdigis[i].alias[j]=selfdigis[i-1].alias[j];
     }
    }
    for(i=0;i<7;i++){
    selfdigis[0].alias[i]=buffer[digi_start+i+1];
    }
    selfdigis[0].alias[6]=selfdigis[0].alias[6]&0x0F;
    selfdigis[0].time=timeval;
    selfdigis[0].status=1;
    } //if dup
  }
}




int digi_path(char n, char crc)
{
char c,i,j,aliaspos,aliasvalid,k;
char ss[30],ssa[7];
char digi_start,hops;

#ifdef digidebug
       printf("DIGI ");
#endif
	//sprawdz czy taka sama nie jest na liscie
        i=0;
        //ramka moze byc powtorzona ale po timeoucie albo jeszcze nie powtorzona
        while((i<DIGIQUEUELEN)&&(!((rpque[i].crc==crc)&&(rpque[i].id==n)&&(rpque[i].rpflag!=0)\
            &&((timeval-rpque[i].reptime)<(digidupetime*10))))){i++;}
        if(i<DIGIQUEUELEN){return(0);}//znaleziona taka sama, powrot

	digi_start=21;		//omit source&dest
        aliaspos=0;
	i=0;hops=0;
        while( (buffer[digi_start+i-6]!=0x03)  && (i<200) ){
          if((buffer[digi_start+i]&0x40)==0x40){aliaspos=0;hops++;}
           else
           {
             if(aliaspos==0){aliaspos=digi_start+i-6;}
           }
	  i=i+7;
	}
	//if((buffer[digi_start+i-6]!=0x03)&&(i<200))
        if(aliaspos>0)
        {//so we have found a free alias
#ifdef digidebug
       printf("Free alias ");
       for(k=0;k<6;k++){putchar(stations[n].sign[k]);}
       printf("\n");
       printf("Hops:%d\n",hops);
#endif
          c=aliaspos;

          //check if we repeat this alias
          for(i=0;i<6;i++){ss[i]=buffer[c+i];}
          aliasvalid=255;

          for(i=0;i<4;i++){
           if(aliases[i].enable){
            //copy alias with "*" conversion
            for(j=0;j<6;j++){
              if(aliases[i].alias[j]!='*'){ssa[j]=aliases[i].alias[j];}else{ssa[j]=ss[j];}
            }
            if(!memcmp(ss,ssa,6)){aliasvalid=i;}
           }
          }//for
          /*
          i=0;
          while((i<4)&&((!aliases[i].enable)||(memcmp(ss,aliases[i].alias,6)))){i++;}
          */
          if((aliasvalid!=255)&&(hops<aliases[aliasvalid].hoplimit)){


            //znajdz wolny slot
           j=0;
           while((!(((timeval-rpque[j].reptime)>(digidupetime*10))||(rpque[j].rpflag==0)))&&(j<DIGIQUEUELEN)){j++;}
           if(j<DIGIQUEUELEN){
#ifdef digidebug
       for(k=0;k<6;k++){putchar(stations[n].sign[k]);}
       sprintf(ss," %d %d %d",n,j,stations[n].slot);
       printf(ss);
       printf("\n");
#endif

           rpque[j].slot=stations[n].slot;
           rpque[j].reptime=timeval+digidelay*10;
           rpque[j].alias=aliasvalid;
           rpque[j].aliaspos=c;
           rpque[j].crc=crc;
           rpque[j].rpflag=1;
           rpque[j].id=n;
           } //else - brak wolnego slotu - nie powtarzamy
#ifdef digidebug
           else{
           printf("QUEUE FULL");
       for(k=0;k<6;k++){putchar(stations[n].sign[k]);}
       printf("\n");
           }
#endif
             }
        }
	return(0);
}







void remove_station(char n)
{
  char i;
  //sprawdz liste wiadomosci i usun odwolania do tej stacji
  for(i=0;i<MAX_MESSAGES;i++){
    if(messages[i].stid==n){
      messages[i].status=0;
      messages[i].ackstatus=0;
    }
  }
}








void update_aprs(char msg_start)
{
//char znak[7];
char i,n,rpe,crc,ob;
char tp,eoh,st,uimark;
int rp;
unsigned long tt;
char ss[8];
//printf("procedure start: update aprs\n");
#ifdef rsdebug
    printf("procedure start: update aprs\n");
    for(i=0;i<7;i++){putchar(buffer[msg_start+7+i]);}
      printf("\n");
    i=0;while((i<210)&(buffer[i]>0)){putchar(buffer[i]);i++;}
      printf("@\n");
      printf("(%x)",buffer[i-2]);
      printf("(%x)\n",buffer[i-1]);

#endif
//station copy;
      /*
  if(buffer[msg_start]!=0xAA)	
  //nie dekoduj ramek ip bo juz zdekodowane w poprzednim przebiegu
  {
   rp=0;//header_decode(100,&buffer[0]);
  }
  else
  {
   rp=254;
  }*/

  //czy to jest object?

  rp=analyze_path(msg_start);

  sprintf(ss," OBJECT");
  buffer[0]=' ';
  if(memcmp(ss,buffer,6)==0){ob=2;}else{ob=0;}


  rpe=0;
  if(rp<0){
    rp=-rp;
	rpe=1;
  }
  //filtruj bledne ramki z przypadkowych znakow
  //z=buffer[msg_start];
  n=1;
  buffer[249]=0;  //na wypadek kaszki
  //sprawdz czy to nie ramka wlasna
  //z=header_match(&buffer[msg_start],(buffer[msg_start+13]&0x0F));
  //if(z){PORTB|=0x20;}							 //beep on self frame
  //if(z&(!allowself)){n=0;}
  //czy chcemy odbierac repeatery niosace dane z internetu
  //if ((buffer[msg_start]=='}')&&(!rplayer)){n=0;}

  //jesli poprawne dwa pierwsze znaki A-Z 0-9
  //oraz ramka w direkcie lub zezwalam na repeatowane

#ifdef debug
	  	printf("processing\n");
#endif
  st=header_match(&buffer[msg_start+7],(buffer[msg_start+13]&0x0F));

  //search for end of header eoh
  eoh=msg_start;
  while((buffer[eoh]!=0x03)&&(eoh<200)){eoh++;}
  eoh=eoh+2;

  //calculate crc
  crc=0;
  i=eoh;
  while(buffer[i]>0){
   crc+=buffer[i++];
  }

  uimark=0;
  if(buffer[i-1]==0x0d){uimark=1;}

  //fill with '0'
  while(i<249){buffer[i++]=0;}

  if(serial0_opt==1) //packet monitor
  {
    i=eoh;while((i<240)&&(buffer[i]>0)){i++;}
    if(rp==255){printf("INVALID! ");}
    printf("%6d (LEN:%3d EOH:%3d CRC:%3d) ",timeval/10,i,eoh,crc);
    //header
    i=1;while((i<240)&&(buffer[i]!=0x03)){
      if((buffer[i]<32)||(buffer[i]>126)){putchar('.');}
      else{putchar(buffer[i]);}
      i++;
    }
    //body
    //header
    while((i<240)&&(buffer[i]>0)){
      if((buffer[i]<32)||(buffer[i]>126)){putchar('.');}
      else{putchar(buffer[i]);}
      i++;
    }
      printf("\n");
  }

  if((serial0_opt==5)&&\
     (marked_station>0)&&(marked_station==st))
  {//marked packet monitor
    i=eoh;while((i<240)&&(buffer[i]>0)){i++;}
    if(rp==255){printf("INVALID! ");}
    printf("%6d (LEN:%3d EOH:%3d CRC:%3d) ",timeval/10,i,eoh,crc);
    //header
    i=1;while((i<240)&&(buffer[i]!=0x03)){
      if((buffer[i]<32)||(buffer[i]>126)){putchar('.');}
      else{putchar(buffer[i]);}
      i++;
    }
    //body
    //header
    while((i<240)&&(buffer[i]>0)){
      if((buffer[i]<32)||(buffer[i]>126)){putchar('.');}
      else{putchar(buffer[i]);}
      i++;
    }
      printf("\n");
  }


  if((rp==255)&&(validonly)){invalid_count++;return;}


  uniquemark=0;

  if(st<255) //i=addres in hash
  {
#ifdef rsdebug
    for(n=0;n<6;n++){putchar(stations[st].sign[n]);}

	  	printf(" update\n");
#endif
    //is my frame?
    if(st==0){
      if(beacon_delay-(next_beacon_time-timeval)<300) //30s
      {
        self_digipeated=1;
      }
      digimeter(msg_start);
      digisens_time=999000000;
    }
   //***** update entry

   //verify unique


  if((crc!=stations[st].framecrc)||((timeval-stations[st].minutes)>300))
  {//so we have a different frame or very delayed repeated >30sec
    uniquemark=1;
    stations[st].framecrc=crc;
  }
  n=st;
  if(uniquemark||allframes){
   if(stations[n].count<999){stations[n].count=stations[n].count+1;}
   stations[n].frame=buffer[msg_start];
   tp=0;
   if(rp>0){
     tp=1;
	 for(i=0;i<6;i++){stations[n].path[i]=buffer[msg_start+rp-1+i];}
	
	 }
   //jesli w ciagu 30s przchodzi direct i przez digi to...
   if(((timeval-stations[n].minutes)<300)&&\
       ((stations[n].type==0)||(stations[n].type==2))&&(tp==1)){tp=2;}
   stations[n].type=tp;
   stations[n].pathvalid=1;
   if((!rp)||rpe){stations[n].pathvalid=0;}
   stations[n].frame=buffer[eoh];
   stations[n].minutes=timeval;
   stations[n].opt=uimark;
   stations[n].opt|=ob;
  } //if unique
   //********************

  }
  else
  {
#ifdef rsdebug
	  	printf("new entry ");
#endif
  //******** new entry
  uniquemark=1;
  if (num_stations<MAX_STATIONS-1)
  {
   num_stations=num_stations+1;
   n=num_stations;
//HACKERS CONTROL
   if(!hackcheck){n=1;}
   //if(num_stations>10){n=1;}
//HACKERS CONTROL
  }
  else
  {//replace oldest
   tt=timeval;
   for(i=1;i<MAX_STATIONS;i++){
	 if(stations[i].minutes<tt){tt=stations[i].minutes;n=i;}
   }
   remove_station(n);
  }

    for (i=0;i<6;i++){stations[n].sign[i]=buffer[msg_start+7+i];}
    stations[n].ssid=buffer[msg_start+13]&0x0F;

#ifdef rsdebug
  for(i=0;i<6;i++){putchar(stations[n].sign[i]);}
  printf("\n");
#endif

  stations[n].count=1;
  stations[n].dist=999999;
  stations[n].type=0;
  if(rp>0){
   stations[n].type=1;
   for(i=0;i<6;i++){stations[n].path[i]=buffer[msg_start+rp-1+i];}
   };
  stations[n].pathvalid=1;
   if(!rp||(rpe)){stations[n].pathvalid=0;}
  stations[n].symbol=32;
  stations[n].info[0]=0;
  stations[n].moving=0;
  stations[n].framecrc=crc;
  stations[n].distchg=0;
  stations[n].minutes=timeval;
  stations[n].frame=buffer[eoh];
  stations[n].wxtemp=NOTEMP;
  stations[n].opt|=uimark;
  stations[n].opt|=ob;
  //**********************
  st=n;
  }





  //stations[n].timeh=0;
  //stations[n].timem=0;
  msg_start=eoh;



#ifdef spidebug
  if(spirequest>0){
    for(i=0;i<6;i++){putchar(stations[lastreceived_num].sign[i]);}
    printf(" buffer error\n");
  }
#endif


  if(uniquemark||allframes){

    buffer[0]=stations[n].frame;
    buffer[250]=(timeval/10)&0x000000FF;
    buffer[251]=crc;
    //zmiana testowa 3.06.2006 00:31
    //if(n>0){
      stations[n].slot=get_freepage();         //save to exteral flash
     spirequest=1;
     spi_buffer_to_flash_rq=stations[n].slot;
     //flash_write(buffer,stations[n].slot,255);
    //}
    lastreceived_num=n;
  }

  traffic[0]++;
  packet_count++;





  if(uniquemark&&(n>0))
  {//so we have a different frame or very delayed repeated >30sec
   utraffic[0]++;
   stations[n].minutes=timeval;
   unique_packet_count++;



   switch (stations[n].frame)
   {
   case ':' :add_message(n,msg_start);break;
   case '=' :;
   case '/' :;
   case '@' :decode_position(n,msg_start);break;
   case '!' :if(buffer[msg_start+1]=='!'){/*WX*/}
              else {decode_position(n,msg_start);}
             break;
   case '`' :decode_mic_e(n,tp,msg_start);break;
   case 0x27 :decode_mic_e(n,tp,msg_start);break;
   case '>' :decode_status(n,msg_start);break;
   case '}' :it_packet(n,msg_start);break;
   case '#' :;
   case '*' :;
   case '_' :decode_wx(n,msg_start);break;//wx report
   case '?' :decode_query(n,msg_start);break;
   case ')' :decode_item(n,msg_start);break;
   case ';' :decode_object(n,msg_start);break;

   default  : //? "!"
     i=0;rp=0;while(i<40){if(buffer[msg_start+i]=='!')rp=1;i++;}
     if(rp){
      stations[n].frame='x';
     }



   }

   if(n>0){digi_path(n,crc);} //blokuj wlasne ramki

   //popraw liczniki dla tcpip
   if(tcpipframes&&(stations[n].frame=='}'))
   {
     if(uniquemark){
      unique_packet_count--;
      utraffic[0]--;
     }
     traffic[0]--;
     packet_count--;
   }

/*
  //zdalne sterowanie
  sprintf(txbuf,"ENABL2");
  tp=1;
  for(i=0;i<6;i++){if(txbuf[i]!=buffer[8+i]){tp=0;};}
  if(tp){raxkiller=1;}

  sprintf(txbuf,"DISABL");
  tp=1;
  for(i=0;i<6;i++){if(txbuf[i]!=buffer[8+i]){tp=0;};}
  if(tp){raxkiller=0;}

  sprintf(txbuf,"ENABL3");
  tp=1;
  for(i=0;i<6;i++){if(txbuf[i]!=buffer[8+i]){tp=0;};}
  if(tp){__disable_interrupt();while(1);}

  //-zdalne sterowanie
*/

  } //if unique


  /*
  else
  { //sprawdz liste digi
    for(i=0;i<DIGIQUEUELEN;i++)
    {
     if(rpque[i].id==n) //uproszczenie, a co jak stacja przysle inna ramke?
     {
       rpque[i].reptime=NEVER;
     }
    }
  }
*/
  setsound(soundtab[SND_STATION]);
  if(uniquemark||allframes)
  {

  if(n==0){setsound(soundtab[SND_SELF]);}
    //else{setsound(soundtab[SND_MARKED]);}
  if((n==marked_station)&&(n>0)){setsound(soundtab[SND_MARKED]);}
  }




#ifdef rsdebug
	  	printf("update aprs end %d\n\n",packet_count);
#endif
}
