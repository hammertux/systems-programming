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

int syncWithServer(int sockfd, fd_set* sync, struct timeval* timeout) {
    int select_rv = select(sockfd + 1, sync, NULL, NULL, timeout);
    if(select_rv < 0) {
        fprintf(stderr, "ERROR: Could not sync fds. %s\n", strerror(errno));
        return 1;
    }
    else if(select_rv == 0) {//Timeout
        return 1;
    }
    else {
        return 0;
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

float inline getTotalTime(struct timeval start, struct timeval end) {
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
    hints.ai_family = AF_UNSPEC;//Handles both IPv6 and IPv4
    hints.ai_socktype = SOCK_DGRAM;

    const char* hostname = argv[1];
    char buffer[BUFFER_SIZE] = "Hello World!";
    char recv_buffer[BUFFER_SIZE];
    fd_set read_set;

    FD_ZERO(&read_set);

    struct timeval timeout, start_time, end_time;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    getaddrinfo_rv = getaddrinfo(hostname, STR_SERVER_PORT, &hints, &server);

    if(getaddrinfo_rv != 0) {
        fprintf(stderr, "Error: Hostname unresolved %s\n", gai_strerror(getaddrinfo_rv));
        return 1;
    }

    int sockfd = setupSocket(server);

    getCurrentTime(&start_time);
    sendMessage(sockfd, &buffer, server);

    FD_SET(sockfd, &read_set);
    int sync_rv = syncWithServer(sockfd, &read_set, &timeout);

    if(sync_rv == 1) {
        printf("The packet was lost.\n");
    }
    else if(FD_ISSET(sockfd, &read_set) && sync_rv == 0) {
        recieveMessage(sockfd, &recv_buffer, server);
        getCurrentTime(&end_time);

        float total_time = getTotalTime(start_time, end_time);
        printf("The RTT was: %f seconds.\n", total_time);
    }

    FD_CLR(sockfd, &read_set);

    freeaddrinfo(server);

    int close_rv = close(sockfd);
    if(close_rv < 0) {
        fprintf(stderr, "Error: Could not close the socket\n");
        return 1;
    }

    return 0;
}