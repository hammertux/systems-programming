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
    memcpy(&packet->data, buffer, sizeof(buffer));
    
    return packet;
}

char* serializePacket(Packet* packet) {
    int byte_offset = 0;
    char header_buffer[sizeof(PacketHeader)];
    char packet_buffer[sizeof(Packet)];

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

    memcpy((packet_buffer + byte_offset), packet->data, sizeof(packet->data));
    byte_offset += sizeof(packet->data);

    return packet_buffer;
}


