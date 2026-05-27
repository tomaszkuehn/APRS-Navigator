
#include "includes.h"

unsigned long timeval=0;    //0.1s tick
unsigned long utimeval=0;   //0.2mS tick
unsigned long sekundnik=0;
unsigned long minutnik=0;
unsigned long minutnik10=0;

char lbuf[255];
char lbufflag;

char transmitinprogress=0;

void waitms(unsigned long tt)
{
int i;
//printf("%d:waitms\n",timeval);
tt=5*tt;
tt=tt+utimeval;
while(utimeval<tt){};
//for(tt=0;tt<1999999;tt++){i++;}
}

static unsigned int tick,tick1;
static char snd=0;



/* Timer Counter 0 Interrupt executes each 10ms @ 60 MHz CPU Clock */
void tc0 (void) {
//  T0TCR = 0;

  utimeval=utimeval+1;
  tick++;
  if(tick>499){
  timeval=timeval+1;
  tick=0;
  }
  //gps processing
#ifdef nouse    
  if(!gpslock){
gpslock=1;
    if(gpsenable&&(U0LSR & 0x01)){   //char received

      gps_process();

    }
    else
    if(askgpsdata){
      gps_read();
      askgpsdata=0;
    }
gpslock=0;
  }
#endif


  //sound
#ifdef nouse
  if(sounddelay){
   sounddelay--;
   if(soundmod>0){
       soundmodvar--;
       if(!soundmodvar){
         snd=1-snd; //flip-flop
         if(snd){IO0CLR=0x400;}else{IO0SET=0x400;}
         soundmodvar=soundmod;
       }
     }
   }
   else
   {//wylacz dzwiek
//     IO0CLR=0x400;
   }
#endif   
//  T0TCR = 1;
//  T0IR        = 1;                            // Clear interrupt flag
//  VICVectAddr = 0;                            // Acknowledge Interrupt

}










static unsigned int tc1c;
static unsigned char tc1err=0;

void tc1_alarm(int sig_num){
 
 if(tc1err){printf("TC ERR\n");return;}

 tc1err++;
 utimeval=utimeval+100;
 
  tick++;
  tick1++;
  if(tick>4){//4
  timeval=timeval+1;
  tick=0;
  pixmap_sync=1;
  }
  if(tick1>20){ //ok 2x/sek
  tick1=0;
  tc1();
  }
  tc1err--;
}

