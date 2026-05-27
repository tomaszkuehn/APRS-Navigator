//gcc nc.c -lncurses
//

#include <ncurses.h>


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#define BUFSIZE 300


void Die(char *mess) { perror(mess); exit(1); }

typedef struct {
    char busy;
    time_t tt;
    unsigned char data[256];
} txte;

txte txtable[20];

static int fd;		//serial handler

void putchar1(unsigned char c)
{
unsigned char ss[2];
int n;
  //printf("P");fflush(stdout);
  ss[0]=c;
  n=write(fd, ss, 1);
  if(n<1){printf("serial write error\n");}
}

unsigned char getchar1()
{
unsigned char buf[5];
int n;
  //printf("G");fflush(stdout);
  n=read(fd,buf,1);
  if(n!=1){printf("read error\n");}
  return(buf[0]);
}




void data_write(unsigned int adr, unsigned char count, unsigned char *string)
{

unsigned char i,c;

           
           //if(aprsf>1){printf("RX ERROR\n");}
           //printf("aprs write\n");
	   c=(unsigned char)((adr&0x0F00)>>8);
	   c=c|0x40;
           putchar1(c);c=getchar1();//while(!(REGLSR & 0x01)){};c=REGRBR;
	   c=(unsigned char)(adr&0x00FF);
	   putchar1(c);c=getchar1();//while(!(REGLSR & 0x01)){};c=REGRBR;
	   putchar1(count);c=getchar1();//while(!(REGLSR & 0x01)){};c=REGRBR;
	   for(i=0;i<count;i++){
	     putchar1(*string++);c=getchar1();//while(!(REGLSR & 0x01)){};c=REGRBR;
             //printf("RX:%d\n",c);
	   }
           //putchar1(0);while(!(REGLSR & 0x01)){};c=REGRBR;
           

}

void data_read(unsigned int adr, unsigned char count, unsigned char *string)
{

unsigned char i,c;

#define noemu
#ifdef noemu
            
            //if(aprsf>1){printf("RX ERROR\n");}
            //printf("aprs read\n");
	    
	   c=(unsigned char)((adr&0x0F00)>>8);
	   c=c|0x80;
           putchar1(c);c=getchar1();//while(!(REGLSR & 0x01)){};c=REGRBR;//printf("RX:%d\n",c);
	   c=(unsigned char)(adr&0x00FF);
	   putchar1(c);c=getchar1();//while(!(REGLSR & 0x01)){};c=REGRBR;//printf("RX:%d\n",c);
	
	   for(i=1;i<=count;i++){
	    if(i==count){putchar1(1);}else{putchar1(0);}
		//while(!(REGLSR & 0x01)){};
            //c=REGRBR;
	    c=getchar1();
            //printf("RX:%d\n",c);
            (*string++)=c;
	   }
#else
	for(i=0;i<count;i++){(*string++)='a';}
#endif	   
           

}






    
#define BAUDRATE B57600
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
        
      volatile int STOP=FALSE; 
       
void serial_initialize()
      {
        int c, res;
        struct termios oldtio,newtio;
        char buf[255];
        
        fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY ); 
        if (fd <0) {perror(MODEMDEVICE); exit(-1); }
        
        tcgetattr(fd,&newtio); /* save current port settings */
        
        bzero(&newtio, sizeof(newtio));
        //newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;
        
	
        /* set input mode (non-canonical, no echo,...) */
        newtio.c_lflag = 0;
         
        newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */
        
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd,TCSANOW,&newtio);
        
      }
      
      
typedef struct field
{
    char	*name;
    int		id;
    int		x,y,len;
    char	*charset;
    
    char	value[50];
    int		addr;
    char	format;
    int		bytes;
    char 	*help;
} FIELD;

FIELD ff[40];
#define NAME_LEN	20
#define NUM_FIELDS	38
#define NUM_MENUS	2

typedef struct menu_st {
    unsigned char items[30];
    char	title1[80];
    char	title2[80];
    int		title2y;
} menu_st;
menu_st	menus[]={
    {
    .items={0,1,2,3,4,5,6,7,8,9,37,36,10,11,12,13,14,15,16,0,255},
    .title1="------------------------ STATION SETTINGS -------------------------",
    .title2="------------------------- SYSTEM SETTINGS -------------------------",
    .title2y=12
    },
    {
    .items={17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,255},
    .title1="-------------------------- DIGI SETTINGS --------------------------",
    .title2="",
    .title2y=0
    }
    };
    
