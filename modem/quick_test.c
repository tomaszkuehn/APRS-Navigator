#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
    size_t c=0; int r=0;
    int16_t* s=wav_read("/tmp/simple.wav",&c,&r);
    if(!s){printf("no wav\n");return 1;}
    size_t spb=r/1200; int last=1; uint8_t b=0; int bc=0;
    uint8_t fb[1024]; size_t fl=0; int inf=0; int ones=0;
    for(size_t i=0;i+spb<=c;i+=spb){
        int cr=0;for(size_t j=1;j<spb;j++)if((s[i+j]^s[i+j-1])<0)cr++;
        int raw=(cr>=3)?0:1;int dec=(raw!=last)?0:1;last=raw;
        if(inf&&ones==5&&dec==0){ones=0;continue;}
        ones=dec?ones+1:0;
        b=(uint8_t)((b>>1)|(dec<<7));bc++;
        if(bc==8){bc=0;
            if(b==0x7E){if(!inf){inf=1;fl=0;ones=0;}else if(fl>2){uint16_t c1=ax25_crc(fb,fl-2);uint16_t c2=(uint16_t)fb[fl-2]|((uint16_t)fb[fl-1]<<8);if(c1==c2){char t[512];size_t tl=ax25_decode(fb,fl-2,t,sizeof(t));if(tl>0){t[tl]=0;printf("OK: %s\n",t);}else printf("OK: %zu bytes\n",fl-2);}else printf("CRC FAIL %04x/%04x fl=%zu\n",c1,c2,fl-2);inf=0;fl=0;}}
            else if(inf&&fl<1024)fb[fl++]=b;b=0;
        }
    }
    int lib=afsk_demod_buffer(s,c,NULL,NULL);printf("Library: %d frames\n",lib);
    free(s);return 0;
}
