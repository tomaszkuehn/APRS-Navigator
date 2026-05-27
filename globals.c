
#include "includes.h"

#define FLASH_CFG_VERSION  1
#define FLASH_CFG_VOFF     252
#define FLASH_CFG_CKSUM    253

static char flash_cfg_checksum(char *data, char len)
{
  char i, sum = 0;
  for (i = 0; i < len; i++) { sum ^= data[i]; }
  return sum;
}

static char flash_cfg_validate(char *data, char len)
{
  return data[FLASH_CFG_VOFF] == FLASH_CFG_VERSION
      && data[FLASH_CFG_CKSUM] == flash_cfg_checksum(data, FLASH_CFG_VOFF);
}

static void flash_cfg_finalize(char *data, char len)
{
  data[FLASH_CFG_VOFF] = FLASH_CFG_VERSION;
  data[FLASH_CFG_CKSUM] = flash_cfg_checksum(data, FLASH_CFG_VOFF);
}

/* this variable indicates change in station list */
char receive_status;

/* #column to start display */
char station_list_col;

/* #line to place cursor */
char station_list_cur;
int station_list_start;
char station_list_sort;
unsigned int slot_lock;
char update_status;
char gpslock;

char lastreceived_num;
unsigned int packet_count;
unsigned int unique_packet_count;
unsigned int message_count;
unsigned int my_beacon_count;
unsigned int invalid_count;

char gpsenable;

int radar_range;
char filter_opt_include;
char filter_opt_exclude;
char filtered_stations;
char announce_filter;
char uniquemark;
char gpsfix;
char gpsdata;
char gpspresent;
char self_digipeated;

char repeater_enable;
char txenable;
char queryenable;

char hackcheck;

char system_userlevel;

unsigned char trx_txdelay;
unsigned char trx_txheader;
unsigned char trx_rxtune;
unsigned char trx_bitdelay;
unsigned char trx_squelch;

unsigned char msg_retries;
unsigned char msg_delay;
_msgpredef msgpredef[5];

char beacon_auto;
char beacon_digi;

unsigned int sounddelay;
unsigned char soundmod;
unsigned char soundmodvar;

char     posbank1[7];
char     posbank2[7];
char     posbank3[7];
char posuse;

char soundtab[7];

char traffic[60];
char utraffic[60];
char dtraffic[60];
int  htraffic[120];

char      rpbuf[255];
rpque_    rpque[DIGIQUEUELEN];
_txque txqueue[TXQUEUELEN];
char msbuf[255];

char tcpbuf[255];
char tcpflag;
char allmessages;

char serial0_opt;

_aliases aliases[4];
char digidupetime;
unsigned int digidelay;

_path paths[3];
char udest[6];
char pathuse;
char msgpathuse;

_status statusy[3];
char statuse;

char frame_injection;

char allframes;
char tcpipframes;
char validonly;
char navmark;
unsigned int msgid;
char admin_code;

char debuglevel;

_rkey    rkey[4];

/*
typedef struct {
   	   char sign[6];
	   char ssid;								// np. -1
	   long  dist;
	   int namiar;
	   char frame;							//typ ramki	
	   char symbol;							//symbol stacji
	   char count;							//pckts received
	   char type;							//direct=0, digi=1, it=254
	   //char slot;							//lokalizacja ramki w eeprom
	   char minutes;						//ile minelo od odebrania
} station_;
*/
station_ stations[MAX_STATIONS];
char    sort[MAX_STATIONS];
char 	num_stations;
char 	buffer[264];
char    napis[264];
char    txbuf[255];
char    msbuf[255];
char    frame[255];

_recmessage messages[MAX_MESSAGES];
_txmessage  txmessages[TX_MESSAGEBUF];
_selfdigi selfdigis[6];
_selfdigi memdigi[6];

char menup1;
char menup2;
char raxkiller;
char cstation;
char marked_station;
int rranges[5];
char currange;
char position_mode;
unsigned int spirequest;
unsigned int spi_buffer_to_flash_rq;
unsigned int spi_flash_to_rpbuf_rq;
unsigned int spi_txbuf_to_flash_rq;
unsigned int spi_flash_to_msbuf_rq;



