#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"


int setupSocket(struct addrinfo* server) {
    int sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if(sockfd < 0) {
        fprintf(stderr, "ERROR: Could not create socket. %s\n", strerror(errno));
        exit(1);
    }

    return sockfd;
}

void getCurrentTime(struct timeval* day_time) {
    int clock_rv = gettimeofday(day_time, NULL);
    if(clock_rv < 0) {
        fprintf(stderr, "Could not get time%s\n", strerror(errno));
        exit(1);
    }

}

void sendMessage(int sockfd, char(* message)[BUFFER_SIZE], struct addrinfo* server) {
    int sendto_rv = sendto(sockfd, message, sizeof(*message), 0, server->ai_addr, server->ai_addrlen);
    if(sendto_rv < 0) {
        fprintf(stderr, "ERROR: Could not send data. %s\n", strerror(errno));
        exit(1);
    }
}

void recieveMessage(int sockfd, char(* message)[BUFFER_SIZE], struct addrinfo* server) {
    int recvfrom_rv = recvfrom(sockfd, message, sizeof(*message), 0, server->ai_addr, &server->ai_addrlen);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }
}

float inline getTime(struct timeval start, struct timeval end) {
    return (float) (((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec) - start.tv_usec) / 1000000;
}


int main(int argc, char** argv) {

    if(argc != 2) {
        printf("Usage: ./pingclient1 <server_host_name>");
        return 1;
    }

    struct addrinfo hints, *server;
    int getaddrinfo_rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;//Accepts both IPv4 and IPv6
    hints.ai_socktype = SOCK_DGRAM;
    

    const char* hostname = argv[1];
    char buffer[BUFFER_SIZE] = "Hello World!";
    char recv_buffer[BUFFER_SIZE];

    struct timeval start_time, end_time;

    getaddrinfo_rv = getaddrinfo(hostname, STR_SERVER_PORT, &hints, &server);

    if(getaddrinfo_rv != 0) {
        fprintf(stderr, "Error: Hostname unresolved %s\n", gai_strerror(getaddrinfo_rv));
        return 1;
    }

    int sockfd = setupSocket(server);

    getCurrentTime(&start_time);
    sendMessage(sockfd, &buffer, server);
    recieveMessage(sockfd, &recv_buffer, server);
    getCurrentTime(&end_time);
    
    float total_time = getTime(start_time, end_time);
    printf("The RTT was: %f seconds.\n", total_time);

    freeaddrinfo(server);

    int close_rv = close(sockfd);
    if(close_rv < 0) {
        fprintf(stderr, "Error: Could not close the socket\n");
        return 1;
    }

    return 0;
}