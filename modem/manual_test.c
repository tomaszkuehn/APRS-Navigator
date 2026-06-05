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
    
    size_t spb=40;
    /* Manual decode with fixed threshold */
    printf("Threshold=2: ");
    int last=1; uint8_t byte=0; int bc=0; int fcount=0;
    uint8_t fbuf[512]; size_t flen=0; int inframe=0;
    for(size_t i=0;i+spb<=n;i+=spb){
        int cross=0;for(size_t j=1;j<spb;j++) if((a[i+j]^a[i+j-1])<0) cross++;
        int raw=cross>=3?0:1;
        int dec=raw!=last?0:1; last=raw;
        byte=(uint8_t)((byte>>1)|(dec<<7)); bc++;
        if(bc==8){bc=0;
            if(byte==0x7E){
                if(!inframe){inframe=1;flen=0;}
                else if(flen>2){
                    uint16_t c1=ax25_crc(fbuf,flen-2);
                    uint16_t c2=(uint16_t)fbuf[flen-2]|((uint16_t)fbuf[flen-1]<<8);
                    if(c1==c2){fcount++;printf("[FRAME %d: %zu bytes] ",fcount,flen-2);}
                    inframe=0;flen=0;
                }
            }else if(inframe&&flen<512){fbuf[flen++]=byte;}
            printf("%02x ",byte); byte=0;
        }
    }
    printf("\nFrames found: %d (manual decode)\n",fcount);
    
    /* Now full demod */
    int found=afsk_demod_buffer(a,n,NULL,NULL);
    printf("Full demod found: %d frames\n",found);
    free(a);return 0;
}
