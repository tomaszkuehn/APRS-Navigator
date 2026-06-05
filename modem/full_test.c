#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fcount=0;

static void on_frame(const uint8_t* data, size_t len, void* user) {
    (void)user;
    fcount++;
    /* Decode AX.25 to text */
    char text[512];
    size_t tl = ax25_decode(data, len, text, sizeof(text));
    if (tl > 0) { text[tl]=0; printf("FRAME: %s\n", text); }
    else printf("FRAME: %zu bytes (decode failed)\n", len);
}

int main() {
    size_t count=0; int rate=0;
    int16_t* samples = wav_read("/tmp/test_beacon.wav", &count, &rate);
    if (!samples) { printf("Cannot read WAV\n"); return 1; }
    printf("WAV: %zu samples, %d Hz\n", count, rate);
    
    /* Manual decode with correct threshold */
    size_t spb = rate / 1200;  /* 40 at 48000 Hz */
    int last=1; uint8_t byte=0; int bc=0;
    uint8_t fbuf[1024]; size_t flen=0; int inframe=0;
    int ones=0; int found=0;
    
    for (size_t i=0; i+spb <= count; i += spb) {
        int cross=0;
        for(size_t j=1; j<spb; j++) if((samples[i+j]^samples[i+j-1])<0) cross++;
        int raw = (cross >= 3) ? 0 : 1;  /* >=3 crossings = space = 0 */
        int dec = (raw != last) ? 0 : 1;
        last = raw;
        
        /* Bit destuff */
        if (inframe && ones==5 && dec==0) { ones=0; continue; }
        ones = dec ? ones+1 : 0;
        
        byte = (uint8_t)((byte>>1) | (dec<<7));
        bc++;
        if (bc==8) { bc=0;
            if (byte == 0x7E) {
                if (!inframe) { inframe=1; flen=0; ones=0; }
                else if (flen > 2) {
                    uint16_t c1 = ax25_crc(fbuf, flen-2);
                    uint16_t c2 = (uint16_t)fbuf[flen-2] | ((uint16_t)fbuf[flen-1]<<8);
                    if (c1 == c2) {
                        found++;
                        char text[512]; size_t tl=ax25_decode(fbuf, flen-2, text, sizeof(text));
                        if(tl>0){text[tl]=0;printf("  FRAME %d: %s\n",found,text);}
                        else printf("  FRAME %d: %zu bytes\n",found,flen-2);
                    }
                    inframe=0; flen=0;
                }
            } else if (inframe && flen<1024) fbuf[flen++] = byte;
            byte=0;
        }
    }
    printf("Manual decode: %d frames\n", found);
    
    /* Also test the library function */
    int lib_found = afsk_demod_buffer(samples, count, on_frame, NULL);
    printf("Library decode: %d frames\n", lib_found);
    
    free(samples);
    return 0;
}
