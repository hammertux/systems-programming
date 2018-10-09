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
#include "util.h"


int syncWithClient(int sockfd, fd_set* sync, struct timeval* timeout) {
    int select_rv = select(sockfd + 1, sync, NULL, NULL, timeout);
    if(select_rv < 0) {
        fprintf(stderr, "ERROR: Could not sync fds. %s\n", strerror(errno));
        return 1;
    }
    else if(select_rv == 0) {
        fprintf(stderr, "ERROR: A timeout has occured. %s\n", strerror(errno));
        return 1;
    }
    else {
        return 0;
    }
}

int setupSocket(struct sockaddr_in6* server) {
    
    int sockfd = socket(AF_INET6, SOCK_DGRAM, IP_PROTOCOL);
    if(sockfd < 0) {
        fprintf(stderr, "ERROR: Could not create socket. %s\n", strerror(errno));
        exit(1);
    }
    #ifdef V6ONLY //Allows IPv6-to-IPv4 mapping in kernel
        int remove_ipv6only = 0;
        setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &remove_ipv6only, sizeof(remove_ipv6only));
    #endif
    server->sin6_family = AF_INET6;
    server->sin6_port = htons(SERVER_PORT);
    server->sin6_addr = in6addr_any;

    int bind_rv = bind(sockfd, (struct sockaddr* )server, sizeof(*server));
    if(bind_rv < 0) {
        int close_rv = close(sockfd);
        if(close_rv < 0) {
            fprintf(stderr, "ERROR: Could not close the socket. %s\n", strerror(errno));
            exit(1);
        }

        fprintf(stderr, "ERROR: Could not bind socket. %s\n", strerror(errno));
        exit(1);
    }

    return sockfd;
}

void sendMessage(int sockfd, char(* message)[BUFFER_SIZE], struct sockaddr_in6* client, socklen_t* length) {
    int sendto_rv = sendto(sockfd, message, sizeof(*message), 0, (struct sockaddr* )client, *length);
    if(sendto_rv < 0) {
        fprintf(stderr, "ERROR: Could not send data. %s\n", strerror(errno));
        exit(1);
    }
}

void recieveMessage(int sockfd, char(* message)[BUFFER_SIZE], struct sockaddr_in6* client, socklen_t* length) {
    int recvfrom_rv = recvfrom(sockfd, message, sizeof(*message), 0, (struct sockaddr* )client, length);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }
}


int main(int argc, char** argv) {

    if(argc != 1) {
        fprintf(stderr, "Usage: ./pingserver\n");
        return 1;
    }

    int processing = 1;
    char buffer[BUFFER_SIZE];
    fd_set read_set;
    struct timeval timeout;

    struct sockaddr_in6 client, server;
    socklen_t from_len = sizeof(client);

    memset(&server, 0, sizeof(server));//Init server sockaddrin_6 struct

    FD_ZERO(&read_set);

    timeout.tv_sec = 2;
    timeout.tv_usec = 500000;

    int sockfd = setupSocket(&server);

    FD_SET(sockfd, &read_set);
    int sync_rv = syncWithClient(sockfd, &read_set, &timeout);

    do {
        if(FD_ISSET(sockfd, &read_set) && sync_rv == 0) {
            recieveMessage(sockfd, &buffer, &client, &from_len);
            sendMessage(sockfd, &buffer, &client, &from_len);
            memset(buffer, 0, sizeof(buffer));//Re-init buffer
        }
        else {
            processing = 0;
        }

    }while(processing);

    int close_rv = close(sockfd);
    if(close_rv < 0) {
        fprintf(stderr, "ERROR: Could not close the socket. %s\n", strerror(errno));
        return 1;
    }

    return 0;

}