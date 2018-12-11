#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER 500
#define BACKLOG 10

void* get_in_addr(struct sockaddr* sa)
{
	if( sa->sa_family == AF_INET)
		return &( (struct sockaddr_in*) sa)->sin_addr;
	else
		return &( (struct sockaddr_in6*) sa)->sin6_addr;
}

void myexit(int fd, struct addrinfo* info)
{
	close(fd);
	freeaddrinfo(info);
	exit(EXIT_FAILURE);
}

void startServer(int socket_fd, struct addrinfo* info)
{
	int on = 1;
	if( setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1)
	{
		perror("server: setsockopt");
		myexit(socket_fd, info);
	}
	if( bind(socket_fd, info->ai_addr, info->ai_addrlen) == -1)
	{
		perror("server: bind");
		myexit(socket_fd, info);
	}	
	if( listen(socket_fd, BACKLOG) == -1)
	{
		perror("listen");
		myexit(socket_fd, info);
	}
	freeaddrinfo(info);

	return;
}

void* receive(void* socket)
{
	int response;
	intptr_t socket_fd;
	char message[BUFFER];
	memset(message, 0, BUFFER);

	socket_fd = (intptr_t) socket;

	while(true)
	{
		response = recvfrom(socket_fd, message, BUFFER, 0, NULL, NULL);
		if(response)
		{
			printf("%s", message);
		}
	}
}

void send_message(int socket_fd, struct sockaddr_storage* address, socklen_t address_size)
{
	char message[BUFFER];
	while( fgets(message, BUFFER, stdin) != NULL)
	{
		if( strncmp(message, "/quit", 5) == 0)
		{
			printf("Closing connection...\n");
			exit(EXIT_SUCCESS);
		}
		sendto(socket_fd, message, BUFFER, 0, (struct sockaddr*) address, address_size); 
	}
}

int main(int argc, char** argv)
{
	struct addrinfo hints, *servInfo;
	struct sockaddr_storage client_addr;
	int status, fd_server;
	intptr_t fd_client;
	char addrstr[INET6_ADDRSTRLEN];
	socklen_t client_addr_size;
	pthread_t thread;

	if (argc < 2)
	{
		printf("Usage: server port_number\n");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(NULL, argv[1], &hints, &servInfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
	if((fd_server = socket(servInfo->ai_family, servInfo->ai_socktype, servInfo->ai_protocol)) == -1)
	{
		perror("server: socket");
		exit(EXIT_FAILURE);
	}

	startServer(fd_server, servInfo);

	client_addr_size = sizeof(struct sockaddr_storage);
	if((fd_client = accept(fd_server, (struct sockaddr*) &client_addr, &client_addr_size)) == -1)
	{
		perror("accept");
		myexit(fd_server, NULL);
	}	

	inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*) &client_addr), addrstr, sizeof(addrstr));
	printf("server: got connection from %s\n", addrstr);

	pthread_create(&thread, NULL, receive, (void*) fd_client);

	send_message(fd_client, &client_addr, client_addr_size);

	close(fd_client);
	close(fd_server);
	pthread_exit(NULL);

	return EXIT_SUCCESS;
}
