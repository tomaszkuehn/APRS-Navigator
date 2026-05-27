#include "includes.h"

char gpslat[10];
char gpslon[11];
char gpscourse[7];
char gpsspeed[7];
char gpssatnum;

char askgpsdata;

//local buffers
char lastgpslat[10];
char lastgpslon[11];
char lastgpscourse[7];
char lastgpsspeed[7];
char lastgpssatnum;
char sats[3];


char Latitude_Temp[12];
char Longitude_Temp[12];
char Satellites_Temp[12];
char Speed[12];
char Course[12];

char gpsvalid;		//valid coordinates only if gpsvalid==1
char gpgga,gpgga1;
char gprmc;


void update_bygps()
{
int cs;
char i,nwe;

  //cs=gpscourse[2]-'0'+(gpscourse[1]-'0')*10+(gpscourse[0]-'0')*100;

  //while(cs>360){cs=cs-360;}
  //if((cs>360)||(cs<0)){cs=1;}
  i=0;cs=mycourse;
    while((gpscourse[i]>'.')&&(i<3)){i++;}
    if(gpscourse[i]=='.')
    {
     i=0;cs=0;
     while(gpscourse[i]!='.'){
       cs=cs*10;
       cs=cs+gpscourse[i]-'0';
       i++;
     }
    }
  mycourse=cs;
  if((position_mode&0xF0)==0){ //update my position by gps
    nwe=0;
    cs=(gpslat[0]-'0')*10+\
       (gpslat[1]-'0');
    stations[0].position[0]=cs;
    cs=(gpslat[2]-'0')*10+\
       (gpslat[3]-'0');
    stations[0].position[1]=cs;
    cs=(gpslat[5]-'0')*10+\
       (gpslat[6]-'0');
    stations[0].position[2]=cs;
    if(gpslat[7]=='N'){nwe=0x10;}
    cs=(gpslon[0]-'0')*100+\
       (gpslon[1]-'0')*10+\
         (gpslon[2]-'0');
    stations[0].position[3]=cs;
    cs=(gpslon[3]-'0')*10+\
       (gpslon[4]-'0');
    stations[0].position[4]=cs;
    cs=(gpslon[6]-'0')*10+\
       (gpslon[7]-'0');
    stations[0].position[5]=cs;
    if(gpslon[8]=='E'){nwe|=0x01;}
  }

}

void gps_disable(void)
{
 //UCSRB = 0; //disable gps
}

void gps_enable(void)
{
 //UCSRB = (1<<RXCIE)|(0<<TXCIE)|(1<<RXEN)|(0<<TXEN);  //re-enable gps
}


void init_gps(void)
{

    gpsvalid=0;	
    gpgga=0;
    gpgga1=0;
    gprmc=0;
    sprintf(gpslat,"00000000");
    sprintf(gpslon,"000000000");
    sprintf(gpscourse,"000000");

    sprintf(gpsspeed,"000000");
    gpssatnum=0;
    askgpsdata=0;
}






extern void SerHandler (unsigned char newchar)