char	chgflag=0;    
char 	fwstring[6];
      
int instring(char c, char *ss){
char found=0;
    while(*ss){
	if(*ss==c){found=1;}
	ss++;
    }
    return found;
}

int edit_field(int id,char *initstr)
{
char exit=0;
unsigned char c,i;
char pos=0;
unsigned char ss[40];
int len,x,y;
char *valptr;

len=ff[id].len;
x=ff[id].x+NAME_LEN+2;
y=ff[id].y;

for(i=0;i<len;i++){ss[i]=' ';}
while(*initstr){
    ss[pos++]=*initstr++;
}
c=255;
while(exit==0){
    //mvprintw(10,30,"%d        ",c);
    if(c!=255)
    switch(c){
    case 127:ss[pos]=' ';if(pos>0){pos=pos-1;};break;
    case 27:exit=1;break;
    case 10:exit=2;break;
    default: if(instring(c,ff[id].charset)){if(pos<len){ss[pos]=c;pos++;};}
    }
    if(!exit){
     for(i=0;i<len;i++){mvprintw(y,x+i,"%c",ss[i]);}
     mvprintw(y,x+len," ");
     mvprintw(y,x+pos,"_");
     c=getch();
     refresh();
    }
}

    valptr=ff[id].value;
    if(exit==2){
	for(i=0;i<pos;i++){ff[id].value[i]=ss[i];}
	ff[id].value[i]=0;
	//*initstr=0;
	chgflag=1;
    }
    //sprintf(*ff[id].value,"elamela");
    //ff[id].value=ss;
    for(i=0;i<=len;i++){
	mvprintw(y,x+i," ");
    }
    return(exit);	
}

void redraw(int curmenu, int curpos){
int i,k,pos;
unsigned char ss[40];
unsigned long beacon_delay;
char *opts;

      for(i=1;i<19;i++){mvprintw(i,0,"|");}
      for(i=1;i<19;i++){mvprintw(i,33,"|");}
      for(i=1;i<19;i++){mvprintw(i,66,"|");}

    mvprintw(0,0,"------------ APRS-Navigator DIGI interface (C) SP3FHI -------------");
    mvprintw(1,0,"%s",menus[curmenu].title1);
    mvprintw(18,0,"-------------------------------------------------------------------");    
    //mvprintw(7,0,"---------------------SYSTEM SETING---------------------------");
    if(menus[curmenu].title2y>0){
    mvprintw(menus[curmenu].title2y,0,"%s",menus[curmenu].title2);
    };
    //mvprintw(12,0,"---------------------DIGIPEATER-----------------------------");
    //----------POS 001
      //mvprintw(1,0,"| CALLSIGN    :");
      //data_read(16,6,ss);
      //if(curpos==1)attron(A_BOLD);
      //for(i=0;i<6;i++){
        //mvprintw(1,i+16,"%c",ss[i]);
      //}
      //attroff(A_BOLD);
      
      //data_read(22,2,ss);
      //if(curpos==2)attron(A_BOLD);
      //mvprintw(3,0,"| SSID        :%4d",ss[0]);
      //attroff(A_BOLD);
     
      //mvprintw(4,0,"| SYMBOL      :%4d",ss[1]);
      //data_read(30,4,ss); //beacon delay
      //beacon_delay=ss[0]+256*ss[1];
      //beacon_delay=beacon_delay/10;
      //mvprintw(5,0,"| BEACON DELAY:%4ds",beacon_delay);
      //for(i=0;i<4;i++){mvprintw(5,30+4*i,"%d",ss[i]);}
      
      k=0;
      while(menus[curmenu].items[k]!=255){
        i=menus[curmenu].items[k];
	mvprintw(ff[i].y,ff[i].x+NAME_LEN-1," ");
	if(curpos==i){
	    attron(A_BOLD);
	    mvprintw(19,1,"                                                          ");
	    mvprintw(19,1,"HELP:%s",ff[i].help);
	    mvprintw(ff[i].y,ff[i].x+NAME_LEN-1,"*");
	    }
        mvprintw(ff[i].y,ff[i].x,ff[i].name);
	mvprintw(ff[i].y,ff[i].x+NAME_LEN,":");
	mvprintw(ff[i].y,ff[i].x+NAME_LEN+2,ff[i].value);
	
	attroff(A_BOLD);
	k=k+1;
      
        }
      if(chgflag){
        attron(A_BOLD);
	mvprintw(20,1,"CHANGES NOT SAVED TO EEPROM");
	attroff(A_BOLD);
      }	
      else
      {
        mvprintw(20,1,"                           ");
      }
      /*
      for(i=0;i<NUM_FIELDS;i++){
        if(curpos==i){
	    attron(A_BOLD);
	    mvprintw(23,1,"                                                          ");
	    mvprintw(23,1,"HELP:%s",ff[i].help);
	    }
        mvprintw(ff[i].y,ff[i].x,ff[i].name);
	mvprintw(ff[i].y,ff[i].x+NAME_LEN,":");
	mvprintw(ff[i].y,ff[i].x+NAME_LEN+2,ff[i].value);
	
	attroff(A_BOLD);
      }*/
      mvprintw(21,1,"[m]-main config [d]-digi config [F]-reset to factory");
      mvprintw(22,1,"[b]-send beacon  [r]-refresh [e]-update eeprom  [x]-exit");
      mvprintw(18,4," FW:%s ",fwstring);
      refresh();
}

