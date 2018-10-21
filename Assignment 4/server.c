/* server.c
 *
 * part of the Systems Programming assignment
 * (c) Vrije Universiteit Amsterdam, 2005-2015. BSD License applies
 * author  : wdb -_at-_ few.vu.nl
 * contact : arno@cs.vu.nl
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <signal.h>
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
#include "volumelib.h"
#include "speedlib.h"
#include "monolib.h"

#define IP_PROTOCOL 0

static int breakloop = 0;	///< use this variable to stop your wait-loop. Occasionally check its value, !1 signals that the program should close


/// unimportant: the signal handler. This function gets called when Ctrl^C is pressed
void sigint_handler(int sigint)
{
	if (!breakloop){
		breakloop=1;
		printf("SIGINT catched. Please wait to let the server close gracefully.\nTo close hard press Ctrl^C again.\n");
	}
	else{
       	printf ("SIGINT occurred, exiting hard... please wait\n");
		exit(-1);
	}
}

void gracefullyExit(int sockfd, int audio_fd) {
	close(audio_fd);
	close(sockfd);
	exit(1);
}

int syncWithClient(int sockfd, fd_set* sync, struct timeval* timeout) {
    int select_rv = select(sockfd + 1, sync, NULL, NULL, timeout);
    if(select_rv < 0 && breakloop == 1) {
        //fprintf(stderr, "ERROR: Could not sync fds. %s\n", strerror(errno));
        return 1;
    }
    else if(select_rv == 0) {
        //fprintf(stderr, "ERROR: A timeout has occured. %s\n", strerror(errno));
        return 1;
    }
    else {
        return 0;
    }
}

int setupSocket(struct sockaddr_in* server) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);
	if(sockfd < 0) {
        fprintf(stderr, "ERROR: Could not create socket. %s\n", strerror(errno));
        exit(1);
    }

	server->sin_addr.s_addr = htonl(INADDR_ANY);
	server->sin_family = AF_INET;
	server->sin_port = htons(SERVER_PORT);

	int bind_rv = bind(sockfd, (const struct sockaddr* )server, sizeof(*server));
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

void sendMessage(int sockfd, Packet* packet, struct sockaddr_in* client, socklen_t from_len) {
	char buffer[sizeof(Packet)];
	serializePacket(packet, buffer);
	int send_rv = sendto(sockfd, &buffer, sizeof(Packet), 0, (struct sockaddr* )client, from_len);
	if(send_rv < 0) {
		fprintf(stderr, "ERROR: Could not send data. %s\n", strerror(errno));
		exit(1);
	}
}

void receiveMessage(int sockfd, Packet* packet, struct sockaddr_in* client, socklen_t from_len) {
	char buffer[sizeof(Packet)];
	int recvfrom_rv = recvfrom(sockfd, &buffer, sizeof(Packet), 0, (struct sockaddr* )client, &from_len);
	extractPacket(packet, buffer);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }
}

SyncPacket* recvInitPacket(int sockfd, struct sockaddr_in* client, socklen_t from_len) {
	SyncPacket* init = malloc(sizeof(SyncPacket));
	int recvfrom_rv = recvfrom(sockfd, init, sizeof(SyncPacket), 0, (struct sockaddr* )client, &from_len);
    if(recvfrom_rv < 0) {
        fprintf(stderr, "ERROR: Could not receive data. %s\n", strerror(errno));
        exit(1);
    }

	return init;
}

void sendInfo(int sockfd, struct sockaddr_in* client, AudioInfo* info, socklen_t from_len) {
	char buffer[sizeof(AudioInfo)];
	serializeInfo(info, buffer);
	int send_rv = sendto(sockfd, buffer, sizeof(AudioInfo), 0, (struct sockaddr* )client, from_len);
	if(send_rv < 0) {
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

void resetTimeout(int sockfd, fd_set* read_set, struct timeval* timeout) {
	//Needed to avoid kernel from changing the timeout period.
	FD_ZERO(read_set);
	FD_SET(sockfd, read_set);
	timeout->tv_sec = 2;
	timeout->tv_usec = 500000;
}

int stream(int sockfd, fd_set* read_set, struct sockaddr_in* client, socklen_t from_len, SyncPacket* start_connection) {

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 500000;
	int read_rv;
	int starting = 1;
	char* libfile;
	void* library;
	char option;
	typedef void (*speedfilter)(AudioInfo* info, uint8_t perc);
	typedef void (*monoHeaderFilter)(AudioInfo* info);
	typedef void (*monoFilter)(char* buffer, size_t bufflen);
	typedef void (*volumefilter)(char* buffer, uint8_t perc, size_t bufflen);
	volumefilter increaseVol, decreaseVol;
	speedfilter increaseSp, decreaseSp;
	monoHeaderFilter convertHeader;
	monoFilter mono;
	

	FD_ZERO(read_set);
	Packet* send_packet = buildPacket(NULL, 0, 0, 0), *recv_packet = buildPacket(NULL, 0, 0, 0);
	
	send_packet->fin_bit = 0;
	recv_packet->fin_bit = 0;
	AudioInfo* info = initInfo(0, 0, 0);

	uint16_t read_fd;
	FD_SET(sockfd, read_set);
	//int sync_rv = syncWithClient(sockfd, read_set, &timeout);

	
	start_connection = recvInitPacket(sockfd, client, from_len);
	libfile = start_connection->library;
	option = start_connection->inc_or_dec;


		// optionally open a library
	if (strcmp(libfile, "--speed") == 0){
		// try to open the library, if one is requested
		printf("In Speed");
		library = dlopen("libspeed.so", RTLD_NOW);
		printf("Opened Speed");
		if(option == 'i') {
			increaseSp = dlsym(library, "increaseSpeed");
			if (!increaseSp){
				printf("failed to open the requested library. breaking hard\n");
				return -1;
			}
		}
		else if(option == 'd') {
			decreaseSp = dlsym(library, "decreaseSpeed");
			if (!decreaseSp){
				printf("failed to open the requested library. breaking hard\n");
				return -1;
			}
		} 
		printf("opened libraryfile %s\n",libfile);
	}
	else if(strcmp(libfile, "--mono") == 0) {
		library = dlopen("libmono.so", RTLD_NOW);
		convertHeader = dlsym(library, "adjustHeaderToMono");
		if (!convertHeader){
			printf("failed to open the requested library. breaking hard\n");
			return -1;
		}
		mono = dlsym(library, "convertDataToMono");
		if (!mono){
			printf("failed to open the requested library. breaking hard\n");
			return -1;
		}
		printf("opened libraryfile %s\n",libfile);
	}
	else if(strcmp(libfile, "--volume") == 0) {
		library = dlopen("libvolume.so", RTLD_NOW);
		if(option == 'i') {
			increaseVol = dlsym(library, "increaseVolume");
			if (!increaseVol){
				printf("failed to open the requested library. breaking hard\n");
				return -1;
			}
		}
		else if(option == 'd') {
			decreaseVol = dlsym(library, "decreaseVolume");
			if (!decreaseVol){
				printf("failed to open the requested library. breaking hard\n");
				return -1;
			}
		}
		printf("opened libraryfile %s\n",libfile);
	}
	else{
		printf("not using a filter\n");
	}
	
	
	read_fd = aud_readinit(start_connection->file, &info->sample_rate, &info->sample_size, &info->channels);
	if(read_fd < 0) {
		return -1;
	}
	if(increaseSp) {
		increaseSp(info, start_connection->percentage);
	}
	else if(decreaseSp) {
		decreaseSp(info, start_connection->percentage);
	}
	else if(convertHeader) {
		convertHeader(info);
	}
	sendInfo(sockfd, client, info, from_len);
	
				
	read_rv = read(read_fd, send_packet->data, MAX_BUFFER);
	//printf("READ: %d\n", read_rv);
	send_packet->size = read_rv;
	if(mono) {
		mono(send_packet->data, send_packet->size);
	}
	else if(increaseVol) {
		increaseVol(send_packet->data, start_connection->percentage, send_packet->size);
	}
	else if(decreaseVol) {
		decreaseVol(send_packet->data, start_connection->percentage, send_packet->size);
	}
	if(read_rv < 0) {
		printPacket(recv_packet, 's');
		fprintf(stderr, "Could not read from audio fd: %s", strerror(errno));
		return -1;
	}

	while(read_rv > 0) {
		int check = checkPacket(send_packet, recv_packet);
		if(check == 1) {
			printf("OOps");
			sendMessage(sockfd, send_packet, client, from_len);
		}
		else {
			if(starting == 0) {
				read_rv = read(read_fd, send_packet->data, MAX_BUFFER);
				//printf("READ: %d\n", read_rv);
				if(read_rv < 0) {
					fprintf(stderr, "Could not read from audio fd: %s", strerror(errno));
					return -1;
				}
			}
			
			if(read_rv < MAX_BUFFER && read_rv != 0) {// !=0 to avoid looping an extra time
				send_packet->fin_bit = 1;
				send_packet->size = read_rv;
				send_packet->ack_number++;
				send_packet->sequence_number++;
				sendMessage(sockfd, send_packet, client, from_len);
				//printf("Successfully streamed the audio file to the client!");
				
			}
			else{
				send_packet->size = read_rv;
				if(increaseVol) {
					increaseVol(send_packet->data, start_connection->percentage, send_packet->size);
				}
				else if(decreaseVol) {
					decreaseVol(send_packet->data, start_connection->percentage, send_packet->size);
				}		
				sendMessage(sockfd, send_packet, client, from_len);
				send_packet->ack_number++;
				send_packet->sequence_number++;
				starting = 0;
			}
			
		}

		resetTimeout(sockfd, read_set, &timeout);
		int sync_rv = syncWithClient(sockfd, read_set, &timeout);

		if(FD_ISSET(sockfd, read_set) && sync_rv == 0){
			receiveMessage(sockfd, recv_packet, client, from_len);
			if(recv_packet->fin_bit == 1) {//mention two army problem in report
				//printf("Successfully streamed the audio file to the client!");
				starting = 1;
				
			}
		}
		else if(sync_rv == 1) {
			printf("Client Timed out. Ready for new requests...\n\n");
			return -1;
		}
		
	}
	close(read_fd);
	free(recv_packet);
	free(send_packet);
	free(info);
	return 0;
}


int main (int argc, char **argv) {

	printf ("SysProg network server\n");
	printf ("handed in by Andrea Di Dio\n");
	if(argc != 1) {
		printf("Usage: ./audioserver\n");
		return 1;
	}

	int sockfd;
	SyncPacket* start_connection = NULL;

	struct sockaddr_in client, server;
	socklen_t from_len = sizeof(client);

	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	sockfd = setupSocket(&server);
	

	while(!breakloop) {
		signal(SIGINT, sigint_handler );// trap Ctrl^C signals
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(sockfd, &read_set);
		int stream_rv;

		int select_rv = select(sockfd+1, &read_set, NULL, NULL, NULL);
		if(select_rv < 0) {
			printf("Something went wrong! Ready for new requests...\n\n");
		}
			
		if(select_rv > 0) {
			stream_rv = stream(sockfd, &read_set, &client, from_len, start_connection);
		}

		if(stream_rv < 0) {
			printf("Something went wrong! Ready for new requests...\n\n");
		}
		else {
			printf("Streaming Successful! Ready for new requests...\n\n");
			free(start_connection);
			select(sockfd+1, &read_set, NULL, NULL, NULL);
		}
	}

	int close_rv = close(sockfd);
    if(close_rv < 0) {
        fprintf(stderr, "ERROR: Could not close the socket. %s\n", strerror(errno));
        return 1;
    }

	return 0;
}