/*
This program attempts to measure the entropy in an input stream of bits.  It
assumes there is only one true state variable that gets updated with some noise
and a nonlinear function between generation of bits.  When this model is true,
bits close to each other are more correlated than bits far apart.  Regardless
of the nonlinear function used to update the state, this algorithm should do a
reasonable job of guestimating the entropy in the stream, so long as it
conforms well to this simple model.

Random number generators that should conform reasonably to this model include
ring-oscillators, zener noise, which are two of the most common, as well as
infinite entropy multiplier based TRNGs, which are currently rare.  TRNGs
should consider using this algorithm as a halth monitor to track the health of
the entropy source over time.

Having only one state variable in the model is a severe limitation that makes
this unsuitable for estimating entropy in other TRNG types.  For example,
feeding the bit stream from an analog-to-digital converter into this algorithm
will not work out, as there is additional state not modeled, which is which bit
is being shifted in out of each byte.

*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define INM_MIN_DATA 80000
#define INM_MIN_SAMPLE_SIZE 100
#define INM_MAX_SEQUENCE 20
#define INM_MAX_COUNT (1 << 14)

static uint8_t inmN;
static uint32_t inmPrevBits;
static uint32_t inmNumBitsSampled;
static uint32_t *inmOnes, *inmZeros;
// The total probability of generating the string of states we did is
// 1/(2^inmNumBitsOfEntropy * inmCurrentProbability).
static uint32_t inmNumBitsOfEntropy;
static double inmCurrentProbability;
static uint64_t inmTotalBits;
static bool inmPrevBit;
static uint32_t inmTotalOnes, inmTotalZeros;
static bool inmDebug;

// Print the tables of statistics.
void inmDumpStats(void) {
    uint32_t i;
    uint32_t one = 1;
    for(i = 0; i < one << inmN; i++) {
        printf("%x ones:%u zeros:%u\n",
            i, inmOnes[i], inmZeros[i]);
    }
}

// Once we have enough samples, we know that entropyPerBit = log(K)/log(2), so
// K must be 2^entryopPerBit.
double inmHealthCheckEstimateEntropyPerBit(void) {
    return (double)inmNumBitsOfEntropy/inmNumBitsSampled;
}

// Free memory used by the health check.
void inmHealthCheckStop(void) {
    if(inmOnes != NULL) {
        free(inmOnes);
    }
    if(inmZeros != NULL) {
        free(inmZeros);
    }
}

// Reset the statistics.
static void resetStats(void) {
    inmNumBitsSampled = 0;
    inmCurrentProbability = 1.0;
    inmNumBitsOfEntropy = 0;
    inmTotalOnes = 0;
    inmTotalZeros = 0;
}

// Initialize the health check.  N is the number of bits used to predict the next bit.
// At least 8 bits must be used, and no more than 30.  In general, we should use bits
// large enough so that INM output will be uncorrelated with bits N samples back in time.
bool inmHealthCheckStart(uint8_t N, bool debug) {
    if(N < 1 || N > 30) {
        return false;
    }
    inmDebug = debug;
    inmNumBitsOfEntropy = 0;
    inmCurrentProbability = 1.0;
    inmN = N;
    inmPrevBits = 0;
    inmOnes = calloc(1u << N, sizeof(uint32_t));
    inmZeros = calloc(1u << N, sizeof(uint32_t));
    inmTotalBits = 0;
    inmPrevBit = false;
    resetStats();
    if(inmOnes == NULL || inmZeros == NULL) {
        inmHealthCheckStop();
        return false;
    }
    return true;
}

// If running continuously, it is possible to start overflowing the 32-bit counters for
// zeros and ones.  Check for this, and scale the stats if needed.
static void scaleStats(void) {
    uint32_t i;
    for(i = 0; i < (1u << inmN); i++) {
        inmZeros[i] >>= 1;
        inmOnes[i] >>= 1;
    }
}

// If running continuously, it is possible to start overflowing the 32-bit counters for
// zeros and ones.  Check for this, and scale the stats if needed.
static void scaleEntropy(void) {
    if(inmNumBitsSampled == INM_MIN_DATA) {
        inmNumBitsOfEntropy >>= 1;
        inmNumBitsSampled >>= 1;
    }
}

// If running continuously, it is possible to start overflowing the 32-bit counters for
// zeros and ones.  Check for this, and scale the stats if needed.
static void scaleZeroOneCounts(void) {
    uint64_t maxVal = inmTotalZeros >= inmTotalOnes? inmTotalZeros : inmTotalOnes;
    if(maxVal == INM_MIN_DATA) {
        inmTotalZeros >>= 1;
        inmTotalOnes >>= 1;
    }
}

// This should be called for each bit generated.
bool inmHealthCheckAddBit(bool bit) {
    inmTotalBits++;
    if(inmDebug && (inmTotalBits & 0xfffffll) == 0) {
        fprintf(stderr, "Generated %llu bits.  Estimated entropy per bit: %f\n",
            (long long)inmTotalBits, inmHealthCheckEstimateEntropyPerBit());
        fprintf(stderr, "num1s:%f%%\n", inmTotalOnes*100.0/(inmTotalZeros + inmTotalOnes));
        fflush(stderr);
    }
    inmPrevBits = (inmPrevBits << 1) & ((1 << inmN)-1);
    if(inmPrevBit) {
        inmPrevBits |= 1;
    }
    inmPrevBit = bit;
    if(inmNumBitsSampled > 100) {
        if(bit) {
            inmTotalOnes++;
        } else {
            inmTotalZeros++;
        }
    }
    uint32_t zeros = inmZeros[inmPrevBits];
    uint32_t ones = inmOnes[inmPrevBits];
    uint32_t total = zeros + ones;
    if(bit) {
        if(ones != 0) {
            inmCurrentProbability *= (double)ones/total;
        }
    } else {
        if(zeros != 0) {
            inmCurrentProbability *= (double)zeros/total;
        }
    }
    while(inmCurrentProbability <= 0.5) {
        inmCurrentProbability *= 2.0;
        inmNumBitsOfEntropy++;
    }
    //printf("probability:%f\n", inmCurrentProbability);
    inmNumBitsSampled++;
    if(bit) {
        inmOnes[inmPrevBits]++;
        if(inmOnes[inmPrevBits] == INM_MAX_COUNT) {
            scaleStats();
        }
    } else {
        inmZeros[inmPrevBits]++;
        if(inmZeros[inmPrevBits] == INM_MAX_COUNT) {
            scaleStats();
        }
    }
    scaleEntropy();
    scaleZeroOneCounts();
    return true;
}

// Once we have enough samples, we know that entropyPerBit = log(K)/log(2), so
// K must be 2^entryopPerBit.
double inmHealthCheckEstimateK(void) {
    double entropyPerBit = (double)inmNumBitsOfEntropy/inmNumBitsSampled;
    return pow(2.0, entropyPerBit);
}

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: predict N datafile\n"
            "    N is the number of bits to use in predicting the next bit.\n"
            "    datafile is a binary file of random data to be tested.\n"
            "        Bits are shifted in MSB to LSB in each byte.\n");
        return 1;
    }
    uint8_t N = atoi(argv[1]);
    if(N < 1 || N > 30) {
        fprintf(stderr, "N must be from 1 to 30.\n");
        return 1;
    }
    FILE *datafile = fopen(argv[2], "rb");
    if(datafile == NULL) {
        fprintf(stderr, "Unable to open file %s for reading\n", argv[2]);
        return 1;
    }
    //double K = sqrt(2.0);
    inmHealthCheckStart(N, false);
    int c;
    while((c = getc(datafile)) != EOF) {
        uint32_t i;
        for(i = 0; i < 8; i++) {
            bool bit = c & (1 << (7-i))? true : false;
            inmHealthCheckAddBit(bit);
        }
    }
    printf("Estimated entropy per bit: %f, estimated K: %f\n", inmHealthCheckEstimateEntropyPerBit(),
        inmHealthCheckEstimateK());
    //inmDumpStats();
    inmHealthCheckStop();
    fclose(datafile);
    return 0;
}
