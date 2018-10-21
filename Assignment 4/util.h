#ifndef _UTIL_H_
#define _UTIL_H_

#include <inttypes.h>
#include <sys/types.h>

#define MAX_BUFFER 1024
#define SERVER_PORT 1234
#define STR_SERVER_PORT "1234"
#define IP_PROTOCOL 0

typedef struct __attribute__((packed)) _audioinfo {
    int32_t channels;
    int32_t sample_size;
    int32_t sample_rate;
}AudioInfo;

//Library typedefs
typedef void (*speedfilter)(AudioInfo* info, uint8_t perc);
typedef void (*monoHeaderFilter)(AudioInfo* info);
typedef void (*monoFilter)(char* buffer, size_t bufflen);
typedef void (*volumefilter)(char* buffer, uint8_t perc, size_t bufflen);






#endif /* _UTIL_H_ */