unsigned long next_beacon_time;
unsigned long beacon_delay;
unsigned long digisens_time;

float mylat,mylon;
int mycourse;
unsigned char 	hangup;

#define SYMCNT 6
const char symnames[]={"House     Car       Kenwood   Digi      Horse     Shuttle   "};

const char symtable[]={
//
'/','-',0x0D,
'/','>',0x0F,
'\\','K',0x0E,
'\\','#',0,
'/','e',0,
'/','S',0x10
};

char errtable[10];
char ser_kbden;


int _getopt(int opt)
{
  return(1);
}

void setopt(int opt)
{

}

void restore_factory()
{
char i;


  stations[0].position[0]=52;
  stations[0].position[1]=25;
  stations[0].position[2]=81;
  stations[0].position[3]=16;
  stations[0].position[4]=57;
  stations[0].position[5]=31;
  stations[0].position[6]=0;


  stations[0].ssid=0;
  stations[0].symbol=0;
  digidupetime=30;
  radar_range=rranges[0];
  filter_opt_include=0xFF;    //no filtering
  filter_opt_exclude=0x00;
  announce_filter=0;
  position_mode=0x00;
   //trx tunable params
  trx_txdelay=5;
  trx_txheader=35;
  trx_rxtune=75;
  trx_bitdelay=187;
  trx_squelch=0;
  allframes=0;
  tcpipframes=1;
  allmessages=1;
  navmark=1;
  msg_retries=4;
  msg_delay=35;
  beacon_auto=0;
  beacon_digi=0;
  beacon_delay=99000000;

  sprintf(udest,"APZ01 ");
  for(i=0; i<4;i++){aliases[i].enable=0;aliases[i].hoplimit=3;}
  sprintf(aliases[0].alias,"RELAY ");
  sprintf(aliases[1].alias,"WIDE  ");
  sprintf(aliases[2].alias,"WIDE* ");
  sprintf(aliases[3].alias,"SP*   ");
  digidelay=0;

  sprintf(paths[0].spath,"RELAY 0WIDE  0");paths[0].plen=2; paths[0].enable=1;
  paths[0].msgenable=1;
  sprintf(paths[1].spath,"WIDE1 1WIDE2 2");paths[1].plen=2; paths[1].enable=0;
  paths[1].msgenable=0;
  sprintf(paths[2].spath,"RELAY 0SP3   3");paths[2].plen=2; paths[2].enable=0;
  paths[2].msgenable=0;

  sprintf(statusy[0].text,"APRS Navigator");statusy[0].enable=1;
  sprintf(statusy[1].text,"www.aprs-navigator.com");statusy[1].enable=0;
  sprintf(statusy[2].text,"Hello world");statusy[2].enable=0;
  posuse=0;

  sprintf(msgpredef[0].text,"Witam!");
  sprintf(msgpredef[1].text,"Jestem w drodze, odezwe sie pozniej!");
  sprintf(msgpredef[2].text,"Co slychac?");
  sprintf(msgpredef[3].text,"Pozdrawiam");
  sprintf(msgpredef[4].text,"Do widzenia 73!");

  rranges[0]=6;
  rranges[1]=30;
  rranges[2]=90;
  rranges[3]=300;
  rranges[4]=900;

  for(i=0;i<7;i++){
  posbank1[i]=stations[0].position[i];
  posbank2[i]=stations[0].position[i];
  posbank3[i]=stations[0].position[i];
  }
  posbank3[2]=0;

  repeater_enable=1;
  txenable=1;
  queryenable=1;
  for(i=0;i<7;i++){soundtab[i]=0;}

}






