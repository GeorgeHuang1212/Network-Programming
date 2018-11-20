#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#define MAXBUF 2048
#define BACKLOG 10
#define ICOSIZE 70000
#define IMGSIZE 120000
#define SVR_PORT "80"

char webpage[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>NetworkProgramming hw</title>\r\n"
"<style>body { background-color: #FFFFFF }</style></head>\r\n"
"<body><center><h1>James Rodriguez <3</h1><br>\r\n"
"<img src=\"1.jpg\"></center></body></html>\r\n";

void* get_in_addr(struct sockaddr *sa)
{
	if( sa->sa_family == AF_INET)
		return &( (struct sockaddr_in*) sa)->sin_addr;
	else
		return &( (struct sockaddr_in6*) sa)->sin6_addr;
}

int main(int argc, char* argv[])
{
	struct sockaddr_storage client_addr;
	struct addrinfo hints, *servinfo, *p;
	socklen_t caddr_size;
	char buf[MAXBUF];
	int fdimg, status;
	fd_set active_fd_set;
	int fd_server, fd_client, fdmax;
	int on = 1;
	char addrstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if((status = getaddrinfo(NULL, SVR_PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	for( p = servinfo; p != NULL; p = p->ai_next)
	{
		if((fd_server = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}
	
		if( setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1)
		{
			perror("setsockopt");
			close(fd_server);
			freeaddrinfo(servinfo);
			exit(1);
		}
		
		if( bind(fd_server, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
		{
			perror("server: bind");
			close(fd_server);
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "server: bind failed...\n");
		close(fd_server);
		freeaddrinfo(servinfo);
		exit(1);
	}

	freeaddrinfo(servinfo);
	
	if( listen(fd_server, BACKLOG) == -1)
	{
		perror("listen");
		close(fd_server);
		exit(1);
	}

	FD_ZERO(&active_fd_set);
	FD_SET(fd_server, &active_fd_set);
	fdmax = fd_server;
	
	while(!0)
	{
		int ret;
		struct timeval tv;
		fd_set read_fds;

		tv.tv_sec = 8;
		tv.tv_usec = 0;

		read_fds = active_fd_set;
		ret = select(fdmax+1, &read_fds, NULL, NULL, &tv);

		if(ret == -1)
		{
			perror("select()");
			return -1;
		}
		else if(ret == 0)
		{
			printf("select timeout\n");
			continue;
		}
		else
		{
			int i;

			for (i = 0; i < FD_SETSIZE; i++)
			{
				if( FD_ISSET(i, &read_fds) )
				{
					if( i == fd_server )
					{
						caddr_size = sizeof(caddr_size);
						fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &caddr_size);
						if(fd_client == -1)
						{
							perror("accept()");
							return -1;
						}
						else
						{
							memset(buf, 0, MAXBUF);
							read(fd_client, buf, MAXBUF-1);

							if(strncmp(buf, "GET /favicon.ico", 16) == 0)
							{
								fdimg = open("favicon.ico", O_RDONLY);
								sendfile(fd_client, fdimg, NULL, ICOSIZE);
								close(fdimg);
							}
							else if(strncmp(buf, "GET /1.jpg", 10) == 0)
							{
								fdimg = open("1.jpg", O_RDONLY);
								sendfile(fd_client, fdimg, NULL, IMGSIZE);
								close(fdimg);
							}
							else
								write(fd_client, webpage, sizeof(webpage)-1);

							FD_SET(fd_client, &active_fd_set);
							if (fd_client > fdmax)
								fdmax = fd_client;
							printf("server: new connection from %s on socket %d\n", inet_ntop(client_addr.ss_family, get_in_addr( (struct sockaddr *) &client_addr), addrstr, INET6_ADDRSTRLEN), fd_client);
						}
					}
					else
					{
						int recv_len;
						//memset(buf, 0, MAXBUF);
            if((recv_len = recv(i, buf, sizeof(buf), 0) <= 0))
						{
							if(recv_len == 0)
								printf("server: socket %d hung up\n", i);
							else
								perror("recv");
							close(i);
							FD_CLR(i, &active_fd_set);
						}
						else
						{

 
						//send(i, buf, recv_len, 0);
            }
					}
				}
			}
		}
	}
	return 0;
}
