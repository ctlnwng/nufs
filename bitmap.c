#include <stdint.h>
#include <stdio.h>
#include "bitmap.h"

// NOTE: bitmap_get and bitmap_put are from me and my partner's CH02 bitmap implementation
int
bitmap_get(void* bm, int ii) {
    return ((uint8_t*)bm)[ii / 8] & (1 << (ii & 7)) ? 1 : 0;
}

void
bitmap_put(void* bm, int ii, int vv) {
    if (vv == 0) {
        ((uint8_t*)bm)[ii / 8] &= ~(1 << (ii & 7));
    }
    else {
        ((uint8_t*)bm)[ii / 8] |= 1 << (ii & 7);
    }
} 

// NOTE: bitmap_print is from this stack overflow thread:
// https://stackoverflow.com/questions/18327439/printing-binary-representation-of-a-char-in-c
void
bitmap_print(void* bm, int size) {
    for (int ii = 0; ii < size; ii++) {
        uint8_t b = ((uint8_t*)bm)[ii];

        for (int jj = 0; jj < 8; jj++) {
            printf("%d", !!((b << jj) & 0x80));
        }
        printf("\n");
    }
}