void globals_initialize(void)
{
  char i,c,j;
  char ss[4];

  //zmienne czytane z konfiguracji
  stations[0].sign[0]='U';
  stations[0].sign[1]='N';
  stations[0].sign[2]='L';
  stations[0].sign[3]='I';
  stations[0].sign[4]='S';
  stations[0].sign[5]=' ';



  //--test only
  stations[0].position[0]=52;
  stations[0].position[1]=25;
  stations[0].position[2]=81;
  stations[0].position[3]=16;
  stations[0].position[4]=57;
  stations[0].position[5]=31;
  stations[0].position[6]=0;
  //--//

  stations[0].ssid=0;
  stations[0].symbol=0;
  digidupetime=30;
  radar_range=rranges[0];
  filter_opt_include=0xFF;    //no filtering
  filter_opt_exclude=0x00;
  announce_filter=0;
  position_mode=0x00;
   //trx tunable params
  trx_txdelay=5;
  trx_txheader=35;
  trx_rxtune=75;
  trx_bitdelay=187;
  trx_squelch=0;
  allframes=0;
  tcpipframes=1;
  allmessages=1;
  navmark=1;
  msg_retries=4;
  msg_delay=35;
  beacon_auto=0;
  beacon_digi=0;
  beacon_delay=99000000;
/*
  sprintf(udest,"APZ01 ");
  for(i=0; i<4;i++){aliases[i].enable=0;aliases[i].hoplimit=3;}
  sprintf(aliases[0].alias,"RELAY ");
  sprintf(aliases[1].alias,"WIDE  ");
  sprintf(aliases[2].alias,"WIDE* ");
  sprintf(aliases[3].alias,"SP*   ");
  */
  digidelay=0;
/*
  sprintf(paths[0].spath,"RELAY 0WIDE  0");paths[0].plen=2; paths[0].enable=1;
  paths[0].msgenable=1;
  sprintf(paths[1].spath,"WIDE1 1WIDE2 2");paths[1].plen=2; paths[1].enable=0;
  paths[1].msgenable=0;
  sprintf(paths[2].spath,"RELAY 0WIDE  0SP3   3");paths[2].plen=3; paths[2].enable=0;
  paths[2].msgenable=0;

  sprintf(statusy[0].text,"APRS Navigator");statusy[0].enable=1;
  sprintf(statusy[1].text,"www.aprs-navigator.com");statusy[1].enable=0;
  sprintf(statusy[2].text,"Hello world");statusy[2].enable=0;

  posuse=0;
*/
  rranges[0]=6;
  rranges[1]=30;
  rranges[2]=90;
  rranges[3]=300;
  rranges[4]=900;
/*
  //test only
  //w rzeczywistosci odczyt z pamieci
  for(i=0;i<7;i++){
  posbank1[i]=stations[0].position[i];
  posbank2[i]=stations[0].position[i];
  posbank3[i]=stations[0].position[i];
  }
  posbank3[2]=0;

  repeater_enable=1;
  txenable=1;
  queryenable=1;
*/

  //reads from flash
  flash_read(napis,2002,254);
  if (!flash_cfg_validate(napis, 254))
    debugmsg(WARNLEVEL, "config page 2002 invalid or unversioned");
  //restore record1
  digidupetime=napis[0];
  announce_filter=napis[1];
  position_mode=napis[2];
  trx_txdelay=napis[3];
  trx_txheader=napis[4];
  trx_rxtune=napis[5];
  trx_bitdelay=napis[6];
  allframes=napis[7];
  tcpipframes=napis[8];
  allmessages=napis[9];
  navmark=napis[10];
  msg_retries=napis[11];
  msg_delay=napis[12];
  beacon_auto=napis[13];
  beacon_digi=napis[14];
  posuse=napis[15];
  repeater_enable=napis[16];
  txenable=napis[17];
  queryenable=napis[18];
  stations[0].ssid=napis[19];
  stations[0].symbol=napis[20];
  for(i=0;i<7;i++){
    soundtab[i]=napis[21+i];
  }


  trx_squelch=napis[29];
  for(i=0;i<7;i++){
    posbank1[i]=napis[30+i];
    posbank2[i]=napis[40+i];
    posbank3[i]=napis[50+i];
  }
  for(j=0;j<4;j++)
  {
    for(i=0;i<6;i++){
     aliases[j].alias[i]=napis[10*j+60+i];
    }
    aliases[j].enable=napis[10*j+60+6];
    aliases[j].aliastype=napis[10*j+60+7];
    aliases[j].hoplimit=napis[10*j+60+8];
  }
  for(i=0;i<6;i++){
    udest[i]=napis[100+i];
  }


  for(i=0;i<4;i++){ss[i]=napis[110+i];}
  memcpy(&beacon_delay,ss,4);
  //
  flash_read(napis,2003,254);
  if (!flash_cfg_validate(napis, 254))
    debugmsg(WARNLEVEL, "config page 2003 invalid or unversioned");
  for(i=0;i<3;i++){
    for(c=0;c<35;c++){paths[i].spath[c]=napis[35*i+c];}
  }
  paths[0].plen=napis[106];
  paths[0].enable=napis[107];
  paths[0].msgenable=napis[108];
  paths[1].plen=napis[109];
  paths[1].enable=napis[110];
  paths[1].msgenable=napis[111];
  paths[2].plen=napis[112];
  paths[2].enable=napis[113];
  paths[2].msgenable=napis[114];

  for(j=0;j<3;j++)
  {
   for(i=0;i<44;i++){
    statusy[j].text[i]=napis[116+45*j+i];
   }
   statusy[j].enable=napis[116+45*j+44];
  }

  flash_read(napis,2004,254);
  if (!flash_cfg_validate(napis, 254))
    debugmsg(WARNLEVEL, "config page 2004 invalid or unversioned");
  for(j=0;j<5;j++){
    for(i=0;i<50;i++){
      msgpredef[j].text[i]=napis[50*j+i];
    }
  }

  //zmienne inicjalizowane

  hackcheck=1;
  frame_injection=0;
  slot_lock=0;
  radar_range=rranges[0];
  stations[0].count=0;
  stations[0].dist=0;
  stations[0].type=0;
  stations[0].pathvalid=1;

  stations[0].info[0]=0;
  stations[0].moving=0;
  stations[0].framecrc=0;
  stations[0].distchg=0;

  stations[0].slot=0;
  mylat=convert_position(0,0);
  mylon=convert_position(1,0);
  gpslock=0;
  gpsdata=0;
  num_stations=0;
  filtered_stations=0;
  timeval=0;
  spirequest=0;
  spi_buffer_to_flash_rq=0;
  spi_flash_to_rpbuf_rq=0;
  spi_txbuf_to_flash_rq=0;
  spi_flash_to_msbuf_rq=0;

  receive_status=0;
  station_list_col=1;
  station_list_cur=1;
  station_list_start=1;
  station_list_sort=0;
  update_status=0;
  packet_count=0;
  my_beacon_count=0;
  unique_packet_count=0;
  menup1=1;
  menup2=1;
  mycourse=0;
  marked_station=0;
  message_count=0;
  lbufflag=0;
  currange=0;
  gpsfix=0;
  gpsenable=1;
  self_digipeated=0;
  pathuse=0;
  msgpathuse=0;
  for(i=0;i<60;i++){traffic[i]=0;utraffic[i]=0;dtraffic[i]=0;}
  for(i=0;i<120;i++){htraffic[i]=0;}
  for(i=0;i<MAX_MESSAGES;i++){messages[i].status=0;}
  for(i=0;i<TX_MESSAGEBUF;i++){txmessages[i].status=0;}
  statuse=0;
  gpspresent=0;
  tcpflag=0;
  serial0_opt=0;
  validonly=1;
  invalid_count=0;
  for(i=0;i<TXQUEUELEN;i++){txqueue[i].status=0;}
  debuglevel=0;
  msgid=33;
  digisens_time=999000000;
  admin_code=0;
  ser_kbden=0;
  for(i=0;i<250;i++){frame[i]=0;}
  for(i=0;i<6;i++){selfdigis[i].status=0;}
  for(i=0;i<6;i++){memdigi[i].status=0;}
  //wpisz pozycje statyczna na starcie trakera lub moze byc
  //odczytana z pamieci ostatnia pozycja (ale jak zapamietac?)
  switch(posuse){
  //case 1:break;//no change
        case 0:for(c=0;c<7;c++){stations[0].position[c]=posbank1[c];}break;
        case 1:for(c=0;c<7;c++){stations[0].position[c]=posbank2[c];}break;
        case 2:for(c=0;c<7;c++){stations[0].position[c]=posbank3[c];}break;
   };

  for(i=0;i<10;i++){errtable[i]=0;}

  for(i=0;i<4;i++){rkey[i].enable=0;}


  //do sprawdzenia
  next_beacon_time=timeval+beacon_delay;

  raxkiller=0;

  hangup=0;
  sounddelay=0;
  system_userlevel=10;//lowest


  for(i=0;i<DIGIQUEUELEN;i++){rpque[i].rpflag=0;}

}