//void tc1 (void) {
void tc1(void){
unsigned char ss[23];
char i;

 
#ifdef APRS_HARDWARE
  if(!aprs_exclusive_flag){
           aprs_exclusive_flag=1;
//           T1TCR = 0;                                  // Timer1 Disable

           if((!aprs_data_flag)&&(tcpflag)&&(!spirequest)){
             for(i=0;i<240;i++){buffer[i]=tcpbuf[i];}

             update_aprs(1);
             tcpflag=0;
             if(uniquemark||allframes){aprs_received_flag=1;}

           }
           //przetwarzanie z bufora tylko gdy bufor pelen - lbufflag
           //i nie jest w trakcie przetwarzania danych - aprs_data_flag
           //i poprzednie przetwarzanie zakonczone - spirequest
           if((!aprs_data_flag)&&lbufflag&&(!spirequest)){
#ifdef rsdebug
             printf("tc1:point1\n");
#endif
             for(i=0;i<240;i++){buffer[i]=lbuf[i];}
             update_aprs(1);
             if(uniquemark||allframes){aprs_received_flag=1;}

             lbufflag=0;
           }

           aprs_read(0x200,2,ss);

           if(ss[0]>0){
             aprs_squelch_flag=1;
           } else {
             aprs_squelch_flag=0;
           }



	   if(ss[1]>0){//new received
             if(aprs_data_flag||spirequest)
             {
#ifdef rsdebug
             printf("tc1:point2\n");
#endif

               if(!lbufflag){

                 //IO0SET=0x400;               //Receive led indicator
	         aprs_read(0x000,240,lbuf);
	         lbuf[0]=' ';
	         ss[0]=0;
	         aprs_write(0x201,1,ss);
                 //IO0CLR=0x400;
                 lbufflag=1;

               }

             } else
             {
#ifdef rsdebug
               printf("tc1:point3:%d\n",aprs_data_flag);
             //printf("tc1:point3\n");

#endif
            //IO0SET=0x400;               //Receive led indicator
	    aprs_read(0x000,240,buffer);
	    buffer[0]=' ';
	    ss[0]=0;
	    aprs_write(0x201,1,ss);
	    update_aprs(1);
            //IO0CLR=0x400;
            if(uniquemark||allframes){aprs_received_flag=1;}

            aprs_atomic_flag=0;
             }
           } else
           {

//transmitter check

             aprs_read(0x202,1,ss); //czy cos czeka do wyslania
             transmitinprogress=ss[0];
              //if(ss[0]==0){//transmitted

                //setsound(soundtab[SND_TRANSMIT]);
              //}


           //jesli transmiter pusty to sprawdzam kolejke do wyslania
           if(!transmitinprogress)
           {
             if(txqueue[0].status>0){
              send_txqueue();
             }
           }

           //czas na kolejny beacon
           if((timeval>next_beacon_time)&&(txqueue[TXQUEUELEN-2].status==0))
           {

               send_beacon();
               next_beacon_time=timeval+beacon_delay;    //5sec
               digisens_time=timeval+170;                //15s

           }
           //digi sens
           if(beacon_digi&&(timeval>digisens_time)&&(txqueue[0].status==0)){
               send_beacon();
               next_beacon_time=timeval+beacon_delay;
               digisens_time=999000000;
           }

           //messages to send?
           if(spi_flash_to_msbuf_rq>10000)
           {
             transmit_message();
           } else
           {
            while(i<TX_MESSAGEBUF)
            {
             if((txmessages[i].status==1)&&\
               (timeval>txmessages[i].time)&&\
                 (txqueue[TXQUEUELEN-1].status==0))
             {
               spi_flash_to_msbuf_rq=txmessages[i].slot;
               spirequest=1;
               i=TX_MESSAGEBUF;
             }
             else {i++;}
            }
           }
           //digi aktywuje tylko jesli w kolejce co najmniej 3 wolne miejsca
           if((txqueue[TXQUEUELEN-3].status==0))
           {
            //verify repeater
             if(spi_flash_to_rpbuf_rq>10000)
             {

                  do_repeat();

             } else //process next entry
             {
              i=0;
              while((i<DIGIQUEUELEN)&&((rpque[i].rpflag!=1)\
                ||(rpque[i].reptime>timeval))){i++;}
              if(i<DIGIQUEUELEN)//ready to repeat
              {
                spi_flash_to_rpbuf_rq=rpque[i].slot;
                spirequest=1;
              }
             }
           }//repeater
           //frame injection from app level
           if(frame_injection){
             application_frame_get();
           }
           } //if ss[1]
//        T1TCR = 1;                                  // Timer0 Enable
           aprs_exclusive_flag=0;
  }
#endif



  //operacje co sekunde
  if(timeval-sekundnik>9){
   sekundnik=timeval;
   if(gpsfix){gpsfix--;}
   if(gpsdata){gpsdata--;}
  }
  //operacje co minute
  if(timeval-minutnik>599){
   minutnik=timeval;
   for(i=59;i>0;i--){
     traffic[i]=traffic[i-1];
     utraffic[i]=utraffic[i-1];
     dtraffic[i]=dtraffic[i-1];
   }
   traffic[0]=0;
   utraffic[0]=0;
   dtraffic[0]=0;
   minutnik10++;
  }

  if(minutnik10>9){
    for(i=119;i>0;i--){htraffic[i]=htraffic[i-1];}
    htraffic[0]=0;
    for(i=1;i<11;i++){htraffic[0]+=traffic[i];}
    minutnik10=0;
  }

//  T1IR        = 1;                            // Clear interrupt flag
//  VICVectAddr = 0;                            // Acknowledge Interrupt
//exit(0);
}




void tc1_poll (void) {
unsigned char i;

if(aprs_exclusive_flag){

            if(spirequest>0){spi_request_handler();}

            if((!aprs_data_flag)&&lbufflag){
#ifdef rsdebug
             printf("tc1_poll\n");
#endif
             for(i=0;i<240;i++){buffer[i]=lbuf[i];}
             update_aprs(1);
             if(uniquemark||allframes){aprs_received_flag=1;}

             lbufflag=0;
           }

}




	

                            // Acknowledge Interrupt

}



/* Setup the Timer Counter 0 Interrupt */
void init_timer (void) {
  //filter_select(1);
  //filter=1;
  //T0MR0 = 1499;                             // 10mSec = 150.000-1 counts
  //T0MR0 = 1401;								  // 10700Hz
  //T0MR0 = 2343;
  //T0MCR = 3;                                  // Interrupt and Reset on MR0
  //T0TCR = 1;                                  // Timer0 Enable
  //VICVectAddr0 = (unsigned long)tc0;          // set interrupt vector in 0
  //VICVectCntl0 = 0x20 | 4;                    // use it for Timer 0 Interrupt
  //VICIntEnable = 0x00000010;                  // Enable Timer0 Interrupt
  //IODIR0|=0x0700;
  signal(SIGALRM, tc1_alarm);
  ualarm(50000,20000);
}
