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

int main (int argc, char *argv [])
{
	int server_fd, audio_fd;
	int sample_size, sample_rate, channels;
	client_filterfunc pfunc;
	char buffer[BUFSIZE];
	struct addrinfo hints, *server;
	fd_set read_set;

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

	FD_SET(server_fd, &read_set);
	int sync_rv = syncWithServer(server_fd, &read_set, &timeout);

	printf ("SysProg2006 network client\n");
	printf ("handed in by VOORBEELDSTUDENT\n");
	
	signal( SIGINT, sigint_handler );	// trap Ctrl^C signals
	
	// parse arguments
	if (argc < 3){
		printf ("error : called with incorrect number of parameters\nusage : %s <server_name/IP> <filename> [<filter> [filter_options]]]\n", argv[0]) ;
		return -1;
	}

	// TO IMPLEMENT
	// send the requested filename and library information to the server
	// and wait for an acknowledgement. Or fail if the server returns an errorcode	
	{
		sample_size = 4;
		sample_rate = 44100;
		channels = 2;
	}
	
	// open output
	audio_fd = aud_writeinit(sample_rate, sample_size, channels);
	if (audio_fd < 0){
		printf("error: unable to open audio output.\n");
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

