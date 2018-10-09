#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "buffer_list.h"

BufferList* initBuffer(BufferList* prev_buf, int bytes) {
    BufferList* buffer = (BufferList*) malloc(sizeof(BufferList));
    if(buffer == NULL) {
        perror("Error: Could not allocate memory for the buffer linked list.\n");
        exit(1);
    }

    buffer->byte_buffer = (char*) malloc(sizeof(char) * BUF_MAX_SIZE);
    if(buffer->byte_buffer == NULL) {
        perror("Error: Could not allocate memory for the buffer.\n");
        exit(1);
    }

    buffer->previous_buffer = prev_buf;
    buffer->bytes_read = bytes;
    return buffer;
}

void destroyList(BufferList* buffer) {
    free(buffer->byte_buffer);
    free(buffer);
}

BufferList* switchBuffer(BufferList* buffer) { //to avoid leaking memory using the same buffer.
    BufferList* tmp_buffer = buffer->previous_buffer;
    destroyList(buffer);
    return tmp_buffer;
}

void printReverseBuffer(BufferList* buffer) {
    int putc_rv = 0;

    while(buffer->previous_buffer != NULL) {
        int i;
        int length = buffer->bytes_read;
        for(i = 1; i <= length; i++) {
            putc_rv = fputc(buffer->byte_buffer[length - i], stdout);
            if(putc_rv == EOF) {
                perror("ERROR: Could not print character.\n");
                exit(1);
            }
        }
        buffer = switchBuffer(buffer);
    }
    if(buffer->previous_buffer == NULL) {
        int i;
        int length = buffer->bytes_read;
        for(i = 1; i <= length; i++) {
            fputc(buffer->byte_buffer[length - i], stdout);
            if(putc_rv == EOF) {
                perror("ERROR: Could not print character.\n");
                exit(1);
            }
        }
        destroyList(buffer);
    }
}

int readFile(const char* filename) {
    int reading = 1;
    FILE* fp = fopen(filename, "r");
    if(fp == NULL) {
        perror("Error: Could not open file.\n");
        return 1;
    }
    
    BufferList* current_buffer = initBuffer(NULL, 0);

    if(current_buffer == NULL || current_buffer->byte_buffer == NULL) {
        perror("ERROR: Could not allocate memory for buffer.\n");
        return 1;
    }

    if(fp == NULL) {
        perror("ERROR: Could not open file.\n");
        return 1;
    }
    
    while(reading) {
        int bytes_read = fread(current_buffer->byte_buffer, sizeof(char), BUF_MAX_SIZE, fp);
        current_buffer->bytes_read = bytes_read;
        if(bytes_read < 0) {
            perror("ERROR: Could not read file.\n");
            return 1;
        }
        if(bytes_read < 1024) {
            reading = 0;
            break;
        }
        BufferList* new_buffer = initBuffer(current_buffer, current_buffer->bytes_read);
        current_buffer = new_buffer;
    }
    printReverseBuffer(current_buffer);

    if(fclose(fp) == EOF) {
        perror("Error: Could not close file.\n");
        return 1;
    }

    return 0;
}

int main(int argc, char** argv) {

    if(argc != 2) {
        perror("Usage: ./reverse <filename>\n");
        return 1;
    }

    int read_file_return = readFile(argv[1]);

    if(read_file_return == 1) {
        perror("ERROR: Could not read the file!\n");
        return 1;
    }

    return 0;
}