#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
    size_t count=0; int rate=0;
    int16_t* s = wav_read("/tmp/test_beacon.wav", &count, &rate);
    if (!s) { printf("no wav\n"); return 1; }
    size_t spb=rate/1200; int last=1; uint8_t byte=0; int bc=0;
    uint8_t buf[1024]; size_t bl=0; int inf=0; int ones=0;
    
    for (size_t i=0; i+spb <= count; i+=spb) {
        int c=0; for(size_t j=1;j<spb;j++) if((s[i+j]^s[i+j-1])<0) c++;
        int raw=(c>=3)?0:1; int dec=(raw!=last)?0:1; last=raw;
        if(inf && ones==5 && dec==0){ones=0;continue;}
        ones=dec?ones+1:0;
        byte=(uint8_t)((byte>>1)|(dec<<7)); bc++;
        if(bc==8){bc=0;
            if(byte==0x7E){
                if(!inf){inf=1;bl=0;ones=0;printf("[F] ");}
                else if(bl>2){
                    uint16_t c1=ax25_crc(buf,bl-2);
                    uint16_t c2=(uint16_t)buf[bl-2]|((uint16_t)buf[bl-1]<<8);
                    printf("[E crc=%04x/%04x %s] ",c1,c2,c1==c2?"OK":"FAIL");
                    if(c1==c2){
                        char txt[512]; size_t tl=ax25_decode(buf,bl-2,txt,sizeof(txt));
                        if(tl>0){txt[tl]=0;printf("TEXT: %s",txt);}
                    }
                    inf=0;bl=0;
                }
            }else if(inf&&bl<1024){buf[bl++]=byte;printf("%02x ",byte);}
            byte=0;
        }
    }
    printf("\n\nLibrary: "); int lib=afsk_demod_buffer(s,count,NULL,NULL);
    printf("%d frames\n",lib);
    free(s); return 0;
}
