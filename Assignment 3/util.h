#ifndef _UTIL_H_
#define _UTIL_H_


#ifdef DEBUG
#define dprint printf
#else
#define dprint (void)
#endif

#define BUFFER_SIZE 64
#define SERVER_PORT 1234
#define STR_SERVER_PORT "1234" //for getaddrinfo
#define IP_PROTOCOL 0





#endif /* _UTIL_H_ */