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
#include <signal.h>
#define MAXBUF 2048
#define ICOSIZE 70000
#define IMGSIZE 120000
#define SVR_PORT "80"
#define BACKLOG 10

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void* get_in_addr(struct sockaddr *sa)
{
	if( sa->sa_family == AF_INET)
		return &( (struct sockaddr_in*) sa)->sin_addr;
	else
		return &( (struct sockaddr_in6*) sa)->sin6_addr;
}

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
	struct sockaddr_storage client_addr;
	struct addrinfo hints, *servinfo, *p;
	struct sigaction siga;
	socklen_t caddr_size;
	int fd_server, fd_client;
	char buf[MAXBUF];
	char addrstr[INET6_ADDRSTRLEN];
	int fdimg, pid, status;
	int on = 1;

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

	siga.sa_handler = sigchld_handler;
	sigemptyset(&siga.sa_mask);
	siga.sa_flags = SA_RESTART;
	
	if( sigaction(SIGCHLD, &siga, NULL) == -1)
	{
		perror("sigaction");
		close(fd_server);
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(!0)
	{
		caddr_size = sizeof(client_addr);
		if((fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &caddr_size)) == -1)
		{
			perror("accept");
			continue;	
		}

		inet_ntop(client_addr.ss_family, get_in_addr( (struct sockaddr *) &client_addr), addrstr, sizeof(addrstr));
		printf("server: got connection from %s\n", addrstr);

		pid = fork();
		if(pid < 0)
		{
			perror("fork error");
			continue;
		}
		else if(pid == 0)
		{
			/* child process */
			close(fd_server);
			memset(buf, 0, MAXBUF);
			recv(fd_client, buf, MAXBUF-1, 0);
			
			printf("%s\n", buf);

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
				send(fd_client, webpage, sizeof(webpage)-1, 0);

			close(fd_client);
			printf("closing...\n");
			printf("\n");
			exit(0);
		}
		else
			/* parent process */
		{
			close(fd_client);
		}

	}


	return 0;
}