void read_value(int id)
{
unsigned char ss[50];
unsigned char path[50];
unsigned char ss1[10];
char format;
int addr,bytes,i;
unsigned long num,mul;
unsigned char c,c1,ssp;
    addr=ff[id].addr;
    bytes=ff[id].bytes;
    data_read(addr,bytes,ss);
    //for(i=0;i<4;i++){mvprintw(5,30+4*i,"%d",ss[i]);}
    format=ff[id].format;
    switch(format){
    case 's': for(i=0;i<bytes;i++){ff[id].value[i]=ss[i];};ff[id].value[i]=0;break;
    case 'b': num=0;
	      mul=1;
	      for(i=0;i<bytes;i++){
		num=num+ss[i]*mul;
		mul=mul*256;
	      };
	      num=num/10;
	      sprintf(ff[id].value,"%d",num);
	      break;
    case 'n': num=0;
	      mul=1;
	      for(i=0;i<bytes;i++){
		num=num+ss[i]*mul;
		mul=mul*256;
	      };
	      //num=ss[0]+256*ss[1];
	      sprintf(ff[id].value,"%d",num);
	      break;
    case 'p': 	if(ss[6]&0xF0>0){c='S';}else{c='N';}
		if(ss[6]&0x0F>0){c1='W';}else{c1='E';}
		sprintf(ff[id].value,"%02d.%02d%02d%c %03d.%02d%02d%c",ss[0],ss[1],ss[2],c,ss[3],ss[4],ss[5],c1);    
		break;
    case 'a':	data_read(104,1,ss1);//path0 plen
		bytes=ss1[0];
		ssp=0;
		for(i=0;i<bytes;i++){
		    c=0;
		    while( (c<6) && (ss[7*i+c]>' ') ) {path[ssp++]=ss[7*i+c];c++;};
		    //if(ss[7*i+6]>'0'){path[ssp++]='-';path[ssp++]=ss[7*i+6];};
		    if(i<bytes-1)path[ssp++]=',';
		};
		path[ssp++]=0;
		sprintf(ff[id].value,"%s",path);break;		
    default :	sprintf(ff[id].value,"ERR");		
    }
    
}


