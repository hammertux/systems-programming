#include "volumelib.h"

void increaseVolume(char* buffer, uint8_t percent, size_t bufflen) {
    double multiplier = (double) 1 + ((double)percent/100);
    int i;
    for(i = 0; i < bufflen; i++) {
        buffer[i] *= multiplier;
    }
}


void decreaseVolume(char* buffer, uint8_t percent, size_t bufflen) {
    double multiplier = (double) 1 + ((double)percent/100);
    int i;
    for(i = 0; i < bufflen; i++) {
        buffer[i] /= multiplier;
    }
}