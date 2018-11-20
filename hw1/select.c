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
#define ICOSIZE 70000
#define IMGSIZE 120000
#define SVR_PORT 80

char webpage[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>NetworkProgramming hw</title>\r\n"
"<style>body { background-color: #FFFFFF }</style></head>\r\n"
"<body><center><h1>James Rodriguez <3</h1><br>\r\n"
"<img src=\"1.jpg\"></center></body></html>\r\n";

int main(int argc, char* argv[])
{
	struct sockaddr_in server_addr, client_addr;
	socklen_t	sin_len = sizeof(client_addr);
	fd_set active_fd_set;
	int fd_server, fd_client, max_fd;
	char buf[MAXBUF];
	int fdimg;
	int on = 1;

	if((fd_server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	else
	{
		printf("fd_server = [%d]\n", fd_server);
	}

	if (setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
	{
		perror("setsockopt()");
		return -1;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(SVR_PORT);

	if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		close(fd_server);
		exit(1);
	}
	else
	{
		printf("bind [%s:%u] success\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	}

	if(listen(fd_server, 10) == -1)
	{
		perror("listen");
		close(fd_server);
		exit(1);
	}

	FD_ZERO(&active_fd_set);
	FD_SET(fd_server, &active_fd_set);
	max_fd = fd_server;
	
	while(!0)
	{
		int ret;
		struct timeval tv;
		fd_set read_fds;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		read_fds = active_fd_set;
		ret = select(max_fd+1, &read_fds, NULL, NULL, &tv);

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
						fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);
						if(fd_client == -1)
						{
							perror("accept()");
							return -1;
						}
						else
						{
						memset(buf, 0, MAXBUF);
						read(fd_client, buf, MAXBUF-1);
							printf("Accept client come from [%s:%u] by fd [%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), fd_client);

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
							if (fd_client > max_fd)
								max_fd = fd_client;
						}
					}
					else
					{
						int recv_len;
						memset(buf, 0, MAXBUF);
            recv_len = recv(i, buf, sizeof(buf), 0);
                        if (recv_len == -1) {
                            perror("recv()");
                            return -1;
                        } else if (recv_len == 0) {
                            printf("Client disconnect\n");
                        } else {
                            printf("Receive: len=[%d] msg=[%s]\n", recv_len, buf);
 
                            send(i, buf, recv_len, 0);
                        }
 
                        close(i);
                        FD_CLR(i, &active_fd_set);
					}
				}
			}
		}
	}
	return 0;
}
