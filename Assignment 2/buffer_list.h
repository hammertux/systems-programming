#ifndef _H_BUFFER_LIST_H_
#define _H_BUFFER_LIST_H_

#define BUF_MAX_SIZE 1024



typedef struct BufferList {
    char* byte_buffer;
    struct BufferList* previous_buffer;
    int bytes_read;
}BufferList;

BufferList* initBuffer(BufferList* prev_buf, int bytes);

void printReverseBuffer(BufferList* buffer);

BufferList* switchBuffer(BufferList* buffer);

void destroyList(BufferList* buffer);

#endif /* _H_BUFFER_LIST_H_*/