void update_value(int id)
{
unsigned char ss[40];
char format;
int addr,bytes,i,nn,len;
unsigned long num,mul;
unsigned char c,c1,p,n,k;
    addr=ff[id].addr;
    bytes=ff[id].bytes;
    //data_read(addr,bytes,ss);
    //for(i=0;i<4;i++){mvprintw(5,30+4*i,"%d",ss[i]);}
    format=ff[id].format;
    switch(format){
    case 's': 	for(i=0;i<bytes;i++){ss[i]=ff[id].value[i];};
		data_write(addr,bytes,ss);
		break;
    case 'n':	num=atoi(ff[id].value);
		for(i=0;i<bytes;i++){
		    mul=num/256;
		    mul=mul*256;
		    nn=num-mul;
		    ss[bytes-i-1]=nn;
		    num=num/256;
		};
		data_write(addr,bytes,ss);
		break;
    case 'b':	num=atoi(ff[id].value);
		num=num*10;
		for(i=0;i<bytes;i++){
		    mul=num/256;
		    mul=mul*256;
		    nn=num-mul;
		    ss[i]=nn;
		    num=num/256;
		};
		data_write(addr,bytes,ss);
		break;
    case 'p': 	c=0;
		if(ff[id].value[7]=='S'){c=0x10;}
		if(ff[id].value[17]=='W'){c=c|0x01;}
		ss[0]=(ff[id].value[0]-'0')*10+ff[id].value[0]-'0';
		ss[1]=(ff[id].value[3]-'0')*10+ff[id].value[4]-'0';
		ss[2]=(ff[id].value[5]-'0')*10+ff[id].value[6]-'0';
		ss[3]=(ff[id].value[9]-'0')*100+(ff[id].value[10]-'0')*10+ff[id].value[11]-'0';
		ss[4]=(ff[id].value[13]-'0')*10+ff[id].value[14]-'0';
		ss[5]=(ff[id].value[15]-'0')*10+ff[id].value[16]-'0';
		data_write(addr,bytes,ss);
		break;						
    case 'a':	i=0;
		while(ff[id].value[i]>0){i++;}		
		ff[id].value[i++]=0;
		ff[id].value[i++]=0;
		ff[id].value[i++]=0;
		ff[id].value[i++]=0;
		p=0;n=0;
		while((n<5)&&(ff[id].value[p]>0))
		{
		 k=0;
		 while((k<6)&&(ff[id].value[p]>0)&&(ff[id].value[p]!=','))
		 {
		  ss[7*n+k]=ff[id].value[p];
		  p++;k++;
		 }
		while(k<6){ss[7*n+k]=' ';k++;}
		
		if((ff[id].value[p-1]<='9')&&(ff[id].value[p-1]>='0')){
		 ss[7*n+6]=ff[id].value[p-1];
		 }
		else
		{
		 ss[7*n+6]='0';
		} 
		n=n+1;
		len=n;
		k=k+1;
		p=p+1;
		};
		data_write(105,34,ss);
		ss[0]=len;
		data_write(104,1,ss);
		break;
    }
    
}

void data_refresh()
{
int i;

    for(i=0;i<NUM_FIELDS;i++){
	read_value(i);
    };
}

int cforward(int curmenu, int curpos)
{
int i;
i=0;
while((menus[curmenu].items[i]!=curpos)&&(menus[curmenu].items[i]!=255)){i++;}
if(menus[curmenu].items[i]==curpos)
{
    curpos=menus[curmenu].items[i+1];
    if(curpos==255){curpos=menus[curmenu].items[i];}
}
else
{
curpos=menus[curmenu].items[0];
}
return(curpos);
}

int cbackward(int curmenu, int curpos)
{
int i;
i=0;
while((menus[curmenu].items[i]!=curpos)&&(menus[curmenu].items[i]!=255)){i++;}
if(menus[curmenu].items[i]==curpos)
{
if(i>0){curpos=menus[curmenu].items[i-1];}else{curpos=menus[curmenu].items[0];}
}
else
{
curpos=menus[curmenu].items[0];
}
return(curpos);
}


