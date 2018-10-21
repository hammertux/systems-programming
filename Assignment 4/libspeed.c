#include "speedlib.h"
#include <stdio.h>

void increaseSpeed(AudioInfo* info, uint8_t percent) {
    double multiplier = (double) 1 + ((double)percent/100);
    printf("[INFO] Speed Increase Multiplier = %f\n", multiplier);
    info->sample_rate *= multiplier;
}

void decreaseSpeed(AudioInfo* info, uint8_t percent) {
    double multiplier = (double) 1 + ((double)percent/100);
    printf("[INFO] Speed Decrease Multiplier = %f\n", multiplier);
    info->sample_rate /= multiplier;
}