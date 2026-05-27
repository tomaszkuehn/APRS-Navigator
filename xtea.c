#include <stdio.h>


unsigned long k[4];
unsigned long v[4];


void xtea(long N)
{
unsigned long y=v[0];
unsigned long z=v[1];
unsigned long DELTA=0x9e3779b9;

if(N>0){
unsigned long limit=DELTA*N;
unsigned long sum=0;
while(sum!=limit){
 y+=((z<<4 ^ z>>5) + z) ^ (sum + k[sum&3]);
 sum+=DELTA;
 z+=((y<<4 ^ y>>5) + y) ^ (sum + k[(sum>>11)&3]);
}
}
else
{
unsigned long sum=DELTA*(-N);
while(sum){
 z-=((y<<4 ^ y>>5) +y ) ^ (sum + k[(sum>>11)&3]);
 sum-=DELTA;
 y-=((z<<4 ^ z>>5) +z ) ^ (sum + k[sum&3]);
}
}
v[0]=y;
v[1]=z;

}

test()
{
 printf("Tea test \n");
 k[0]=0x00000000;
 k[1]=0x00000000;
 k[2]=0x00000000;
 k[3]=0x00000000; /* keys redacted */
 
 v[0]=0x00000000;
 v[1]=0x05060708;
 
  printf("%8x%8x\n",v[0],v[1]);
 printf("encoding...\n");
 xtea(28);
 printf("%8x%8x\n",v[0],v[1]);
 printf("decoding...\n");
 xtea(-28);
 printf("%8x%8x\n",v[0],v[1]);
 
 v[0]=0x19370000;
 v[1]=0x00000000;
 
 printf("%8x%8x\n",v[0],v[1]);
 printf("encoding...\n");
 xtea(8);
 printf("%8x%8x\n",v[0],v[1]);
 printf("decoding...\n");
 xtea(-8);
 printf("%8x%8x\n",v[0],v[1]);

 v[0]=0x19370001;
 v[1]=0x00000000;
 
 printf("%8x%8x\n",v[0],v[1]);
 printf("encoding...\n");
 xtea(8);
 printf("%8x%8x\n",v[0],v[1]);
 printf("decoding...\n");
 xtea(-8);
 printf("%8x%8x\n",v[0],v[1]);
 
}

main()
{
char t,s[20];
char i;
char znak[]="SQ9IWS";

 k[0]=0x00000000;
 k[1]=0x00000000;
 k[2]=0x00000000;
 k[3]=0x00000000; /* keys redacted */
 
 v[0]=0;
 v[1]=0;
 for(i=0;i<4;i++)
 {
  v[0]=v[0]<<8;
  v[0]+=znak[i];
 }
 v[1]=256*znak[4]+znak[5];
 xtea(33);
 
 printf("Dla znaku [%s]:%08x,%08x\n",znak,v[0],v[1]);
 /*
 k[0]=0x00000000;
 k[1]=0x00000000;
 k[2]=0x00000000;
 k[3]=0x00001234;
 scanf("%s",s);
 printf("%s=\n",s);
 v[0]=0;
 v[1]=0;
 for(i=0;i<8;i++)
 {
   v[0]=v[0]<<4;
   v[0]+=s[i]-'0';
   printf("%d:  %x  %d\n",i,v[0],s[i]-'0');
 }
 printf("%x\n",v[0]);
 xtea(4);
 printf("%08x\n",v[0]);
*/
}
