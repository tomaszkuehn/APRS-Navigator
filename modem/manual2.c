#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
    uint8_t d[]={0x41}; uint16_t f=ax25_crc(d,1);
    uint8_t fr[256]; size_t fl=0;
    fr[fl++]=0x7E; memcpy(fr+fl,d,1); fl+=1;
    fr[fl++]=(uint8_t)(f&0xFF); fr[fl++]=(uint8_t)(f>>8); fr[fl++]=0x7E;
    int16_t* a=malloc(48000*10);
    size_t n=afsk_mod_frame(fr,fl,a,48000*10);
    printf("Modulated %zu samples\n",n);
    
    size_t spb=40;
    int last=1; uint8_t byte=0; int bc=0;
    uint8_t fbuf[512]; size_t flen=0; int in=0;
    int ones=0, fcount=0;
    
    /* Use threshold>=3 (3-4 crossings = space=0, 1-2 crossings = mark=1) */
    for(size_t i=0;i+spb<=n;i+=spb){
        int cross=0;for(size_t j=1;j<spb;j++) if((a[i+j]^a[i+j-1])<0) cross++;
        int raw=cross>=3?0:1;
        int dec=raw!=last?0:1; last=raw;
        
        /* Bit destuff */
        if(in==1 && ones==5 && dec==0){ones=0;continue;}
        ones=dec?ones+1:0;
        
        byte=(uint8_t)((byte>>1)|(dec<<7)); bc++;
        if(bc==8){bc=0;
            if(byte==0x7E){
                if(!in){in=1;flen=0;ones=0;printf("[FLAG] ");}
                else if(flen>2){
                    uint16_t c1=ax25_crc(fbuf,flen-2);
                    uint16_t c2=(uint16_t)fbuf[flen-2]|((uint16_t)fbuf[flen-1]<<8);
                    printf("[END flen=%zu crc=%04x/%04x %s] ",flen-2,c1,c2,c1==c2?"OK":"FAIL");
                    if(c1==c2){fcount++;printf("DATA: ");for(size_t k=0;k<flen-2;k++)printf("%c",fbuf[k]);}
                    in=0;flen=0;
                }
            }else if(in&&flen<512){fbuf[flen++]=byte;printf("%02x ",byte);}
            byte=0;
        }
    }
    printf("\nFrames found: %d\n",fcount);
    free(a);return fcount==0;
}
