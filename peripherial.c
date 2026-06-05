/* ten modul dostosowany do pracy w symulacji pc */

#include "includes.h"

//char kbdbuf[1];

void initialize_peripherial()
{
}

/* obsluguje zadania spi pochodzace od warstwy przerwan */
/* moze byc wywolany tylko z warstwy aplikacji */
void spi_request_handler()
{
//  T1TCR = 0;    //disable aprs interrupts
  if(spi_buffer_to_flash_rq){
    flash_write(buffer,spi_buffer_to_flash_rq,255);
    spi_buffer_to_flash_rq=0;
  }

  if(spi_txbuf_to_flash_rq){
    flash_write(txbuf,spi_txbuf_to_flash_rq,255);
    spi_txbuf_to_flash_rq=0;
  }

  if((spi_flash_to_rpbuf_rq)&&(spi_flash_to_rpbuf_rq<10000)){
    flash_read(rpbuf,spi_flash_to_rpbuf_rq,255);
    spi_flash_to_rpbuf_rq+=10000;
  }

  if((spi_flash_to_msbuf_rq)&&(spi_flash_to_msbuf_rq<10000)){
    flash_read(msbuf,spi_flash_to_msbuf_rq,255);
    spi_flash_to_msbuf_rq+=10000;
  }

  spirequest=0;
  //T1TCR = 1;
}



void setsound(char id)
{

  switch(id){

  case 1: sounddelay=2;
          soundmod=0;
          soundmodvar=0;
          break;
  case 2: sounddelay=30;
          soundmod=0;
          soundmodvar=0;
          break;
  case 3: sounddelay=100;
          soundmod=0;
          soundmodvar=0;
          break;
  case 4: sounddelay=500;
          soundmod=0;
          soundmodvar=0;
          break;
  case 5: sounddelay=5000;
          soundmod=0;
          soundmodvar=0;
          break;

  case 6: sounddelay=1000;
          soundmod=250;
          soundmodvar=0;
          break;

  case 7: sounddelay=2000;
          soundmod=50;
          soundmodvar=0;
          break;

  case 8: sounddelay=2000;
          soundmod=250;
          soundmodvar=0;
          break;

  case 9: sounddelay=1000;
          soundmod=16;
          soundmodvar=0;
          break;
  }
  //if(id>0)IO0SET=0x400;

}

//static char cm;

static char shift=0;

char read_kbd()
{
char c;
//char napis[6];

if(spirequest>0){spi_request_handler();}
#ifdef nouse
  S0SPCCR = 0x00000040;			//Set bit timing
  IO0CLR=0x200000; //select slave
  waitms(1);
  S0SPDR=0x22;
  while(!(S0SPSR & 0x80));//wait send
  c=S0SPDR;
  IO0SET=0x200000; //deselect slave

  S0SPCCR = SPI_SPEED;			//Set bit timing

  waitms(1);
  if(ser_kbden&&(U0LSR & 0x01)){
    c=U0RBR;
    if(c==8){c=12;} //bkspc
  }
#endif

  if(pixmap_sync){
   pixmap_sync=0;
   pixmap_x_sync();
  }
  c=get_x_key();
  if(shift&&(c>0)){c=c+10;shift=0;}
  if((c==9)&&(!shift)){c=0;shift=1;}
  
  //if(c>0){printf("key...%d\n",c);}
  return c;
}

void send_m8_command(char comm,char *ss)
{

}

char get_m8_byte()
{

}

static unsigned long ev[2];
static unsigned long ek[5];



void crypter(long N)
{

}

void crypter1(long N)
{

}

char test_crypt(char *cc)
{
  
  return(1);
 
}

char test_call(char *cc)
{
    return(1);
}
