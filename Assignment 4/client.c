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

void sendMessage(int sockfd, Packet* packet, struct addrinfo* server, const char* file_name) {
	if(packet == NULL) {
		PacketHeader* header = buildHeader(0, 0, sizeof(PacketHeader) + sizeof(Packet));
		header->syn_bit = 1;
		packet->header = header;
		strncpy(packet->data, file_name, MAX_BUFFER);
	}

	char buffer[sizeof(Packet)];
	serializePacket(packet, buffer);

	int sendto_rv = sendto(sockfd, buffer, sizeof(Packet), 0, server->ai_addr, server->ai_addrlen);

	if(sendto_rv < 0) {
        fprintf(stderr, "ERROR: Could not send data. %s\n", strerror(errno));
        exit(1);
    }
}

int checkPacket(Packet* sent, Packet* recv) {
	if(recv->header->sequence_number == (sent->header->sequence_number + 1) &&
	   recv->header->ack_number == sent->header->sequence_number) {
		   return 0;
	   }
	   else {
		   return 1;
	   }
}

Packet* receiveMessage(int sockfd, struct addrinfo* server) {
	char buffer[sizeof(Packet)];
	int recvfrom_rv = recvfrom(sockfd, &buffer, sizeof(Packet), 0, server->ai_addr, &server->ai_addrlen);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }

	return extractPacket(buffer);
}

int setAudioData(Packet* packet, AudioInfo* info) {
	info = extractInfo(packet->data);
	int write_fd = aud_writeinit(info->sample_rate, info->sample_size, info->channels);
	if (write_fd < 0){
		printf("error: unable to open audio output.\n");
		exit(1);
	}

	return write_fd;
}

int main (int argc, char *argv [])
{
	int server_fd, audio_fd;
	client_filterfunc pfunc;
	char buffer[BUFSIZE];
	struct addrinfo hints, *server;
	fd_set read_set;
	AudioInfo* audioinfo = {0, 0, 0};
	Packet* send_packet, *recv_packet;

	FD_ZERO(&read_set);

	struct timeval timeout = {3, 0};


	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	const char* hostname = argv[1];

	int getaddrinfo_rv = getaddrinfo(hostname, "1234", &hints, &server);
	if(getaddrinfo_rv != 0) {
		fprintf(stderr, "Error: Hostname unresolved %s\n", gai_strerror(getaddrinfo_rv));
		return 1;
    }

	server_fd = setupSocket(server);

	do {
		sendMessage(server_fd, send_packet, server, argv[2]);

	FD_SET(server_fd, &read_set);
	int sync_rv = syncWithServer(server_fd, &read_set, &timeout);
	if(sync_rv == 1) {
		printf("The packet was lost.\n");
	}
	else if(FD_ISSET(server_fd, &read_set) && sync_rv == 0){
		recv_packet = receiveMessage(server_fd, server);
		if(recv_packet->header->syn_bit == 1) { // open output
			audio_fd = setAudioData(recv_packet, audioinfo);
		}

		int check = checkPacket(send_packet, recv_packet);

		if(check == 1) {
			sendMessage(server_fd, send_packet, server, argv[1]);
		}
		else {
			write(audio_fd, recv_packet->data, sizeof(recv_packet->data));
			send_packet->header->sequence_number++;
			send_packet->header->ack_number = recv_packet->header->sequence_number;
		}
	}

	}while(!breakloop);

	

	

	printf ("SysProg2006 network client\n");
	printf ("handed in by VOORBEELDSTUDENT\n");
	
	signal( SIGINT, sigint_handler );	// trap Ctrl^C signals
	
	// parse arguments
	if (argc < 3){
		printf ("error : called with incorrect number of parameters\nusage : %s <server_name/IP> <filename> [<filter> [filter_options]]]\n", argv[0]) ;
		return -1;
	}
	
	
	
	// open the library on the clientside if one is requested
	if (argv[3] && strcmp(argv[3],"")){
		// try to open the library, if one is requested
		pfunc = NULL;
		if (!pfunc){
			printf("failed to open the requested library. breaking hard\n");
			return -1;
		}
		printf("opened libraryfile %s\n",argv[3]);
	}
	else{
		pfunc = NULL;
		printf("not using a filter\n");
	}
	
	// start receiving data
	{
		int bytesread, bytesmod;
		char *modbuffer;
		
		bytesread = read(server_fd, buffer, BUFSIZE);
		while (bytesread > 0){
			// edit data in-place. Not necessarily the best option
			if (pfunc)
				modbuffer = pfunc(buffer,bytesread,&bytesmod); 
			write(audio_fd, modbuffer, bytesmod);
			bytesread = read(server_fd, buffer, BUFSIZE);
		}
	}

	freeaddrinfo(server);

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

