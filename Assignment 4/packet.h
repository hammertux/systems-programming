#ifndef _PACKET_H_
#define _PACKET_H_

#include <inttypes.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

#define MAX_BUFFER 1024
#define IAC 255 //initiate negotition for filters
#define SPEED 254 //speed filter
#define VOLUME 253 // volume filter
#define INIT_CONNECTION 1
#define END_CONNECTION 1
#define NORMAL_PACKET 0

typedef _Bool uint1_t; //for header bits

typedef struct __attribute__((packed)) _syncpacket {
    char file[MAX_BUFFER];
    char library[MAX_BUFFER];
}SyncPacket;


typedef struct __attribute__((packed)) _packet {
    uint32_t sequence_number;
    uint32_t ack_number;
    uint1_t fin_bit;
    uint16_t size;
    char data[MAX_BUFFER];
}Packet;

Packet* buildPacket(char buffer[MAX_BUFFER], uint32_t seq_num, uint32_t ack, uint16_t size);

void serializePacket(Packet* packet, char* buf);

void extractPacket(Packet* packet, char buffer[sizeof(Packet)]);

SyncPacket* initSync(const char* filename, const char* lib);

void printPacket(Packet* packet, const char* s_or_r);

AudioInfo* initInfo(int32_t size, int32_t rate, int32_t channels);

void freeAudioInfo(AudioInfo* info);

void serializeInfo(AudioInfo* info, char* buf);

AudioInfo* extractInfo(char buffer[sizeof(AudioInfo)]);


#endif /* _PACKET_H_ */