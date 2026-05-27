#include <stdio.h>
          #include <sys/socket.h>
          #include <arpa/inet.h>
          #include <stdlib.h>
          #include <string.h>
          #include <unistd.h>
          #include <netinet/in.h>
	  #include <fcntl.h>

          #define BUFFSIZE 300
          void Die(char *mess) { perror(mess); exit(1); }

 int main(int argc, char *argv[]) {
            int sock;
            struct sockaddr_in echoserver,csin;
            char buffer[BUFFSIZE];
            unsigned int echolen;
            int bytes,i,len;
	    time_t tt;
	    char ss[100];
	    int cnt=0;


            if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
              Die("Failed to create socket");
            }
             /* Construct the server sockaddr_in structure */
            memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
            echoserver.sin_family = AF_INET;                  /* Internet/IP */
            //echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
            echoserver.sin_port = htons(atoi(argv[2]));       /* server port */
            /* Establish connection */
	    if (bind(sock, (struct sockaddr *)&echoserver,  sizeof(echoserver))<0){
              Die("Failed to connect with server");
            }
	    
	    fcntl(sock, F_SETFL, O_NONBLOCK);
	    
	    echoserver.sin_family = AF_INET;                  /* Internet/IP */
            echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
            echoserver.sin_port = htons(464);       /* server port */
/* Send the word to the server */
	    tt=time(NULL)+2;
while(1){
	
	    if(time(NULL)>tt){    
	    tt=time(NULL)+2;
	    sprintf(ss,"hello %d",cnt++);
	    printf("loop:%s\n",ss);	    
            sendto(sock, ss, strlen(ss), 0, (struct sockaddr *) &echoserver,sizeof(echoserver));
            //fprintf(stdout, "Received: ");
	    i=0;
	    }
	    memset(&csin, 0, sizeof(csin));
	    bytes=sizeof(csin);
	    len=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *) &csin,&bytes);
	    if(len>1){
	    buffer[len]=0;
	    //printf("%s\n",buffer);
	    printf("received %s\n",buffer);
	    }
	    //printf(".");
	
}	    
fprintf(stdout, "\n");
           close(sock);
           exit(0);
          }
