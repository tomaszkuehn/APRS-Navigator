#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
    /* Build a minimal test frame: just flags and known data */
    uint8_t test_data[] = { 0x41, 0x42, 0x43 }; /* "ABC" */
    uint16_t fcs = ax25_crc(test_data, 3);
    printf("CRC of ABC: 0x%04X\n", fcs);
    
    /* Build HDLC frame: flag + data + FCS + flag */
    uint8_t framed[256];
    size_t framed_len = 0;
    framed[framed_len++] = 0x7E; /* start flag */
    memcpy(framed + framed_len, test_data, 3); framed_len += 3;
    framed[framed_len++] = (uint8_t)(fcs & 0xFF);
    framed[framed_len++] = (uint8_t)(fcs >> 8);
    framed[framed_len++] = 0x7E; /* end flag */
    printf("Frame: %zu bytes\n", framed_len);
    
    /* Modulate */
    int16_t* audio = malloc(48000 * 10);
    size_t samples = afsk_mod_frame(framed, framed_len, audio, 48000 * 10);
    printf("Modulated: %zu samples at 48000 Hz\n", samples);
    
    /* Write WAV for inspection */
    wav_write("/tmp/loopback.wav", audio, samples, 48000);
    
    /* Demodulate */
    int count = 0;
    int frames = afsk_demod_buffer(audio, samples, NULL, NULL);
    printf("Demodulated: %d frames\n", frames);
    
    free(audio);
    return frames == 0 ? 1 : 0;
}
