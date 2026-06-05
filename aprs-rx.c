/* Ten modul zostal dostosowany do pracy w srodowisku symulacji */

#include "includes.h"
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

char aprs_received_flag;
char aprs_squelch_flag;
char aprs_atomic_flag;
char aprs_exclusive_flag;
char aprs_data_flag;

#define REGLSR  U1LSR
#define REGRBR  U1RBR

static char aprsf=0;
char aprs_packet_source=0; //0-none 1-sim 2-net
char bufmem[0x230];





char s0[]=" APZ01 0SP3WBA0RELAY 0WIDE  0# =5225.09N/01657.09E-received simulated traffic packet#";
char s1[]=" APZ01 0SP3CDE0RELAY 0WIDE  0# =5225.18N/01656.18E-received simulated traffic packet#";
char s2[]=" APZ01 0SP3XX 0RELAY 0WIDE  0# =5226.27N/01657.27E-received simulated traffic packet#";
char s3[]=" APZ01 0SP3VER0RELAY 0WIDE  0# =5227.36N/01658.96E-received simulated traffic packet#";
char s4[]=" APZ01 0SP3TTP0RELAY 0WIDE  0# =5225.45N/01659.45E-received simulated traffic packet#";
char s5[]=" APZ01 0SP3CLL0RELAY 0WIDE  0# =5224.54N/01656.54E-received simulated traffic packet#";
char s6[]=" APZ01 0SP3GR 0RELAY 0WIDE  0# =5226.63N/01657.63E-received simulated traffic packet#";
char s7[]=" APZ01 0SP3TU 0RELAY 0WIDE  0# =5227.72N/01658.72E-received simulated traffic packet#";
char s8[]=" APZ01 0SP3PAS0RELAY 0WIDE  0# =5223.81N/01655.81E-received simulated traffic packet#";
char s9[]=" APZ01 0SP3ECH0RELAY 0WIDE  0# =5225.95N/01657.95E-received simulated traffic packet#";

static unsigned long simt=0;
static unsigned long mt=0;

void simpacket()
{
int c;
char tmpbuf[256];
    
    //if(timeval>mt){
    // mycourse=mycourse+2;
    // if(mycourse>359){mycourse=1;}
    // mt=timeval+5;
    //}
    //if(timeval<simt){return;}
    
    //generate simulated traffic
    simt=timeval+(unsigned long)(40.0*rand()/(RAND_MAX+1.0))+20;
    simt=simt+10;
    c=(int)(10.0*rand()/(RAND_MAX));
    //printf("random:%d\n",c);
    switch (c){
    case 0 :sprintf(tmpbuf,"%s",s0);break;
    case 1 :sprintf(tmpbuf,"%s",s1);break;
    case 2 :sprintf(tmpbuf,"%s",s2);break;
    case 3 :sprintf(tmpbuf,"%s",s3);break;
    case 4 :sprintf(tmpbuf,"%s",s4);break;
    case 5 :sprintf(tmpbuf,"%s",s5);break;
    case 6 :sprintf(tmpbuf,"%s",s6);break;
    case 7 :sprintf(tmpbuf,"%s",s7);break;
    case 8 :sprintf(tmpbuf,"%s",s8);break;
    default :sprintf(tmpbuf,"%s%d",s9,timeval);
    
    }
    //sprintf(tcpbuf," APZ01 0SP3WBX0RELAY 0WIDE  0# =5225.00N/01657.00E-received simulated traffic packet# %d",timeval);
    tmpbuf[29]=0x03;
    tmpbuf[30]=0xF0;
    c=(int)(10.0*rand()/(RAND_MAX+1.0))+65;
    //tcpbuf[12]=c;
    //tcpbuf[9]=c;
    c=0;
    while(tmpbuf[c]!=0){bufmem[c]=tmpbuf[c++];}
    bufmem[c]=0;
}

FILE *it;
char fopenflag=0;

void aprs_rx_fopen()
{
  if(!fopenflag){it=fopen("it.dat","r");}
  fopenflag=1;
}

int file_read(char *bf)
{
fgets(bf,255,it);

}


/**************************************/
/* remote modem */

