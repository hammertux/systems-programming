/* client.c
 *
 * part of the Systems Programming assignment
 * (c) Vrije Universiteit Amsterdam, 2005-2015. BSD License applies
 * author  : wdb -_at-_ few.vu.nl
 * contact : arno@cs.vu.nl
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

#include "library.h"
#include "audio.h"
#include "packet.h"
#include "util.h"

#define BUFSIZE 1024

static int breakloop = 0;	///< use this variable to stop your wait-loop. Occasionally check its value, !1 signals that the program should close

/// unimportant: the signal handler. This function gets called when Ctrl^C is pressed
void sigint_handler(int sigint)
{
	if (!breakloop){
		breakloop=1;
		printf("SIGINT catched. Please wait to let the client close gracefully.\nTo close hard press Ctrl^C again.\n");
	}
	else{
       		printf ("SIGINT occurred, exiting hard... please wait\n");
		exit(-1);
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

void sendMessage(int sockfd, Packet* packet, struct addrinfo* server) {
	char buffer[sizeof(Packet)];
	serializePacket(packet, buffer);
	int sendto_rv = sendto(sockfd, &buffer, sizeof(Packet), 0, server->ai_addr, server->ai_addrlen);
	//printf("SENT: %d\n", sendto_rv);

	if(sendto_rv < 0) {
        fprintf(stderr, "ERROR: Could not send data. %s\n", strerror(errno));
        exit(1);
    }
}

int checkPacket(Packet* sent, Packet* recv) {
	if(recv->sequence_number == sent->ack_number) {
		   return 0;
	   }
	   else {
		   return 1;
	   }
}

void receiveMessage(int sockfd, struct addrinfo* server, Packet* packet) {
	char buffer[sizeof(Packet)];
	int recvfrom_rv = recvfrom(sockfd, &buffer, sizeof(Packet), 0, server->ai_addr, &server->ai_addrlen);
	extractPacket(packet, buffer);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }
}

int setAudioData(AudioInfo* info) {
	int write_fd = aud_writeinit(info->sample_rate, info->sample_size, info->channels);
	if (write_fd < 0){
		printf("error: unable to open audio output.\n");
		exit(1);
	}

	return write_fd;
}

void sendInitPacket(int sockfd, struct addrinfo* server, SyncPacket* sync) {

	int sendto_rv = sendto(sockfd, sync, sizeof(SyncPacket), 0, server->ai_addr, server->ai_addrlen);
	//printf("SENT: %d\n", sendto_rv);

	if(sendto_rv < 0) {
        fprintf(stderr, "ERROR: Could not send data. %s\n", strerror(errno));
        exit(1);
    }
}

AudioInfo* recvAudioInfo(int sockfd, struct addrinfo* server) {
	AudioInfo* info = malloc(sizeof(AudioInfo));
	char buffer[sizeof(AudioInfo)];
	
	int recvfrom_rv = recvfrom(sockfd, buffer, sizeof(AudioInfo), 0, server->ai_addr, &server->ai_addrlen);
	extractInfo(info, buffer);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }

	return info;
}

void endConnection(int sockfd, Packet* send, Packet* recv, struct addrinfo* server) {
	send->fin_bit = 1;
	sendMessage(sockfd, send, server);
	// printPacket(send, 's');
	// printf("SENT FIN\n");
	// printPacket(recv, 'r');
	printf("Server finished streaming requested file!\n");
	
}

int main (int argc, char *argv [])
{
	printf ("SysProg2006 network client\n");
	printf ("handed in by Andrea Di Dio\n");
	char* lib = "";
	char option = 'n';
	uint8_t perc = 0;
	
	signal( SIGINT, sigint_handler );// trap Ctrl^C signals
	
	// parse arguments
	if (argc < 3){
		printf ("error : called with incorrect number of parameters\nusage : %s <server_name/IP> <filename> [<filter> [filter_options]]]\n", argv[0]) ;
		return -1;
	}

	
	// open the library on the clientside if one is requested
	if (argv[3] && strcmp(argv[3],"--speed")==0){
		lib = argv[3];
		// try to open the library, if one is requested
		if(argv[4] && strcmp(argv[4], "-i") == 0) {
			option = 'i';
		}
		else if(argv[4] && strcmp(argv[4], "-d") == 0) {
			option = 'd';
		}
		if(argv[5]) {
			perc = atoi(argv[5]);
		}
	}
	else if (argv[3] && strcmp(argv[3],"--volume")==0){
		lib = argv[3];
		// try to open the library, if one is requested
		if(argv[4] && strcmp(argv[4], "-i") == 0) {
			option = 'i';
		}
		else if(argv[4] && strcmp(argv[4], "-d") == 0) {
			option = 'd';
		}
		if(argv[5]) {
			perc = atoi(argv[5]);
		}
	}
	else if(argv[3] && strcmp(argv[3],"--mono")==0) {
		lib = argv[3];
	}
	else{
		printf("not using a filter\n");
	}

	int server_fd, audio_fd;
	int write_rv;
	int starting = 1;
	
	struct addrinfo hints, *server;
	fd_set read_set;
	AudioInfo* audioinfo = NULL;
	Packet* send_packet = buildPacket(NULL, 1, 0, 0), *recv_packet = buildPacket(NULL, 0, 0, 0);
	SyncPacket* start_connection = initSync(argv[2], lib, option, perc);


	FD_ZERO(&read_set);
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 500000;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	const char* hostname = argv[1];

	int getaddrinfo_rv = getaddrinfo(hostname, STR_SERVER_PORT, &hints, &server);
	if(getaddrinfo_rv != 0) {
		fprintf(stderr, "Error: Hostname unresolved %s\n", gai_strerror(getaddrinfo_rv));
		return 1;
    }

	server_fd = setupSocket(server);

	do {
		if(starting) {
			sendInitPacket(server_fd, server, start_connection);
			//printf("----INIT----\nfilename: %s\nlibrary: %s\n", start_connection->file, start_connection->library);
		}

		FD_SET(server_fd, &read_set);
		int sync_rv = syncWithServer(server_fd, &read_set, &timeout);
		if(sync_rv == 1) {
			printf("The connection with the server was lost...\n");
			return -1;
		}
		else if(FD_ISSET(server_fd, &read_set) && sync_rv == 0){
			if(starting == 1) {
				audioinfo = recvAudioInfo(server_fd, server);
				audio_fd = setAudioData(audioinfo);
				//printf("AUDIO_FD = %d", audio_fd);
				starting = 0;
			}
			else {
				receiveMessage(server_fd, server, recv_packet);
				if(recv_packet->fin_bit == 1) {
					endConnection(server_fd, send_packet, recv_packet, server);
					
					freeaddrinfo(server);
					free(send_packet);
					free(recv_packet);
					free(start_connection);
					free(audioinfo);
					return 1;
				}
					
			}
		
			sendMessage(server_fd, send_packet, server);
				
			send_packet->sequence_number++;
			send_packet->ack_number = recv_packet->sequence_number;

			if(checkPacket(send_packet, recv_packet) == 0) {
				write_rv = write(audio_fd, recv_packet->data, recv_packet->size);
				if(write_rv < 0) {
					fprintf(stderr, "Could not write to audio fd: %s", strerror(errno));
					return 1;
				}
			}
			else {
				sendMessage(server_fd, send_packet, server);
			}
		}

	}while(!breakloop);

	freeaddrinfo(server);
	free(send_packet);
	free(recv_packet);
	free(start_connection);
	free(audioinfo);

    int close_rv = close(server_fd);
    if(close_rv < 0) {
        fprintf(stderr, "Error: Could not close the socket\n");
        return 1;
    }

	if (audio_fd >= 0)	
		close(audio_fd);
	if (server_fd >= 0)
		close(server_fd);
	
	return 0 ;
}