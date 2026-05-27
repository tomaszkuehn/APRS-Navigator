#include "includes.h"

//#define flash
#define flashemu

unsigned int pagepoint=0;
char eepromfull=0;

typedef struct {
    char data[256];
} _flashpage;

_flashpage flashmem[2048];

char flash_status()
{

char c;

  //waitms(1);
  c=156;
  return c;
}

unsigned int get_freepage()
{
  char i,err,fm;

  err=1;
  fm=aprs_data_flag;
  aprs_data_flag=1;
  while(err)
  {
  err=0;
  pagepoint++;
  if(pagepoint>MAXEEPROM){pagepoint=1;eepromfull=1;}
  //check if this slot is busy by station
  for(i=0;i<=num_stations;i++){
    if(stations[i].slot==pagepoint)
    {
     err=1;

    }
  }
  //check if this slot is busy by message
  for(i=0;i<MAX_MESSAGES;i++){
   if((messages[i].status>0)&&(messages[i].slot==pagepoint))
   {
     err=1;
   }
  }
  //tx messages
  for(i=0;i<TX_MESSAGEBUF;i++){
   if((txmessages[i].status>0)&&(txmessages[i].slot==pagepoint))
   {
     err=1;
   }
  }
  //application lock
  if(pagepoint==slot_lock){err=1;}

  }//while
  aprs_data_flag=fm;
  return pagepoint;
}


//P0.28 CS
//P0.29 RESET
void initialize_flash()
{

}


void flash_read(char *data,unsigned int page, char count)
{
int i;
#ifdef flash
  char c;
  unsigned int i;



 while(flash_status()!=156);

 IO0CLR=0x10000000; //select slave
 //waitms(1);
 S0SPDR=0xD2;
 while(!(S0SPSR & 0x80));//wait send

 page=page<<1;
 i=page;
 i=i>>8;
 c=i&0x00FF;
 S0SPDR=c;
 while(!(S0SPSR & 0x80));//wait send

 c=page&0x00FF;
 S0SPDR=c;
 while(!(S0SPSR & 0x80));//wait send

 S0SPDR=0x00;
 while(!(S0SPSR & 0x80));//wait send

 for(i=0;i<4;i++){
 S0SPDR=0x00;
 while(!(S0SPSR & 0x80));//wait send
 }

 for(i=0;i<count;i++)
 {
   S0SPDR=0;
   while(!(S0SPSR & 0x80));//wait send
   (*data)=S0SPDR;
   data++;
 }
 IO0SET=0x10000000; //deselect slave
 //waitms(1);
#endif

    for(i=0;i<count;i++){
	(*data)=flashmem[page].data[i];
	data++;
    }

}


void flash_write(char *data,unsigned int page, char count)
{
  char c;
  unsigned int i;
#ifdef flash
#ifdef rsdebug
             printf("flash_write\n");
#endif


 while(flash_status()!=156);

#ifdef rsdebug
 printf("flash_write:ready\n");
#endif
 IO0CLR=0x10000000; //select slave
 //waitms(1);
 S0SPDR=0x82;
 while(!(S0SPSR & 0x80));//wait send

 page=page<<1;
 i=page;
 i=i>>8;
 c=i&0x00FF;
 S0SPDR=c;
 while(!(S0SPSR & 0x80));//wait send

 c=page&0x00FF;
 S0SPDR=c;
 while(!(S0SPSR & 0x80));//wait send

 S0SPDR=0x00;
 while(!(S0SPSR & 0x80));//wait send

 for(i=0;i<count;i++)
 {
   S0SPDR=(*data++);
   while(!(S0SPSR & 0x80));//wait send

 }
 IO0SET=0x10000000; //deselect slave

 //waitms(1);
#ifdef rsdebug
             printf("flash_write_end\n");
#endif

#endif
    for(i=0;i<count;i++){
	flashmem[page].data[i]=(*data++);
    }

}