static int sock;
static unsigned long myseq;

void remote_connect()
{
            struct sockaddr_in echoserver,csin;
            char buffer[300];
            unsigned int echolen;
            int bytes,i,len;
	    time_t tt;
	    char ss[100];
	    int cnt=0;

printf("connecting\n");

            if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
              printf("Failed to create socket");
            }
             /* Construct the server sockaddr_in structure */
            memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
            echoserver.sin_family = AF_INET;                  /* Internet/IP */
            //echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
            echoserver.sin_port = htons(464);       /* server port */
            /* Establish connection */
	    if (bind(sock, (struct sockaddr *)&echoserver,  sizeof(echoserver))<0){
              printf("Failed to connect with server");
            }
	    
	    fcntl(sock, F_SETFL, O_NONBLOCK);
	
//fprintf(stdout, "\n");
//           close(sock);
//           exit(0);
	    myseq=0;
printf("connected\n");
}


void remote_write(char *buf)
{
char ss[300];
struct sockaddr_in sin;
printf("r-write\n");
	    sin.sin_family = AF_INET;                  /* Internet/IP */
            sin.sin_addr.s_addr = inet_addr("127.0.0.1");  /* IP address */
            sin.sin_port = htons(464);       /* server port */
	    sprintf(ss,"hello world hej tam");	    
            sendto(sock, ss, strlen(ss), 0, (struct sockaddr *) &sin,sizeof(sin));

}

void remote_read()
{
struct sockaddr_in csin,sin;
char buffer[300];
int len,i;
int bytes;
unsigned long nseq;

        
    memset(&csin, 0, sizeof(csin));
    bytes=sizeof(csin);
    len=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *) &csin,&bytes);
    if(len>1){
	    //printf("%d ",len);
	nseq=0;
        nseq=nseq+buffer[8];nseq=nseq<<8;
	nseq=nseq+buffer[9];nseq=nseq<<8;
	nseq=nseq+buffer[10];nseq=nseq<<8;
	nseq=nseq+buffer[11];
	//printf("(myseq:%d) rcv:%d\n",myseq,nseq);    
	if(buffer[0]==0x20){
	    //printf("rec pkt\n");
	    if(nseq>myseq){ //new packet
	     i=0;
	     while((i<250)&&(buffer[12+i]!=0x03)){
	       bufmem[i]=buffer[12+i];
	       i++;
	     }
	     while((i<250)&&(buffer[12+i]!=0)){
	       bufmem[i]=buffer[12+i];
	       i++;
	     }
	     bufmem[i]=0;
	     bufmem[0x201]=1;
	    }
	    myseq=nseq;
	}
	if(buffer[0]==0x10){
	  //printf("rec stat\n");
	  if(nseq<myseq){myseq=nseq;}
	}
	bufmem[0x200]=buffer[1];	//squelch
	    //printf("%s\n",buffer);
	    //printf("received %s\n",buffer);
    }

    sin.sin_family = AF_INET;                  /* Internet/IP */
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");  /* IP address */
    sin.sin_port = htons(464);       /* server port */
    buffer[0]=0x10;
    buffer[4]=(unsigned char)(myseq&0xFF000000)>>24;
    buffer[5]=(unsigned char)(myseq&0x00FF0000)>>16;
    buffer[6]=(unsigned char)(myseq&0x0000FF00)>>8;
    buffer[7]=(unsigned char)(myseq&0x000000FF);
    buffer[8]=0;	    
    sendto(sock, buffer, 9, 0, (struct sockaddr *) &sin,sizeof(sin));

} //remote read



/**************/


void aprs_write(unsigned int adr, unsigned char count, unsigned char *string)
{

unsigned char i,c;

           aprsf++;
           if(aprsf>1){printf("RX ERROR\n");}
           //printf("aprs write\n");
	   
	   for(i=0;i<count;i++){
	     bufmem[adr+i]=(*string++);
             //printf("RX:%d\n",c);
	   }
           //putchar1(0);while(!(REGLSR & 0x01)){};c=REGRBR;
           aprsf--;
    
    if(bufmem[0x202]){printf("Buffer to transmit\n");bufmem[0x202]=0;}

}