{
	static unsigned char	commas;		
	static unsigned char	index;		
	static unsigned char	sentence_type;		
        static unsigned char checksums;
        static unsigned char sum,recsum;

        char e;
/*
$GPVTG,288.0,T,288.0,M,045.0,N,083.0,K
$GPZDA,204426,01,05,2006,00
$GPGGA,204426,5007.2480,N,01352.0280,E,2,08,2.0,2,M,50.0,M,,
//$GPGSA,A,3,01,02,03,,05,,07,,09,,11,12,2.0,2.0,2.0
$GPGLL,5007.2520,N,01352.0090,E,204426
//$GPRMC,204426,A,5007.2520,N,01352.0090,E,045.0,288.0,010506,000.0,W
$GPVTG,288.0,T,288.0,M,045.0,N,083.0,K
$GPGGA,192552,4705.5150,N,02137.7800,E,1,08,23.0,2,M,50.0,M,,*65
	*/
//        e=U0LSR;
        e=e&0x0A;
        if(e){//error
          gpgga=0;
          gprmc=0;
          newchar='$';
        }

	if (newchar == '$')					/* Start of Sentence character, reset		*/
	{
		commas = 0;							/* No commas detected in sentence for far	*/
		sentence_type = 0;			/* Clear local parse variable					*/
                sum='\0';
                checksums=0;
                gpgga1=0;
		return;
	}

	

        if (newchar == '*')
        {
                checksums += 1;
                return;
        }

        if(checksums == 0)
        {
          sum^=newchar;
        }

        if (newchar == ',')					/* If there is a comma							*/
	{
		commas += 1;						/* Increment the comma count					*/
		index = 0;							/* And reset the field index					*/
		return;
	}

        if(checksums == 1)
        {
          newchar=newchar-48;
          if(newchar>9){
             newchar=newchar-7;
          }
          recsum=newchar*16;
          checksums++;
          return;
        }

        if(checksums == 2)
        {
          newchar=newchar-48;
          if(newchar>9){
             newchar=newchar-7;
          }
          recsum=recsum+newchar;
          checksums=3;
          if(recsum==sum){
            gpsdata=11;
            if (sentence_type == 3){gprmc=1;}
            if (sentence_type == 2){
              gpgga=1;
              if((Latitude_Temp[4]!='.')||(Longitude_Temp[5]!='.')||(!gpgga1))
              {gpgga=0;}
              if((Latitude_Temp[7]!='N')&&(Latitude_Temp[7]!='S')){gpgga=0;}
              if((Longitude_Temp[8]!='E')&&(Longitude_Temp[8]!='W')){gpgga=0;}
            }
          }
          return;
        }



	if (commas == 0)
	{

		switch(newchar)
		{
			case ('C'):						/* Only the GPRMC sentence contains a "C"	*/

                                sentence_type = 3;	/* Set local parse variable					*/
				break;
			case ('S'):				/* Take note if sentence contains an "S"	*/
				sentence_type = 1;		/* ...because we don't want to parse it	*/
				break;
                        case ('Z'):				/* Take note if sentence contains an "S"	*/
				sentence_type = 1;		/* ...because we don't want to parse it	*/
				break;
                        case ('V'):				/* Take note if sentence contains an "S"	*/
				sentence_type = 1;		/* ...because we don't want to parse it	*/
				break;
			case ('A'):							/* The GPGGA sentence ID contains "A"	*/

				  if (sentence_type != 1)	/* As does GPGSA, which we will ignore	*/
                                  {
					sentence_type = 2;	/* Set local parse variable				*/
                                  }

				break;
		}

		return;
	}

        if (index == 11){sentence_type = 0; return;}
	
	if (sentence_type == 3)
	{
		if(commas == 7){Speed[index++] = newchar;Speed[index]=0;}
                if(commas == 8){Course[index++] = newchar;Course[index]=0;}
                //if(commas == 9){if(ccount==10){gprmc=1;}}
                if(commas == 13){sentence_type = 0;}
                return;
	}

	if (sentence_type == 2)	// GPGGA sentence
	{
		switch (commas)
		{
			
			case (2):									
											
				Latitude_Temp[index++] = newchar;

				return;
			case (3):
				Latitude_Temp[7] = newchar;

				return; 	
			case (4):									
				Longitude_Temp[index++] = newchar;

				return;
			case (5):
				Longitude_Temp[8] = newchar;

				return; 	
                        case (6):
                                if((newchar=='1')||(newchar=='2')||(newchar=='3')){gpgga1=1;}

				return;
			case (7):									
				Satellites_Temp[index++] = newchar;

				return;
                case (15): //error
                                sentence_type = 0;
                                return;
			
		}

		return;
	}	/* end if (sentence_type == GPGGA) */

	

	return;

}					/* End Serial_Handler */

unsigned long gpsreadcounter=0;
unsigned long cc=0;

void gps_read()
{
 char c;

 if(gpsfix){
    for(c=0;c<9;c++){gpslon[c]=lastgpslon[c];}
    for(c=0;c<8;c++){gpslat[c]=lastgpslat[c];}
    for(c=0;c<6;c++){gpscourse[c]=lastgpscourse[c];}
    for(c=0;c<6;c++){gpsspeed[c]=lastgpsspeed[c];}
    gpssatnum=sats[1]-'0'+(sats[0]-'0')*10;
    gpspresent=1;
 }

}

void gps_process(void)
{
  char c;
#ifdef nouse
    c=U0RBR;
    SerHandler(c);
    if(gpgga&&gprmc){

     for(c=0;c<9;c++){lastgpslon[c]=Longitude_Temp[c];}lastgpslon[9]=0;
     for(c=0;c<8;c++){lastgpslat[c]=Latitude_Temp[c];}lastgpslat[8]=0;
     for(c=0;c<6;c++){lastgpscourse[c]=Course[c];}
     for(c=0;c<6;c++){lastgpsspeed[c]=Speed[c];}
     sats[0]=Satellites_Temp[0];
     sats[1]=Satellites_Temp[1];
     //lastgpssatnum=Satellites_Temp[1]-'0'+(Satellites_Temp[0]-'0')*10;
     gpsreadcounter++;
     gpsvalid=1;  //gotowosc nowych danych
     gpsfix=10;    //gps on-line
     gpgga=0;
     gprmc=0;

   }
   /*
   if(U0LSR & 0x01){
     c=U0RBR;
     SerHandler(c);
   }
*/
#endif
}

