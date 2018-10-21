#ifndef _VOLUME_LIB_H_
#define _VOLUME_LIB_H_

#include <inttypes.h>
#include <sys/types.h>
#include <stdio.h>

void increaseVolume(char* buffer, uint8_t percent, size_t bufflen);

void decreaseVolume(char* buffer, uint8_t percent, size_t bufflen);




#endif /* _VOLUME_LIB_H_ */