int parse_is(char *itb)
{
int ob=0;

for(ob=0;ob<255;ob++){bufmem[ob]=' ';}

//printf("parse\n");
    if(*itb=='#'){return(0);}
    if((*itb<'A')||(*itb>'Z')){return(0);}
    bufmem[0]=0;
    ob=8;
    while((ob<255)&&((*itb)!='>')&&((*itb)!='-')){bufmem[ob++]=*itb++;}
    if(ob>18){return(0);}
    while(ob<13){bufmem[ob++]=' ';}
    bufmem[14]='0';
    while((*itb)!='>'){itb++;}
    itb++;
    ob=1;
    while((*itb)!=','){bufmem[ob++]=*itb++;}
    while(ob<7){bufmem[ob++]=' ';}
    bufmem[7]='0';
    ob=15;
    bufmem[ob++]=0x03;
    bufmem[ob++]=0xF0;
    while((*itb)!=':'){itb++;}
    itb++;
    while((ob<200)&&((*itb)!=0)&&((*itb)!=0x0d)){bufmem[ob++]=*itb++;}
    bufmem[ob++]=0;
    return(1);
}


static unsigned long mtmv=0;

void aprs_read(unsigned int adr, unsigned char count, unsigned char *string)
{
char itb[512];
unsigned char i,c;

            aprsf++;
            if(aprsf>1){printf("RX ERROR\n");}
            //printf("aprs read\n");
/*	    
	   c=(unsigned char)((adr&0x0F00)>>8);
	   c=c|0x80;
           putchar1(c);while(!(REGLSR & 0x01)){};c=REGRBR;//printf("RX:%d\n",c);
	   c=(unsigned char)(adr&0x00FF);
	   putchar1(c);while(!(REGLSR & 0x01)){};c=REGRBR;//printf("RX:%d\n",c);
*/	
	   for(i=0;i<count;i++){
            (*string++)=bufmem[adr+i];
	   }
	   
           aprsf--;
	   
    
    //if((mtmv>timeval)&&((timeval+10)>mtmv)){bufmem[0x200]=7;}   //symulacja squelch
    //if(bufmem[0x200]){bufmem[0x200]--;};
    if((aprs_packet_source==3)&&(timeval>mtmv)){
	//it_read(itb);
	mtmv=timeval+(unsigned long)(40.0*rand()/(RAND_MAX+1.0))+5;
	i=file_read(itb);
	if(i>16){
	 if(parse_is(itb))bufmem[0x201]=1;	//rxbufflag
	}
	
    }	   
	   
    if((aprs_packet_source==2)&&(timeval>mtmv)){
	//it_read(itb);
	mtmv=timeval+5;
	//i=it_read(itb);
	if(i>16){
	 if(parse_is(itb))bufmem[0x201]=1;	//rxbufflag
	}
	
    }	
    if((aprs_packet_source==10)&&(timeval>mtmv)){
       mtmv=timeval+(unsigned long)(40.0*rand()/(RAND_MAX+1.0))+20;
       simpacket();
       bufmem[0x201]=1;
    }
    
    if((aprs_packet_source==1)&&(timeval>mtmv)){
       mtmv=timeval+3; //0.3sekundy?
       remote_read();
    }
	
}


void aprs_initialize()
{
 char c;
  aprs_received_flag=0;
  aprs_exclusive_flag=0;
  aprs_data_flag=0;
  //IO0SET= 0x0001000;			//reset aprs
  //IO0SET= 0xFFFFFFFF;
  //putchar1(0);					//clear tx buffer
  //waitms(100);					  //500ms
  //printf("Initializing aprs-rx...\n");
  //IO0CLR = 0x0001000;			//unreset aprs
  //IO0CLR= 0xFFFFFFFF;
  //waitms(10);
  //clear uart buffer
  //if(REGLSR & 0x01){c=REGRBR;}
    bufmem[0x200]=0;	//squelch
    bufmem[0x201]=0;	//rxbufflag
    //it_connect();
    //aprs_packet_source=1;
    //remote_connect();
}

