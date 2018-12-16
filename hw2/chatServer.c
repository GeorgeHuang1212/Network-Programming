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
#define MAXCLI 10
#define USERNAMESIZE 20

int fd_client[MAXCLI], thread_n = 0;
struct sockaddr_storage client_addr[MAXCLI];

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
	printf("bind succes\n");
	if( listen(socket_fd, MAXCLI) == -1)
	{
		perror("listen");
		myexit(socket_fd, info);
	}
	freeaddrinfo(info);

	return;
}

void* chatroom(void* i)
{
	int index = *(int*) i;
	char userName[MAXCLI][USERNAMESIZE];
	char readbuf[BUFFER], writebuf[BUFFER];

	memset(userName, 0, sizeof(userName));

	read(fd_client[index], userName[index], USERNAMESIZE);
	
	while(true)
	{
		memset(readbuf, 0, BUFFER);
		read(fd_client[index], readbuf, BUFFER);

		if( strncmp(readbuf, "--quit", 6) == 0)
		{
			fd_client[index] = 0;
			pthread_exit(NULL);
		}	
		else if( strncmp(readbuf, "--who", 5) == 0)
		{
			for( int j = 0; j < thread_n; j++)
			{
				if( fd_client[j] == true)
				{
					strcat(writebuf, userName[j]);
					strcat(writebuf, "\n");
				}
			}
			write(fd_client[index], writebuf, BUFFER);
		}
		else if( strncmp(readbuf, "--dm", 4) == 0)
		{
			char* message = strrchr(readbuf, ' ') + 1;
			char* to = strchr(readbuf, ' ') + 1;
			to[strchr(to, ' ') - to] = '\0';

			sprintf(writebuf, "[For you]%s: %s\n", userName[index], message);
			for( int j = 0; j < thread_n; j++)
			{
				if( strcmp(userName[j], to) == 0 && fd_client[j] == true)
				{
					write(fd_client[j], writebuf, sizeof(writebuf));
					break;
				}
			}
		}
		else if( strncmp(readbuf, "--send", 6))
		{
			char* filename = strrchr(readbuf, ' ') + 1;
			char* to = strchr(readbuf, ' ') + 1;
			to[strchr(to, ' ') - to] = '\0';
			int receive_fd;

			sprintf(writebuf, "[From %s]%s\n", userName[index], filename);
			for( int j = 0; j < thread_n; j++)
			{
				if( strcmp(userName[j], to) == 0 && fd_client[j] == true)
				{
					write(fd_client[j], writebuf, sizeof(writebuf));
					receive_fd = fd_client[j];
					break;
				}
			}
			read(fd_client[index], readbuf, BUFFER);
			write(receive_fd, readbuf, BUFFER);
		}
		else
		{
			sprintf(writebuf, "%s: %s\n", userName[index], readbuf);
			for( int j = 0; j < thread_n; j++)
			{
				if( fd_client[j] == true)
				{
					write(fd_client[j], writebuf, BUFFER);
				}
			}
		}
	}

}

int main(int argc, char** argv)
{
	struct addrinfo hints, *servInfo;
	int status, fd_server;
	intptr_t now_thread;
	char addrstr[INET6_ADDRSTRLEN];
	char ipstr[INET6_ADDRSTRLEN];
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

	void* addr;
	struct sockaddr_in *ipv4 = (struct sockaddr_in*) servInfo->ai_addr;
	addr = &(ipv4->sin_addr);
	inet_ntop(servInfo->ai_family, addr, ipstr, INET6_ADDRSTRLEN);
	printf("address = %s\n", ipstr);

	if((fd_server = socket(servInfo->ai_family, servInfo->ai_socktype, servInfo->ai_protocol)) == -1)
	{
		perror("server: socket");
		exit(EXIT_FAILURE);
	}
	printf("socket succes\n");

	startServer(fd_server, servInfo);

	now_thread = 0;
	while(true)
	{
		client_addr_size = sizeof(struct sockaddr_storage);
		if((fd_client[thread_n] = accept(fd_server, (struct sockaddr*) &client_addr[thread_n], &client_addr_size)) == -1)
		{
			perror("accept");
			myexit(fd_server, NULL);
		}	
	
		inet_ntop(client_addr[thread_n].ss_family, get_in_addr((struct sockaddr*) &client_addr[thread_n]), addrstr, sizeof(addrstr));
		printf("server: got connection from %s\n", addrstr);

		now_thread = thread_n;
		thread_n++;
	
		pthread_create(&thread, NULL, chatroom, &now_thread);
	}

	for( int i = 0; i < thread_n; i++)
	{
		close(fd_client[i]);
	}
	close(fd_server);
	pthread_exit(NULL);

	return EXIT_SUCCESS;
}
