#include "monolib.h"



void convertDataToMono(char* buffer, size_t bufflen) {
    int i = 0;
    for(i = 0; i < bufflen; i++) {//to mono
        if(i % 2 == 0) {
            buffer[i] = (buffer[i] + buffer[i + 1]) / 2;
        }
    }
}

void adjustHeaderToMono(AudioInfo* info) {
    info->channels = 1;
    info->sample_rate *= 2;
    printf("[INFO] Changed to 1 Channel\n");
}