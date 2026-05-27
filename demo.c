#include "includes.h"

int main (void)
{
char c,c1;
char ss[100];
  //SystemInit();


//  if(SysInit() == 0)
 // {
    // Start user program
 //   __enable_interrupt();
 // }

 
  srand(time(NULL));
  init_timer();  
  initialize_peripherial();
  initialize_flash();
  globals_initialize();
  restore_factory();
  init_gps();

  printf("PP1\n");
  aprs_initialize();
  update_aprs_config();


//  VICIntEnable = 0x00000020;

   //__enable_interrupt();
  printf("PP2\n");

  /* Register I/O backends */
  extern input_backend_t input_backend_x11;
  input_register_backend(&input_backend_x11);

  extern frame_io_driver_t frame_io_simulated;
  extern frame_io_driver_t frame_io_file;
  extern frame_io_driver_t frame_io_udp;
  frame_io_register(FRAME_IO_SIMULATED, &frame_io_simulated);
  frame_io_register(FRAME_IO_FILE, &frame_io_file);
  frame_io_register(FRAME_IO_UDP, &frame_io_udp);

  initLCD();
  printf("PP3\n");

  LCDcolor_foreground=0x0000;
  LCDcolor_background=0xFFFF;
  LCDselect(3);
  LCDclear();
  printf("PP4\n");
  
  
  //sprintf(napis,"REG:%4d,%4d,%4d,%4d",ee[0],ee[1],ee[2],ee[3]);
  //LCDyxtxt(0,10,1,napis);

   c=flash_status();
   //S0SPCR  = 0x00000020;
   //sprintf(napis,"%5d",c);
   //LCDyxtxt(0,30,100,napis);
   //while(1);


  /*
  aprs_initialize();
  LCDyx(1,1);
  while(1){
    c=getchar1();
    LCDchr(0,c);
  } */
#ifdef rsdebug
  printf("Navigator debug mode\n");
#endif

//#define test

  /*
 aprs_exclusive_flag=1;
  sprintf(napis,"APRS  0SP3FHI0  =5225.81N/01656.31E-");
 napis[14]=0x03;
 napis[15]=0xF0;
 aprs_write(0x100,40,napis);
 napis[0]=200;
 aprs_write(0x209,1,napis); //txheader
 napis[0]=1;
 aprs_write(0x202,1,napis);
  */
  //next_beacon_time=0; //transmit now

  //send_beacon();
  //napis[0]=1;
  //aprs_write(0x202,1,napis);


//#define aprs_test
#ifdef aprs_test
 //printf("APRS TEST\n");
 aprs_read(0x311,1,napis);
 c=napis[0];
 sprintf(napis,"0x311:%d\n",c);
 //printf(napis);
 for(c=0;c<200;c++){rpbuf[c]=c;}
 while(1)
 {
   //for(c=0;c<10;c++){printf(" %d ",rpbuf[c]);}printf("\n");
 printf(".");
 aprs_write(0x00,15,rpbuf);
 //aprs_read(0x311,1,napis);
 //c=napis[0];
 //sprintf(napis,"0x311:%d\n",c);
 aprs_read(0x00,15,buffer);
 //for(c=0;c<10;c++){printf(" %d ",buffer[c]);}printf("\n");
 c1=0;
 for(c=0;c<10;c++){if(rpbuf[c]!=buffer[c]){c1=1;};}
 if(c1){LCDyxtxt(0,10,10,"ERROR");printf("\nERROR\n");}
 }
#endif

  setsound(0);





  
  //printf("TEST CRYPT%d\n",test_crypt("01234567"));

  //printf("TEST CALL %d\n",test_call("    ecfh"));
  //for(c=0;c<6;c++){putchar(stations[0].sign[c]);}
  printf("APRS Navigator PC (FHI Navigator)\n");
  printf("(C) 2005,2006 T.Kuehn SP3FHI\n");
  system_userlevel=1;
  printf("User level %d\n",system_userlevel);
  
  //LCDcolor_foreground=RED;
//LCDrectangle(0,0,175,131);
  //LCDyxtxt(0,10,10,"ERROR");

  
  //sprintf(tcpbuf," APZ01 0SP3WBQ0RELAY 0WIDE  0# =5225.00N/01657.00E-TEST pakiet symulowany: dane fikcyjne");
  //tcpbuf[29]=0x03;
  //tcpbuf[30]=0xF0;
  //tcpflag=1;
  //printf("goapp\n");
  
  
  
  goapp();





}
