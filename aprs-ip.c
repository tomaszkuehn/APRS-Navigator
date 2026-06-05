#include <includes.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define buflen 512
unsigned int portno = 443;
char hostname[] = "81.15.165.158";

char *buf[buflen];     /* declare global to avoid stack */
int sd;
char connected=0;

void dia(char *sz) { printf("Dia %s\n", sz); }

int printFromSocket(int sd, char *buf)
  {
  int i,len;
  //int continueflag=1;
  //while((len >= buflen)&&(continueflag)) /* quit b4 U read an empty socket */
    
    len = read(sd, buf, buflen);
    if(len>0){
     for(i=0;i<len;i++){
      if((*buf)==0x0D){printf("-END-");}
      printf("%c",(*buf++));
     }
     //printf("END\n");
    }
    //buf[buflen-1]='\0';  /* Note bug if "Finished" ends the buffer */
    //continueflag=(strstr(buf, "Finished")==NULL); /* terminate if server says "Finished" */ 
         
  return(1);
  }

int it_read(char *buf)
{
  int i,len;
  FILE *fo;
  //int continueflag=1;
  //while((len >= buflen)&&(continueflag)) /* quit b4 U read an empty socket */
    if(!connected){return(0);}
    len = read(sd, buf, buflen);
    if(len>0){
     fo=fopen("it.dat","a");
     //printf("Received...");
     //for(i=0;i<len;i++){
     i=0;
     while((i<len)&&(*buf!=0x0F)){
      //if((*buf)==0x0D){printf("-END-");}
      //printf("%c",*buf);
      fprintf(fo,"%c",*buf);
      i++;
      buf++;
     }
     while((i<len)&&(*buf!=0)){
      //if((*buf)==0x0D){printf("-END-");}
      //printf("%c",*buf);
      fprintf(fo,"%c",*buf);
      i++;
      buf++;
     }
     fprintf(fo,"\n");
     //printf("END\n\n");
     fclose(fo);
     return(i);
    } else {len=0;}
  return(0);
}


void it_connect() 
   {
   
   struct sockaddr_in sin;
   struct hostent *host = gethostbyname(hostname);
   char buf[buflen];
   int len;

    if(connected){return;}

   sd = socket(AF_INET, SOCK_STREAM, 0);  /* init socket descriptor */
   /*** PLACE DATA IN sockaddr_in struct ***/
   memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
   sin.sin_family = AF_INET;
   sin.sin_port = htons(portno);

   /*** CONNECT SOCKET TO THE SERVICE DESCRIBED BY sockaddr_in struct ***/
   if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
     {
     perror("connecting");
     exit(1);
     }
   fcntl(sd, F_SETFL, O_NONBLOCK);
   sleep(1);   /* give server time to reply */
   while(0)
     {
     //printf("\n\n");
     printFromSocket(sd, buf);
     //fgets(buf, buflen, stdin);   /* remember, fgets appends the newline */
     //write(sd, buf, strlen(buf));
     //sleep(1);               /* give server time to reply */
     }
   connected=1;
   }



