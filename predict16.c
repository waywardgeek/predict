#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
    static uint32_t next[256][256][256] = {{{0,},},};
    static uint32_t total[256][256] = {{0,},};
    uint8_t prevByte = 0;
    uint8_t prevPrevByte = 0;
    int byte;
    float surprise = 1.0;
    uint64_t totalBytes = 0;
    uint64_t bitsOfEntropy = 0;
    while((byte = getc(datafile)) != EOF) {
        next[prevPrevByte][prevByte][byte]++;
        total[prevPrevByte][prevByte]++;
        surprise *= next[prevPrevByte][prevByte][byte]/(float)total[prevPrevByte][prevByte];
        while(surprise < 0.5f) {
            bitsOfEntropy++;
            surprise *= 2.0f;
        }
        totalBytes++;
        prevPrevByte = prevByte;
        prevByte = byte;
    }
    fclose(datafile);
    printf("There seems to be at most %f bits per byte in this sample\n", (float)bitsOfEntropy/totalBytes);
    printf("Based on %lu total samples\n", totalBytes);
    uint32_t i, j, k;
    uint32_t numBins = 0;
    uint64_t totalHits = 0;
    for(i = 0; i < 256; i++) {
        for(j = 0; j < 256; j++) {
            for(k = 0; k < 256; k++) {
                if(next[i][j][k] > 0) {
                    numBins++;
                    totalHits += next[i][j][k];
                }
            }
        }
    }
    printf("num bins: %u, average hits per bin: %.2f\n", numBins, (float)totalHits/numBins);
    return 0;
}
