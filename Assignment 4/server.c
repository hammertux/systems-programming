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

/// a define used for the copy buffer in stream_data(...)
#define BUFSIZE 1024
#define IP_PROTOCOL 0
#define SERVER_PORT 1234

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

int syncWithClient(int sockfd, fd_set* sync, struct timeval* timeout) {
    int select_rv = select(sockfd + 1, sync, NULL, NULL, timeout);
    if(select_rv < 0) {
        fprintf(stderr, "ERROR: Could not sync fds. %s\n", strerror(errno));
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
	int send_rv = sendto(sockfd, info, sizeof(AudioInfo), 0, (struct sockaddr* )client, from_len);
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

/// stream data to a client. 
///
/// This is an example function; you do *not* have to use this and can choose a different flow of control
///
/// @param fd an opened file descriptor for reading and writing
/// @return returns 0 on success or a negative errorcode on failure
int stream_data(int client_fd)
{
	int data_fd;
	int channels, sample_size, sample_rate;
	server_filterfunc pfunc;
	char *datafile, *libfile;
	char buffer[BUFSIZE];
	AudioInfo* info;
	
	// TO IMPLEMENT
	// receive a control packet from the client 
	// containing at the least the name of the file to stream and the library to use
	{
		datafile = strdup("example.wav");
		libfile = NULL;
	}
	
	// open input
	data_fd = aud_readinit(datafile, &info->sample_rate, &info->sample_size, &info->channels);
	if (data_fd < 0){
		printf("failed to open datafile %s, skipping request\n",datafile);
		return -1;
	}
	printf("opened datafile %s\n",datafile);

	// optionally open a library
	if (libfile){
		// try to open the library, if one is requested
		pfunc = NULL;
		if (!pfunc){
			printf("failed to open the requested library. breaking hard\n");
			return -1;
		}
		printf("opened libraryfile %s\n",libfile);
	}
	else{
		pfunc = NULL;
		printf("not using a filter\n");
	}
	
	// TO IMPLEMENT : optionally return an error code to the client if initialization went wrong
	
	// start streaming
	{
		int bytesread, bytesmod;
		
		bytesread = read(data_fd, buffer, BUFSIZE);
		while (bytesread > 0){
			// you might also want to check that the client is still active, whether it wants resends, etc..
			
			// edit data in-place. Not necessarily the best option
			if (pfunc)
				bytesmod = pfunc(buffer,bytesread); 
			write(client_fd, buffer, bytesmod);
			bytesread = read(data_fd, buffer, BUFSIZE);
		}
	}

	// TO IMPLEMENT : optionally close the connection gracefully 	
	
	if (client_fd >= 0)
		close(client_fd);
	if (data_fd >= 0)
		close(data_fd);
	if (datafile)
		free(datafile);
	if (libfile)
		free(libfile);
	
	return 0;
}

int stream(int sockfd, fd_set* read_set, struct sockaddr_in* client, socklen_t from_len) {

	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 500000;
	int read_rv;

	FD_ZERO(read_set);
	int starting = 1;
	Packet* send_packet = buildPacket(NULL, 0, 0, 0), *recv_packet = buildPacket(NULL, 0, 0, 0);
	SyncPacket* start_connection = NULL;
	send_packet->fin_bit = 0;
	recv_packet->fin_bit = 0;
	char* filename;
	AudioInfo* info = initInfo(0, 0, 0);

	uint16_t read_fd;
	FD_SET(sockfd, read_set);
	int sync_rv = syncWithClient(sockfd, read_set, &timeout);

	do {
		
		if(FD_ISSET(sockfd, read_set) && sync_rv == 0){
			breakloop = 0;
			if(starting) {
				start_connection = recvInitPacket(sockfd, client, from_len);
				
				
				read_fd = aud_readinit(start_connection->file, &info->sample_rate, &info->sample_size, &info->channels);
				if(read_fd < 0) {
					return -1;
				}
				sendInfo(sockfd, client, info, from_len);
				starting = 0;
			}
			else {
				int check = checkPacket(send_packet, recv_packet);
				if(check == 1) {
					printf("OOps");
					sendMessage(sockfd, send_packet, client, from_len);
					receiveMessage(sockfd, recv_packet, client, from_len);
				}
				else {
					read_rv = read(read_fd, send_packet->data, MAX_BUFFER);
					printf("READ: %d\n", read_rv);
					if(read_rv < 0) {
						fprintf(stderr, "Could not read from audio fd: %s", strerror(errno));
						return -1;
					}
					if(read_rv < MAX_BUFFER || read_rv == 0) {
						send_packet->fin_bit = 1;
						send_packet->size = read_rv;
						send_packet->ack_number++;
						send_packet->sequence_number++;
						sendMessage(sockfd, send_packet, client, from_len);
						printf("LESS THAN MAX_BUFFER--SENT FIN\n");
						//printf("Successfully streamed the audio file to the client!");
						close(read_fd);	
						starting = 1;
						//receiveMessage(sockfd, recv_packet, &client, from_len);
						//printPacket(send_packet, "r");
						return 0;
						
					}
					else{
						send_packet->size = read_rv;
						send_packet->ack_number++;
						send_packet->sequence_number++;
						sendMessage(sockfd, send_packet, client, from_len);
						
						
					}
					
				}
			
				receiveMessage(sockfd, recv_packet, client, from_len);
					if(recv_packet->fin_bit == 1) {//mention two army problem in report
							printf("Successfully streamed the audio file to the client!");
							starting = 1;
							breakloop = 1;
							//free(send_packet);
							//free(recv_packet);
							close(read_fd);
							return 0;
						}
				
			}
			
			
			
			

		}
		else if(sync_rv == 1) {
			
			breakloop = 1;
		}
		
	}while(!breakloop);
}

/// the main loop, continuously waiting for clients
int main (int argc, char **argv)
{
	printf ("SysProg network server\n");
	printf ("handed in by VOORBEELDSTUDENT\n");
	if(argc != 1) {
		printf("Usage: ./audioserver");
		return 1;
	}

	int read_rv;
	int sockfd;
	int starting = 1, active = 1;
	fd_set read_set;
	
	char buffer[sizeof(Packet)];

	struct sockaddr_in client, server;
	socklen_t from_len = sizeof(client);
	

	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	sockfd = setupSocket(&server);
	

	while(active) {
		// breakloop = 0;

		FD_SET(sockfd, &read_set);
		int stream_rv;
		

		signal(SIGINT, sigint_handler );	// trap Ctrl^C signals
		if(select(sockfd+1, &read_set, NULL, NULL, NULL) > 0) {
		stream_rv = stream(sockfd, &read_set, &client, from_len);
		}

		if(stream_rv == 0) {
			printf("Streaming Successful!!!\n");
			select(sockfd+1, &read_set, NULL, NULL, NULL);
		}
		else {
			printf("Something went wrong!\n");
			select(sockfd+1, &read_set, NULL, NULL, NULL);
		}

		
		
		//fprintf(stderr, "Client not responding. Ready for new requests.");
	}
	

	int close_rv = close(sockfd);
    if(close_rv < 0) {
        fprintf(stderr, "ERROR: Could not close the socket. %s\n", strerror(errno));
        return 1;
    }

	
	

	return 0;
}

