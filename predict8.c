#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN_SAMPLES 10

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: predict8 datafile\n"
            "    data file is a binary file of random data to be tested.\n");
        return 1;
    }
    FILE *datafile = fopen(argv[1], "rb");
    if(datafile == NULL) {
        fprintf(stderr, "Unable to open file %s for reading\n", argv[2]);
        return 1;
    }
    uint32_t next[256][256] = {{0,},};
    uint32_t total[256] = {0,};
    if(next == NULL) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    uint8_t prevByte = 0;
    int byte;
    float surprise = 1.0;
    uint64_t totalBytes = 0;
    uint64_t bitsOfEntropy = 0;
    while((byte = getc(datafile)) && byte != EOF) {
        if(total[prevByte] > MIN_SAMPLES && next[prevByte][byte] > 0) {
            surprise *= next[prevByte][byte]/(float)total[byte];
            while(surprise < 0.5f) {
                bitsOfEntropy++;
                surprise *= 2.0f;
            }
            totalBytes++;
        }
        next[prevByte][byte]++;
        total[prevByte]++;
        prevByte = byte;
    }
    fclose(datafile);
    printf("There seems to be at most %f bits per byte in this sample\n", (float)bitsOfEntropy/totalBytes);
    return 0;
}
