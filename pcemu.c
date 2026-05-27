#include "includes.h"

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

    return;
    
    if(timeval>mt){
     mycourse=mycourse+2;
     if(mycourse>359){mycourse=1;}
     mt=timeval+5;
    }
    if(timeval<simt){return;}
    
    //generate simulated traffic
    simt=timeval+(unsigned long)(40.0*rand()/(RAND_MAX+1.0))+20;
    simt=simt+10;
    c=(int)(10.0*rand()/(RAND_MAX));
    printf("random:%d\n",c);
    switch (c){
    case 0 :sprintf(tcpbuf,"%s",s0);break;
    case 1 :sprintf(tcpbuf,"%s",s1);break;
    case 2 :sprintf(tcpbuf,"%s",s2);break;
    case 3 :sprintf(tcpbuf,"%s",s3);break;
    case 4 :sprintf(tcpbuf,"%s",s4);break;
    case 5 :sprintf(tcpbuf,"%s",s5);break;
    case 6 :sprintf(tcpbuf,"%s",s6);break;
    case 7 :sprintf(tcpbuf,"%s",s7);break;
    case 8 :sprintf(tcpbuf,"%s",s8);break;
    default :sprintf(tcpbuf,"%s%d",s9,timeval);
    
    }
    //sprintf(tcpbuf," APZ01 0SP3WBX0RELAY 0WIDE  0# =5225.00N/01657.00E-received simulated traffic packet# %d",timeval);
    tcpbuf[29]=0x03;
    tcpbuf[30]=0xF0;
    c=(int)(10.0*rand()/(RAND_MAX+1.0))+65;
    //tcpbuf[12]=c;
    //tcpbuf[9]=c;
    tcpflag=1;
}