int main()
{
char i,c,y;
unsigned char ss[20];
unsigned long beacon_delay;
int curpos,curmenu;

    serial_initialize();


    ff[0].name="CALLSIGN";
    ff[0].x=2;
    ff[0].y=2;
    ff[0].len=6;
    ff[0].charset="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    ff[0].addr=16;
    ff[0].format='s';
    ff[0].bytes=6;
    ff[0].help="";
    
    ff[1].name="SSID";
    ff[1].x=35;
    ff[1].y=2;
    ff[1].len=2;
    ff[1].addr=22;
    ff[1].format='n';
    ff[1].bytes=1;
    ff[1].charset="1234567890";
    ff[1].help="";
    
    ff[2].name="BEACON MODE";
    ff[2].x=2;
    ff[2].y=3;
    ff[2].len=1;
    ff[2].charset="0";
    ff[2].addr=29;
    ff[2].format='n';
    ff[2].bytes=1;
    ff[2].help="0-STATIC DELAY";
    
    ff[3].name="SYMBOL";
    ff[3].x=35;
    ff[3].y=3;
    ff[3].len=1;
    ff[3].charset="012345";
    ff[3].addr=23;
    ff[3].format='n';
    ff[3].bytes=1;
    ff[3].help="0-HOUSE 1-CAR 2-W 3-DIGI 4-HORSE 5-SHUTTLE";
    
    ff[4].name="POSITION USE";
    ff[4].x=2;
    ff[4].y=4;
    ff[4].len=1;
    ff[4].charset="012";
    ff[4].addr=27;
    ff[4].format='n';
    ff[4].bytes=1;
    ff[4].help="0-USE STATIC POS0";
    
    ff[5].name="BEACON DELAY (s)";
    ff[5].x=35;
    ff[5].y=4;
    ff[5].len=7;
    ff[5].addr=30;
    ff[5].format='b';
    ff[5].bytes=4;
    ff[5].charset="1234567890";
    ff[5].help="";
    
    ff[6].name="POSITION MODE";
    ff[6].x=2;
    ff[6].y=5;
    ff[6].len=2;
    ff[6].charset="34";
    ff[6].addr=34;
    ff[6].format='n';
    ff[6].bytes=1;
    ff[6].help="34-REPORT STATIC 33-STATUS ONLY";
    
    ff[7].name="DIGISENS";
    ff[7].x=35;
    ff[7].y=5;
    ff[7].len=1;
    ff[7].addr=28;
    ff[7].format='n';
    ff[7].bytes=1;
    ff[7].charset="10";
    ff[7].help="";

    ff[8].name="DEST ADDR";
    ff[8].x=2;
    ff[8].y=6;
    ff[8].len=6;
    ff[8].addr=90;
    ff[8].format='s';
    ff[8].bytes=6;
    ff[8].charset="10";
    ff[8].help="";
    
    ff[9].name="STATIC POS0";
    ff[9].x=2;
    ff[9].y=7;
    ff[9].len=18;
    ff[9].addr=40;
    ff[9].format='p';
    ff[9].bytes=7;
    ff[9].charset="0123456789.NESW ";
    ff[9].help="52.2581N 016.5731E";
    
    ff[10].name="TXENABLE";
    ff[10].x=2;
    ff[10].y=13;
    ff[10].len=1;
    ff[10].addr=25;
    ff[10].format='n';
    ff[10].bytes=1;
    ff[10].charset="10";
    ff[10].help="0-DISABLE TX 1-ENABLE TX";
    
    ff[11].name="USE SQUELCH";
    ff[11].x=35;
    ff[11].y=13;
    ff[11].len=1;
    ff[11].addr=37;
    ff[11].format='n';
    ff[11].bytes=1;
    ff[11].charset="10";
    ff[11].help="0-SOFTWARE SQL 1-HARDWARE SQL";
    
    ff[12].name="TXHEADER";
    ff[12].x=2;
    ff[12].y=14;
    ff[12].len=2;
    ff[12].addr=97;
    ff[12].format='n';
    ff[12].bytes=1;
    ff[12].charset="1234567890";
    ff[12].help="PACKET HEADER LEN";
    
    ff[13].name="TXDELAY";
    ff[13].x=35;
    ff[13].y=14;
    ff[13].len=2;
    ff[13].addr=96;
    ff[13].format='n';
    ff[13].bytes=1;
    ff[13].charset="1234567890";
    ff[13].help="";
    
    ff[14].name="BIT DELAY";
    ff[14].x=2;
    ff[14].y=15;
    ff[14].len=3;
    ff[14].addr=98;
    ff[14].format='n';
    ff[14].bytes=1;
    ff[14].charset="1234567890";
    ff[14].help="DEFAULT 187";
    
    ff[15].name="RX TUNE";
    ff[15].x=35;
    ff[15].y=15;
    ff[15].len=2;
    ff[15].addr=99;
    ff[15].format='n';
    ff[15].bytes=1;
    ff[15].charset="1234567890";
    ff[15].help="DEFAULT 75";
    
    ff[16].name="ANSWER QUERY";
    ff[16].x=2;
    ff[16].y=16;
    ff[16].len=1;
    ff[16].addr=24;
    ff[16].format='n';
    ff[16].bytes=1;
    ff[16].charset="10";
    ff[16].help="0-NO 1-YES";
    
    ff[17].name="DIGI ENABLE";
    ff[17].x=2;
    ff[17].y=3;
    ff[17].len=1;
    ff[17].addr=26;
    ff[17].format='n';
    ff[17].bytes=1;
    ff[17].charset="10";
    ff[17].help="0-DISABLE 1-ENABLE";
    
    ff[18].name="DIGI DUPETIME (s)";
    ff[18].x=35;
    ff[18].y=3;
    ff[18].len=2;
    ff[18].addr=35;
    ff[18].format='n';
    ff[18].bytes=1;
    ff[18].charset="1234567890";
    ff[18].help="TIMEOUT TO DROP PACKET IN BUFFER";
    
    ff[19].name="DIGI DELAY (s/10)";
    ff[19].x=2;
    ff[19].y=4;
    ff[19].len=2;
    ff[19].addr=36;
    ff[19].format='n';
    ff[19].bytes=1;
    ff[19].charset="1234567890";
    ff[19].help="DELAY BEFORE BUFFERING TO REPEAT (DEFAULT 0) s/10";
    
    ff[20].name="DIGI ALIAS #0";
    ff[20].x=2;
    ff[20].y=6;
    ff[20].len=6;
    ff[20].addr=50;
    ff[20].format='s';
    ff[20].bytes=6;
    ff[20].charset="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789* ";
    ff[20].help="ENTER * AS WILDCARD FINISH WITH SPACES";
    
    ff[21].name="ALIAS #0 ENABLE";
    ff[21].x=35;
    ff[21].y=6;
    ff[21].len=1;
    ff[21].addr=56;
    ff[21].format='n';
    ff[21].bytes=1;
    ff[21].charset="10";
    ff[21].help="";
    
    ff[22].name="ALIAS #0 TYPE";
    ff[22].x=2;
    ff[22].y=7;
    ff[22].len=1;
    ff[22].addr=57;
    ff[22].format='n';
    ff[22].bytes=1;
    ff[22].charset="120";
    ff[22].help="0-SIMPLE 1-FLOOD 2-TRACE";
    
    ff[23].name="ALIAS #0 HOPLIMIT";
    ff[23].x=35;
    ff[23].y=7;
    ff[23].len=1;
    ff[23].addr=58;
    ff[23].format='n';
    ff[23].bytes=1;
    ff[23].charset="1234567890";
    ff[23].help="DEFAULT 3";
    
    ff[24].name="DIGI ALIAS #1";
    ff[24].x=2;
    ff[24].y=9;
    ff[24].len=6;
    ff[24].addr=60;
    ff[24].format='s';
    ff[24].bytes=6;
    ff[24].charset="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789* ";
    ff[24].help="ENTER * AS WILDCARD FINISH WITH SPACES";
    
    ff[25].name="ALIAS #1 ENABLE";
    ff[25].x=35;
    ff[25].y=9;
    ff[25].len=1;
    ff[25].addr=66;
    ff[25].format='n';
    ff[25].bytes=1;
    ff[25].charset="10";
    ff[25].help="";
    
    ff[26].name="ALIAS #1 TYPE";
    ff[26].x=2;
    ff[26].y=10;
    ff[26].len=1;
    ff[26].addr=67;
    ff[26].format='n';
    ff[26].bytes=1;
    ff[26].charset="120";
    ff[26].help="0-SIMPLE 1-FLOOD 2-TRACE";
    
    ff[27].name="ALIAS #1 HOPLIMIT";
    ff[27].x=35;
    ff[27].y=10;
    ff[27].len=1;
    ff[27].addr=68;
    ff[27].format='n';
    ff[27].bytes=1;
    ff[27].charset="1234567890";
    ff[27].help="DEFAULT 3";
    
    ff[28].name="DIGI ALIAS #2";
    ff[28].x=2;
    ff[28].y=12;
    ff[28].len=6;
    ff[28].addr=70;
    ff[28].format='s';
    ff[28].bytes=6;
    ff[28].charset="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789* ";
    ff[28].help="ENTER * AS WILDCARD FINISH WITH SPACES";
    
    ff[29].name="ALIAS #2 ENABLE";
    ff[29].x=35;
    ff[29].y=12;
    ff[29].len=1;
    ff[29].addr=76;
    ff[29].format='n';
    ff[29].bytes=1;
    ff[29].charset="10";
    ff[29].help="";
    
    ff[30].name="ALIAS #2 TYPE";
    ff[30].x=2;
    ff[30].y=13;
    ff[30].len=1;
    ff[30].addr=77;
    ff[30].format='n';
    ff[30].bytes=1;
    ff[30].charset="120";
    ff[30].help="0-SIMPLE 1-FLOOD 2-TRACE";
    
    ff[31].name="ALIAS #2 HOPLIMIT";
    ff[31].x=35;
    ff[31].y=13;
    ff[31].len=1;
    ff[31].addr=78;
    ff[31].format='n';
    ff[31].bytes=1;
    ff[31].charset="1234567890";
    ff[31].help="DEFAULT 3";
    
    ff[32].name="DIGI ALIAS #3";
    ff[32].x=2;
    ff[32].y=15;
    ff[32].len=6;
    ff[32].addr=80;
    ff[32].format='s';
    ff[32].bytes=6;
    ff[32].charset="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789* ";
    ff[32].help="ENTER * AS WILDCARD FINISH WITH SPACES TO 6 CHRS";
    
    ff[33].name="ALIAS #3 ENABLE";
    ff[33].x=35;
    ff[33].y=15;
    ff[33].len=1;
    ff[33].addr=86;
    ff[33].format='n';
    ff[33].bytes=1;
    ff[33].charset="10";
    ff[33].help="";
    
    ff[34].name="ALIAS #3 TYPE";
    ff[34].x=2;
    ff[34].y=16;
    ff[34].len=1;
    ff[34].addr=87;
    ff[34].format='n';
    ff[34].bytes=1;
    ff[34].charset="120";
    ff[34].help="0-SIMPLE 1-FLOOD 2-TRACE";
    
    ff[35].name="ALIAS #3 HOPLIMIT";
    ff[35].x=35;
    ff[35].y=16;
    ff[35].len=1;
    ff[35].addr=88;
    ff[35].format='n';
    ff[35].bytes=1;
    ff[35].charset="1234567890";
    ff[35].help="DEFAULT 3";
    
    ff[36].name="STATUS TEXT";
    ff[36].x=2;
    ff[36].y=10;
    ff[36].len=34;
    ff[36].addr=140;
    ff[36].format='s';
    ff[36].bytes=34;
    ff[36].charset=" ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890,!.;:()";
    ff[36].help="";
    
    
    ff[37].name="PATH 0";
    ff[37].x=2;
    ff[37].y=9;
    ff[37].len=34;
    ff[37].addr=105;
    ff[37].format='a';
    ff[37].bytes=34;
    ff[37].charset="ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890,";
    ff[37].help="EX:RELAY,WIDE RELAY,SP3 SP3 WIDE1";
        
    data_refresh();
    
    data_read(175,5,fwstring);
    
    if(fwstring[1]!='.'){printf("Device not compatible or PC mode is not enabled\n\n");exit(0);}
    
    initscr();
    noecho();
    curs_set(0);
    halfdelay(10);
    c=0;
    y=0;
    while(c!='x'){
      c=getch();
      if(c==65){curpos=cbackward(curmenu,curpos);}
      if(c==66){curpos=cforward(curmenu,curpos);}
      if(curpos<0){curpos=0;}
      if(curpos>=NUM_FIELDS){curpos=NUM_FIELDS-1;}
      if(c=='b'){data_write(2,1,ss);}//send beacon
      if(c=='e'){ss[0]=1;data_write(1,1,ss);}//save eeprom
      if(c=='F'){ss[0]=2;data_write(1,1,ss);}//save eeprom
      if(c=='r'){data_refresh();}
      if(c=='d'){clear();curmenu=1;curpos=cforward(curmenu,-1);}
      if(c=='m'){clear();curmenu=0;curpos=cforward(curmenu,-1);}
      if(c==10){
    	    c=edit_field(curpos,ff[curpos].value);
	    if(c==2){update_value(curpos);}
	}	
      redraw(curmenu,curpos);
    }
    endwin();
    
}
