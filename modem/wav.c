#include "modem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Write 16-bit mono PCM WAV file */
int wav_write(const char* path, const int16_t* samples, size_t count, int rate) {
    FILE* fp = fopen(path, "wb");
    if (!fp) return -1;
    wav_header_t h;
    memset(&h, 0, sizeof(h));
    memcpy(h.riff, "RIFF", 4);
    h.file_size = 36 + (uint32_t)(count * 2);
    memcpy(h.wave, "WAVE", 4);
    memcpy(h.fmt, "fmt ", 4);
    h.fmt_size = 16;
    h.audio_format = 1; /* PCM */
    h.num_channels = 1;
    h.sample_rate = (uint32_t)rate;
    h.byte_rate = (uint32_t)(rate * 2);
    h.block_align = 2;
    h.bits_per_sample = 16;
    memcpy(h.data_tag, "data", 4);
    h.data_size = (uint32_t)(count * 2);
    fwrite(&h, sizeof(h), 1, fp);
    fwrite(samples, sizeof(int16_t), count, fp);
    fclose(fp);
    return 0;
}

/* Read 16-bit mono PCM WAV file */
int16_t* wav_read(const char* path, size_t* out_count, int* out_rate) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    wav_header_t h;
    if (fread(&h, sizeof(h), 1, fp) != 1) { fclose(fp); return NULL; }
    if (memcmp(h.riff, "RIFF", 4) != 0 || memcmp(h.wave, "WAVE", 4) != 0) { fclose(fp); return NULL; }
    if (h.audio_format != 1 || h.bits_per_sample != 16) { fclose(fp); return NULL; }
    size_t count = h.data_size / 2;
    int16_t* buf = (int16_t*)malloc(h.data_size);
    if (!buf) { fclose(fp); return NULL; }
    size_t read = fread(buf, 2, count, fp);
    fclose(fp);
    *out_count = read;
    if (out_rate) *out_rate = (int)h.sample_rate;
    return buf;
}
