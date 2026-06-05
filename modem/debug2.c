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
    printf("Modulated %zu samples for single A byte\n", n);
    size_t spb=40;
    printf("Tones: ");
    for(size_t i=0;i+spb<=n && i<spb*32;i+=spb){
        int c=0;for(size_t j=1;j<spb;j++) if((a[i+j]^a[i+j-1])<0)c++;
        printf("%d",c>2?0:1);
    }
    printf("\nDecoded: ");
    int last=1;uint8_t byte=0;int bc=0;
    for(size_t i=0;i+spb<=n && i<spb*32;i+=spb){
        int c=0;for(size_t j=1;j<spb;j++) if((a[i+j]^a[i+j-1])<0)c++;
        int raw=c>2?0:1;
        int dec=raw!=last?0:1;last=raw;
        byte=(uint8_t)((byte>>1)|(dec<<7));bc++;
        if(bc==8){printf(" %02x",byte);bc=0;byte=0;}
    }
    printf("\nExpected first bytes: 7e 41 ...\n");
    free(a);return 0;
}
