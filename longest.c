#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE ((uint64_t)1 << 22)

struct EntrySt {
    uint64_t value;
    uint32_t count;
};

static struct EntrySt *hashTable;

static inline uint64_t hashValue(uint64_t value) {
    uint64_t mask = HASH_TABLE_SIZE - 1;
    value *= 1830293841;
    value ^= 20947602394;
    value *= 309574038479;
    value ^= 20956749875;
    value *= 409856720348563;
    return value & mask;
}

static uint32_t addToHashTable(uint64_t value) {
    uint64_t hash = hashValue(value);
    uint32_t locationsChecked = 0;
    while(hashTable[hash].value != value && hashTable[hash].count != 0) {
        hash++;
        if(hash == HASH_TABLE_SIZE) {
            hash = 0;
        }
        locationsChecked++;
        if(locationsChecked > 1024) {
            fprintf(stderr, "Hash table full.  Increase HASH_TABLE_SIZE\n");
            exit(1);
        }
    }
    hashTable[hash].value = value;
    hashTable[hash].count++;
    return hashTable[hash].count;
}

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: longest N datafile\n"
            "    N is the number of bytes to look for as exact matches.\n"
            "    datafile is a binary file of random data to be tested.\n");
        return 1;
    }
    uint8_t N = atoi(argv[1]);
    if(N < 1 || N > 8) {
        fprintf(stderr, "N must be from 1 to 8.\n");
        return 1;
    }
    hashTable = calloc(HASH_TABLE_SIZE, sizeof(struct EntrySt));
    FILE *datafile = fopen(argv[2], "rb");
    if(datafile == NULL) {
        fprintf(stderr, "Unable to open file %s for reading\n", argv[2]);
        return 1;
    }
    uint64_t value = 0;
    uint64_t maxValue = 0;
    uint32_t maxCount = 0;
    uint64_t mask;
    if(N < 8) {
        mask = ((uint64_t)1 << 8*N) - 1;
    } else {
        mask = ~(uint64_t)0;
    }
    int c;
    while((c = getc(datafile)) != EOF) {
        value = ((value << 8) | c) & mask;
        uint32_t count = addToHashTable(value);
        if(count > maxCount) {
            maxCount = count;
            maxValue = value;
        }
    }
    printf("Max count %u, maxValue 0x%lx\n", maxCount, maxValue);
    fclose(datafile);
    return 0;
}
