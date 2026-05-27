/*

Struktura ramek protokolu:

1. Ramki od klientow:

a/ramka bez danych
B: 0          1-3    4-7      
D:[typ ramki][opcje][sekwencja]

b/ ramka z danymi
B: 0          1-3    4-7        +256             +4
D:[typ ramki][opcje][sekwencja][dane do wyslania][autoryzacja]

typ ramki: 0x10=a 0x20=b

2. Ramki do klientow
a/ ramka bez danych

B: 0           1-7            8-11
D:[typ ramki][status modemu][biezaca sekwencja]

b/ ramka z danymi
B: 0          1-7            8-11               +(dane zakonczone "0")
D:[typ ramki][status modemu][biezaca sekwencja][dane]

*/

#include <stdio.h>
          #include <sys/socket.h>
          #include <arpa/inet.h>
          #include <stdlib.h>
          #include <string.h>
          #include <unistd.h>
          #include <netinet/in.h>
	  #include <fcntl.h>
          #define BUFSIZE 300
          void Die(char *mess) { perror(mess); exit(1); }



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
unsigned char tmpbuf[256];

void simpacket()
{
int c;
    
    //if(timeval>mt){
    // mycourse=mycourse+2;
    // if(mycourse>359){mycourse=1;}
    // mt=timeval+5;
    //}
    //if(timeval<simt){return;}
    
    //generate simulated traffic
    //simt=timeval+(unsigned long)(40.0*rand()/(RAND_MAX+1.0))+20;
    //simt=simt+10;
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
    default :sprintf(tmpbuf,"%s",s9);
    
    }
    //sprintf(tcpbuf," APZ01 0SP3WBX0RELAY 0WIDE  0# =5225.00N/01657.00E-received simulated traffic packet# %d",timeval);
    tmpbuf[29]=0x03;
    tmpbuf[30]=0xF0;
    c=(int)(10.0*rand()/(RAND_MAX+1.0))+65;
    tmpbuf[60]=c;
    //tcpbuf[9]=c;
    c=0;
    //while(tmpbuf[c]!=0){bufmem[c]=tmpbuf[c++];}
    //bufmem[c]=0;
}

 int main(int argc, char *argv[]) {
            int sock,len;
            struct sockaddr_in sin,csin;
            unsigned char buffer[BUFSIZE];
            unsigned int echolen;
            int bytes,i;
	    time_t tt;
	    unsigned char ss[BUFSIZE];
	    unsigned long pseq,cseq;
	    unsigned char simsquelch;
	    
            if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
              Die("Failed to create socket");
            }
             /* Construct the server sockaddr_in structure */
            memset(&sin, 0, sizeof(sin));       /* Clear struct */
            sin.sin_family = AF_INET;                  /* Internet/IP */
            sin.sin_addr.s_addr = INADDR_ANY;  /* IP address */
            sin.sin_port = htons(464);       /* server port */
            /* Establish connection */
	    bind(sock,(struct sockaddr *) &sin,  sizeof(sin));
	    fcntl(sock,F_SETFL,O_NONBLOCK);
	    tt=time(NULL);
	    
	    pseq=0;
while(1){
	    if(tt==time(NULL)){simsquelch=1;}else{simsquelch=0;}	    
	    if(tt<time(NULL)){
	      simpacket();
	      pseq=pseq+1;
	      printf("simulated packet received %d\n",pseq);
	      i=(int)(6.0*rand()/(RAND_MAX+1.0))+1;
	      tt=time(NULL)+i;
	    }
	    
	    memset(&csin, 0, sizeof(csin));
	    bytes=sizeof(csin);
	    len=recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&csin, &bytes);
	    if(len>1)
	    {
	     buffer[len]=0;	     
	     inet_ntop(AF_INET,&csin.sin_addr.s_addr,ss,40);
	     //odczytaj sekwencje klienta
	     cseq=buffer[4];cseq=cseq<<8;
	     cseq=cseq+buffer[5];cseq=cseq<<8;
	     cseq=cseq+buffer[6];cseq=cseq<<8;
	     cseq=cseq+buffer[7];
	     
	     printf("received from %s seq:%d\n",ss,cseq);
	     
	     if(cseq<pseq){ //wysylaj pakiet
	       ss[0]=0x20; //data
	       ss[1]=simsquelch;
	       ss[2]=0;
	       ss[3]=0;
	       ss[4]=0;
	       ss[5]=0;
	       ss[6]=0;
	       ss[7]=0;
	       
	       ss[8]=(unsigned char)(pseq&0xFF000000)>>24;
	       ss[9]=(unsigned char)(pseq&0x00FF0000)>>16;
	       ss[10]=(unsigned char)(pseq&0x0000FF00)>>8;
	       ss[11]=(unsigned char)(pseq&0x000000FF);
	       //simpacket();
	       i=0;
	       while(tmpbuf[i]>0){ss[12+i]=tmpbuf[i++];}
	       //printf("i=%d\n",i);
	       ss[12+i]=0;
	       len=14+i;
	       printf("sending packet %s %d\n",tmpbuf,len);
	       
	     }
	     else
	     {
	       ss[0]=0x10; //no data
	       ss[1]=simsquelch;
	       ss[2]=0;
	       ss[3]=0;
	       ss[4]=0;
	       ss[5]=0;
	       ss[6]=0;
	       ss[7]=0;
	       
	       ss[8]=(unsigned char)(pseq&0xFF000000)>>24;
	       ss[9]=(unsigned char)(pseq&0x00FF0000)>>16;
	       ss[10]=(unsigned char)(pseq&0x0000FF00)>>8;
	       ss[11]=(unsigned char)(pseq&0x000000FF);
	       ss[12]=0; //end string
	       len=14;
	       printf("sending status\n");
	       //sprintf(ss,"wyslany status");
	       
	     }
	     sendto(sock, ss, len, 0, (struct sockaddr *)&csin, sizeof(csin));
	     //sprintf(ss,"answer (%s)",buffer);
	     //printf("loop:%s\n",ss);	    
	     
	     
	    }
}//while	    
}	    
