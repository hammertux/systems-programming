#ifndef _MONOLIB_H_
#define _MONOLIB_H_

#include "util.h"
#include <sys/types.h>
#include <stdio.h>


void convertDataToMono(char* buffer, size_t bufflen);
void adjustHeaderToMono(AudioInfo* info);




#endif /* _MONOLIB_H_ */