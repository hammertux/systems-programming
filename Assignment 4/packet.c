#include "packet.h"

Packet* buildPacket(char buffer[MAX_BUFFER], uint32_t seq_num, uint32_t ack, uint16_t size) {
    Packet* packet = malloc(sizeof(Packet));
    packet->sequence_number = seq_num;
    packet->ack_number = ack;
    packet->size = size;
    packet->fin_bit = 0;
    
    return packet;
}

SyncPacket* initSync(const char* filename, const char* lib, char op, uint8_t perc) {
    SyncPacket* sync = malloc(sizeof(SyncPacket));
    strncpy(sync->file, filename, strlen(filename));
    strncpy(sync->library, lib, strlen(lib));
    sync->inc_or_dec = op;
    sync->percentage = perc;

    return sync;
}

void serializePacket(Packet* packet, char* buf) {
    int byte_offset = 0;
    char packet_buffer[sizeof(Packet)];

    uint32_t four_byte_data;
    uint1_t one_bit_data;
    uint16_t two_byte_data;

    two_byte_data = htons(packet->sequence_number);
    memcpy((packet_buffer + byte_offset), &two_byte_data, sizeof(two_byte_data));
    byte_offset += sizeof(two_byte_data);

    four_byte_data = htonl(packet->ack_number);
    memcpy((packet_buffer + byte_offset), &four_byte_data, sizeof(four_byte_data));
    byte_offset += sizeof(four_byte_data);

    one_bit_data = packet->fin_bit;
    memcpy((packet_buffer + byte_offset), &one_bit_data, sizeof(one_bit_data));
    byte_offset += sizeof(one_bit_data);

    four_byte_data = htonl(packet->size);
    memcpy((packet_buffer + byte_offset), &four_byte_data, sizeof(four_byte_data));
    byte_offset += sizeof(four_byte_data);

    memcpy((&packet_buffer[byte_offset]), packet->data, MAX_BUFFER);

    memcpy(buf, packet_buffer, sizeof(Packet));
}

void extractPacket(Packet* packet, char buffer[sizeof(Packet)]) {
    //Packet* packet = malloc(sizeof(Packet));

    uint32_t four_byte_data;
    uint1_t one_bit_data;
    uint16_t two_byte_data;
    int byte_offset = 0;

    memcpy(&two_byte_data, (buffer + byte_offset), sizeof(two_byte_data));
    packet->sequence_number = ntohs(two_byte_data);
    byte_offset += sizeof(two_byte_data);

    memcpy(&four_byte_data, (buffer + byte_offset), sizeof(four_byte_data));
    packet->ack_number = ntohl(four_byte_data);
    byte_offset += sizeof(four_byte_data);

    memcpy(&one_bit_data, (buffer + byte_offset), sizeof(one_bit_data));
    packet->fin_bit = one_bit_data;
    byte_offset += sizeof(one_bit_data);

    memcpy(&four_byte_data, (buffer + byte_offset), sizeof(four_byte_data));
    packet->size = ntohl(four_byte_data);
    byte_offset += sizeof(four_byte_data);

    memcpy(packet->data, (buffer + byte_offset), MAX_BUFFER);
}

AudioInfo* initInfo(int32_t size, int32_t rate, int32_t channels) {
    AudioInfo* info = (AudioInfo* ) malloc(sizeof(AudioInfo));
    info->sample_rate = rate;
    info->sample_size = size;
    info->channels = channels;

    return info;
}

void freeAudioInfo(AudioInfo* info) {
    free(info);
}


void serializeInfo(AudioInfo* info, char* buf){
    int32_t four_byte_data;
    int byte_offset = 0;

    four_byte_data = htonl(info->channels);
    memcpy(buf + byte_offset, &four_byte_data, sizeof(four_byte_data));
    byte_offset += sizeof(four_byte_data);

    four_byte_data = htonl(info->sample_rate);
    memcpy(buf + byte_offset, &four_byte_data, sizeof(four_byte_data));
    byte_offset += sizeof(four_byte_data);

    four_byte_data = htonl(info->sample_size);
    memcpy(buf + byte_offset, &four_byte_data, sizeof(four_byte_data));

}

void extractInfo(AudioInfo* info, char buffer[sizeof(AudioInfo)]) {
    //AudioInfo* info = malloc(sizeof(AudioInfo));

    int32_t four_byte_data;
    int byte_offset = 0;

    memcpy(&four_byte_data, buffer + byte_offset, sizeof(four_byte_data));
    info->channels = ntohl(four_byte_data);
    byte_offset += sizeof(four_byte_data);

    memcpy(&four_byte_data, buffer + byte_offset, sizeof(four_byte_data));
    info->sample_rate = ntohl(four_byte_data);
    byte_offset += sizeof(four_byte_data);

    memcpy(&four_byte_data, buffer + byte_offset, sizeof(four_byte_data));
    info->sample_size = ntohl(four_byte_data);

}



void printPacket(Packet* packet, char s_or_r) {
    if(s_or_r == 's') {
        printf("-------------------SENDING--------------------\n");
        printf("Header:\nSize: %d\nSEQ: %d\nACK: %d\nFIN: %d\n\nPacket:\nData: %s\n", packet->size, packet->sequence_number,
            packet->ack_number, packet->fin_bit, packet->data);
        printf("-------------------END SENDING------------------\n");
    }
    else if(s_or_r == 'r') {
        printf("-------------------RECEIVING--------------------\n");
        printf("Header:\nSize: %d\nSEQ: %d\nACK: %d\nFIN: %d\n\nPacket:\nData: %s\n", packet->size, packet->sequence_number,
            packet->ack_number, packet->fin_bit, packet->data);
        printf("-------------------END RECEIVING------------------\n");
    }
}
