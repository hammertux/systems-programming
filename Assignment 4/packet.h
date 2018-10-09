#ifndef _PACKET_H_
#define _PACKET_H_

#include <inttypes.h>

#define MAX_BUFFER 64

typedef struct Packet {
    unsigned int ack_bit : 1;
    char* buffer;
    uint16_t size;
}Packet;


Packet* buildPacket(unsigned int ack, char* buf);



#endif /* _PACKET_H_ */