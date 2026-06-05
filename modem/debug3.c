#include "modem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main() {
    uint8_t d[]={0x41}; uint16_t f=ax25_crc(d,1);
    printf("CRC of 0x41: 0x%04X\n", f);
    uint8_t framed[256]; size_t fl=0;
    framed[fl++]=0x7E; memcpy(framed+fl,d,1); fl+=1;
    framed[fl++]=(uint8_t)(f&0xFF); framed[fl++]=(uint8_t)(f>>8); framed[fl++]=0x7E;
    
    /* Show what gets stuffed */
    uint8_t inner[256]; size_t ilen=fl-2; /* exclude outer flags */
    memcpy(inner, framed+1, ilen);
    printf("Inner data (%zu bytes): ", ilen);
    for(size_t i=0;i<ilen;i++) printf("%02x ",inner[i]);
    
    /* Stuff */
    uint8_t stuffed[256]; memset(stuffed,0,256);
    uint8_t* sp = stuffed;
    int ones=0, outbits=0;
    for(size_t i=0;i<ilen;i++) {
        for(int b=0;b<8;b++) {
            int bit=(inner[i]>>b)&1;
            if(outbits/8>=256)break;
            if(bit) { stuffed[outbits/8]|=(1<<(outbits%8)); ones++; }
            else { stuffed[outbits/8]&=~(1<<(outbits%8)); ones=0; }
            outbits++;
            if(ones==5) { /* insert 0 */ stuffed[outbits/8]&=~(1<<(outbits%8)); outbits++; ones=0; }
        }
    }
    size_t sbytes=(size_t)(outbits+7)/8;
    printf("Stuffed (%zu bits = %zu bytes): ", outbits, sbytes);
    for(size_t i=0;i<sbytes;i++) printf("%02x ",stuffed[i]);
    printf("\n");

    /* Full HDLC */
    uint8_t hdlc[512]; size_t hp=0;
    hdlc[hp++]=0x7E;
    memcpy(hdlc+hp,stuffed,sbytes); hp+=sbytes;
    hdlc[hp++]=0x7E;
    
    /* Show tones */
    int nrzi=1;
    printf("Tones: ");
    for(size_t i=0;i<hp;i++){
        for(int b=0;b<8;b++){
            int bit=(hdlc[i]>>b)&1;
            if(bit==0)nrzi=!nrzi;
            printf("%d",nrzi);
        }
    }
    printf("\n");
    return 0;
}
