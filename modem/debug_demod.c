#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
int main() {
    uint8_t data[]={0x41,0x42,0x43};
    uint16_t fcs=ax25_crc(data,3);
    uint8_t frame[256]; size_t fl=0;
    frame[fl++]=0x7E; memcpy(frame+fl,data,3); fl+=3;
    frame[fl++]=(uint8_t)(fcs&0xFF); frame[fl++]=(uint8_t)(fcs>>8); frame[fl++]=0x7E;
    int16_t* audio=malloc(48000*10);
    size_t n=afsk_mod_frame(frame,fl,audio,48000*10);
    printf("Modulated %zu samples\n",n);
    
    /* Manual demodulate and print */
    size_t spb=40;
    printf("Bit crossings: ");
    for(size_t i=0;i+spb<=n && i<spb*64;i+=spb){
        int c=0; for(size_t j=1;j<spb;j++) if((audio[i+j]^audio[i+j-1])<0) c++;
        printf("%d ",c);
    }
    printf("\n");
    
    /* Now full demod */
    int found=afsk_demod_buffer(audio, n, NULL, NULL);
    printf("AFSK demod found: %d frames\n",found);
    
    /* Write WAV */
    wav_write("/tmp/debug.wav",audio,n,48000);
    printf("Wrote /tmp/debug.wav\n");
    free(audio);
    return 0;
}