void update_aprs_config()
{
  char ss[2];

  aprs_exclusive_flag=1;//uP cache workaround
  aprs_exclusive_flag=1;
  aprs_exclusive_flag=1;

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

  aprs_exclusive_flag=0;
}

void update_config()
{
char chg=0;
char i,j,c;
char ss[4];

  if(system_userlevel>3){ //demo
    // __disable_interrupt();
  }
  if(system_userlevel>2){ //demo&800demo
    navmark=1;
  }
  //sprawdz co sie zmienilo i zapisz zmiany w pamieci
  flash_read(napis,2002,254);
  chg=0;
  //create record1
  if(napis[0]!=digidupetime){napis[0]=digidupetime;chg=1;}
  if(napis[1]!=announce_filter){napis[1]=announce_filter;chg=1;}
  if(napis[2]!=position_mode){napis[2]=position_mode;chg=1;}
  if(napis[3]!=trx_txdelay){napis[3]=trx_txdelay;chg=1;}
  if(napis[4]!=trx_txheader){napis[4]=trx_txheader;chg=1;}
  if(napis[5]!=trx_rxtune){napis[5]=trx_rxtune;chg=1;}
  if(napis[6]!=trx_bitdelay){napis[6]=trx_bitdelay;chg=1;}
  if(napis[7]!=allframes){napis[7]=allframes;chg=1;}
  if(napis[8]!=tcpipframes){napis[8]=tcpipframes;chg=1;}
  if(napis[9]!=allmessages){napis[9]=allmessages;chg=1;}
  if(napis[10]!=navmark){napis[10]=navmark;chg=1;}
  if(napis[11]!=msg_retries){napis[11]=msg_retries;chg=1;}
  if(napis[12]!=msg_delay){napis[12]=msg_delay;chg=1;}
  if(napis[13]!=beacon_auto){napis[13]=beacon_auto;chg=1;}
  if(napis[14]!=beacon_digi){napis[14]=beacon_digi;chg=1;}
  if(napis[15]!=posuse){napis[15]=posuse;chg=1;}
  if(napis[16]!=repeater_enable){napis[16]=repeater_enable;chg=1;}
  if(napis[17]!=txenable){napis[17]=txenable;chg=1;}
  if(napis[18]!=queryenable){napis[18]=queryenable;chg=1;}
  if(napis[19]!=stations[0].ssid){napis[19]=stations[0].ssid;chg=1;}
  if(napis[20]!=stations[0].symbol){napis[20]=stations[0].symbol;chg=1;}
  for(i=0;i<7;i++){
    if(napis[21+i]!=soundtab[i]){napis[21+i]=soundtab[i];chg=1;}
  }
  if(napis[29]!=trx_squelch){napis[29]=trx_squelch;chg=1;}
  for(i=0;i<7;i++){
    if(posbank1[i]!=napis[30+i]){napis[30+i]=posbank1[i];chg=1;}
    if(posbank2[i]!=napis[40+i]){napis[40+i]=posbank2[i];chg=1;}
    if(posbank3[i]!=napis[50+i]){napis[50+i]=posbank3[i];chg=1;}
  }
  for(j=0;j<4;j++)
  {
    for(i=0;i<6;i++){
      if(aliases[j].alias[i]!=napis[10*j+60+i]){napis[10*j+60+i]=aliases[j].alias[i];chg=1;}
    }
    if(aliases[j].enable!=napis[10*j+60+6]){napis[10*j+60+6]=aliases[j].enable;chg=1;}
    if(aliases[j].aliastype!=napis[10*j+60+7]){napis[10*j+60+7]=aliases[j].aliastype;chg=1;}
    if(aliases[j].hoplimit!=napis[10*j+60+8]){napis[10*j+60+8]=aliases[j].hoplimit;chg=1;}
  }
  for(i=0;i<6;i++){
    if(udest[i]!=napis[100+i]){napis[100+i]=udest[i];chg=1;}
  }

  memcpy(ss,&beacon_delay,4);
  for(i=0;i<4;i++){
    if(ss[i]!=napis[110+i]){napis[110+i]=ss[i];chg=1;}
  }
  //if(napis[21]!=){napis[17]=;chg=1;}
  if(chg){flash_cfg_finalize(napis, 254); flash_write(napis,2002,254);}
  //if(chg){printf("write 1\n");}

  flash_read(napis,2003,254);
  chg=0;

  for(i=0;i<3;i++){
    for(c=0;c<35;c++){
      if(paths[i].spath[c]!=napis[35*i+c]){napis[35*i+c]=paths[i].spath[c];chg=1;}
    }
  }
  if(paths[0].plen!=napis[106]){napis[106]=paths[0].plen;chg=1;}
  if(paths[0].enable!=napis[107]){napis[107]=paths[0].enable;chg=1;}
  if(paths[0].msgenable!=napis[108]){napis[108]=paths[0].msgenable;chg=1;}
  if(paths[1].plen!=napis[109]){napis[109]=paths[1].plen;chg=1;}
  if(paths[1].enable!=napis[110]){napis[110]=paths[1].enable;chg=1;}
  if(paths[1].msgenable!=napis[111]){napis[111]=paths[1].msgenable;chg=1;}
  if(paths[2].plen!=napis[112]){napis[112]=paths[2].plen;chg=1;}
  if(paths[2].enable!=napis[113]){napis[113]=paths[2].enable;chg=1;}
  if(paths[2].msgenable!=napis[114]){napis[114]=paths[2].msgenable;chg=1;}

  for(j=0;j<3;j++)
  {
   for(i=0;i<44;i++){
     if(statusy[j].text[i]!=napis[116+45*j+i]){napis[116+45*j+i]=statusy[j].text[i];chg=1;}
   }
   if(statusy[j].enable!=napis[116+45*j+44]){napis[116+45*j+44]=statusy[j].enable;chg=1;}
  }
  if(chg){flash_cfg_finalize(napis, 254); flash_write(napis,2003,254);}
  //if(chg){printf("write 2\n");}

  flash_read(napis,2004,254);
  for(j=0;j<5;j++){
    for(i=0;i<50;i++){
      if(msgpredef[j].text[i]!=napis[50*j+i]){napis[50*j+i]=msgpredef[j].text[i];chg=1;};
    }
  }
  if(chg){flash_cfg_finalize(napis, 254); flash_write(napis,2004,254);};
//printf("update config...done\n");
} //update config

void recalc_positions()
{
char i,mm;
float lat2,lon2;
long distm;
char recalc;

 mylat=convert_position(0,0);
 mylon=convert_position(1,0);

 mm=aprs_data_flag;
 aprs_data_flag=1;
 mm=mm;                         //pipeline ?
 recalc=1;
  for(i=1;i<=num_stations;i++)
  {
    if(stations[i].dist<999999){
     distm=stations[i].dist;
     lat2=convert_position(0,i);
     lon2=convert_position(1,i);
     dist_calc(i,lat2,lon2,recalc);
     recalc=0;
     if(stations[i].dist<distm){stations[i].distchg=1;} //zblizamy sie
     if(stations[i].dist>distm){stations[i].distchg=2;} //oddalamy sie
     if(stations[i].dist==distm){stations[i].distchg=0;}
    }
  }
  aprs_data_flag=mm;
}


void debugmsg(char level,char *msg)
{
  if(level<debuglevel){
    printf("(%d) ",level);
    printf("%s", msg);
    printf("\n");
  }
}

