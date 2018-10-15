#include "packet.h"

PacketHeader* buildHeader(uint32_t seq_num, uint32_t ack, uint16_t size) { //, unsigned char* cmd) {
    PacketHeader* header = malloc(sizeof(PacketHeader));
    header->sequence_number = seq_num;
    header->ack_number = ack;
    header->size = size;
    header->syn_bit = 0;
    header->fin_bit = 0;
    //header->command = cmd;

    return header;
}

Packet* buildPacket(PacketHeader* header, char buffer[MAX_BUFFER]) {
    Packet* packet = malloc(sizeof(Packet));
    packet->header = header;
    memcpy(&packet->data, buffer, MAX_BUFFER);
    
    return packet;
}

void serializePacket(Packet* packet, char* buf) {
    int byte_offset = 0;
    char header_buffer[sizeof(PacketHeader)];
    char* packet_buffer = malloc(sizeof(Packet));

    uint32_t four_byte_data;
    uint1_t one_bit_data;
    uint16_t two_byte_data;

    two_byte_data = htons(packet->header->size);
    memcpy((header_buffer + byte_offset), &two_byte_data, sizeof(two_byte_data));
    byte_offset += sizeof(two_byte_data);

    one_bit_data = packet->header->syn_bit;
    memcpy((header_buffer + byte_offset), &one_bit_data, sizeof(one_bit_data));
    byte_offset += sizeof(one_bit_data);

    one_bit_data = packet->header->fin_bit;
    memcpy((header_buffer + byte_offset), &one_bit_data, sizeof(one_bit_data));
    byte_offset += sizeof(one_bit_data);

    four_byte_data = htonl(packet->header->sequence_number);
    memcpy((header_buffer + byte_offset), &four_byte_data, sizeof(four_byte_data));
    byte_offset += sizeof(four_byte_data);


    four_byte_data = htonl(packet->header->ack_number);
    memcpy((header_buffer + byte_offset), &four_byte_data, sizeof(four_byte_data));
    byte_offset += sizeof(four_byte_data);

    //TODO: IMPLEMENT COMMANDS TELNET STYLE THEN SERIALISE.

    byte_offset = 0;
    memcpy((packet_buffer + byte_offset), header_buffer, sizeof(header_buffer));
    byte_offset += sizeof(header_buffer);

    memcpy((packet_buffer + byte_offset), &packet->data, sizeof(packet->data));
    byte_offset += sizeof(packet->data);

    buf = packet_buffer;
}

Packet* extractPacket(char buffer[sizeof(Packet)]) {
    PacketHeader* header = malloc(sizeof(PacketHeader));
    Packet* packet = malloc(sizeof(Packet));

    uint32_t four_byte_data;
    uint1_t one_bit_data;
    uint16_t two_byte_data;
    int byte_offset = 0;

    memcpy(&two_byte_data, (buffer + byte_offset), sizeof(two_byte_data));
    header->size = ntohs(two_byte_data);
    byte_offset += sizeof(two_byte_data);

    memcpy(&one_bit_data, (buffer + byte_offset), sizeof(one_bit_data));
    header->syn_bit = one_bit_data;
    byte_offset += sizeof(one_bit_data);

    memcpy(&one_bit_data, (buffer + byte_offset), sizeof(one_bit_data));
    header->fin_bit = one_bit_data;
    byte_offset += sizeof(one_bit_data);

    memcpy(&four_byte_data, (buffer + byte_offset), sizeof(four_byte_data));
    header->sequence_number = ntohl(four_byte_data);
    byte_offset += sizeof(four_byte_data);

    memcpy(&four_byte_data, (buffer + byte_offset), sizeof(four_byte_data));
    header->ack_number = ntohl(four_byte_data);
    byte_offset += sizeof(four_byte_data);

    memcpy(packet->header, header, sizeof(*header));
    memcpy(&packet->data, (buffer + byte_offset), MAX_BUFFER);

    return packet;
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

AudioInfo* extractInfo(char buffer[sizeof(AudioInfo)]) {
    AudioInfo* info = malloc(sizeof(AudioInfo));

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

    return info;
}



void printPacket(Packet* packet) {
    printf("Header:\nSize: %d\nSYN: %d\nFIN: %d\nSEQ: %d\nACK: %d\n\n \
            Packet:\nData: %s", packet->header->size, packet->header->syn_bit, packet->header->fin_bit, packet->header->sequence_number,
            packet->header->ack_number, packet->data);
}

//
