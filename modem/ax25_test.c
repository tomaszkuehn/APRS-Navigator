#include "modem.h"
#include <stdio.h>
#include <string.h>
int main() {
    const char* text = "SP3ABC-7>APRS,WIDE1-1:!5210.12N/01655.22E-Modem test";
    uint8_t wire[512]; size_t len = ax25_encode(text, wire, sizeof(wire));
    printf("AX.25 encoded: %zu bytes\n", len);
    printf("Hex: "); for(size_t i=0;i<len;i++) printf("%02x ",wire[i]); printf("\n");
    
    /* Check if bit stuffing is triggered */
    /* Show raw bits */
    printf("Bits: ");
    for(size_t i=0;i<len;i++) for(int b=0;b<8;b++) printf("%d",(wire[i]>>b)&1);
    printf("\n");
    
    /* Look for 5 consecutive 1s */
    int ones=0;
    for(size_t i=0;i<len;i++) {
        for(int b=0;b<8;b++) {
            int bit=(wire[i]>>b)&1;
            if(bit) ones++; else ones=0;
            if(ones>=5) printf("  5 ONES at byte %zu bit %d\n", i, b);
        }
    }
    
    /* Decode back */
    char decoded[512];
    size_t dl = ax25_decode(wire, len, decoded, sizeof(decoded));
    printf("Decoded (%zu): %s\n", dl, decoded);
    return 0;
}
