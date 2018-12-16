#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER 500
#define USERNAMESIZE 20
#define FILESIZE 3000

bool flag = false;
char flag2 = '\0';
int fd_client;
char username[USERNAMESIZE];

typedef struct
{
	char* prompt;
	int socket;
} thread_data;

void* read_other(void *arg)
{
	char readbuf[BUFFER];
	while(true)
	{
		memset(readbuf, 0, BUFFER);
		read(fd_client, readbuf, BUFFER);
		if( strncmp(readbuf, "[From", 5) == 0)
		{
			printf("%s", readbuf);
			fprintf(stderr, "receive? (y or n)");
			flag = true;
			while(!flag2);
			if( flag2 == 'y')
			{
				char* filename = strrchr(readbuf, ']') + 1;
				filename[strlen(filename) - 1] = '\0';
				int file_fd = open(filename, O_WRONLY | O_CREAT, 0777);
				memset(readbuf, 0, BUFFER);
				read(fd_client, readbuf, BUFFER);
				write(file_fd, readbuf, BUFFER);
				close(file_fd);
			}
			else
			{
				memset(readbuf, 0, BUFFER);
				read(fd_client, readbuf, BUFFER);
			}
			flag = false;
			flag2 = '\0';
		}
		else
		{
			printf("%s", readbuf);
		}
	}
}

void connectToServer(int socket_fd, struct addrinfo* info)
{
	int response = connect(socket_fd, info->ai_addr, info->ai_addrlen);
	if (response == -1)
	{
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("Connected\n");
	}

	return;
}

void sendMessage()
{
//	printf("%s", prompt);
	char writebuf[BUFFER];

	while( fgets(writebuf, BUFFER, stdin) != NULL)
	{
		int str_len = strlen(writebuf); 
		writebuf[str_len-1] = '\0';
		if( flag == true)
		{
			if( strncmp(writebuf, "y", 1))
			{
				flag2 = 'y';
				continue;
			}
			else
			{
				flag2 = 'n';
				continue;
			}
		}
		write(fd_client, writebuf, str_len);
		if( strncmp(writebuf, "--quit", 6) == 0)
		{
			return;
		}
		else if( strncmp(writebuf, "--who", 5) == 0)
		{
			;
		}
		else if( strncmp(writebuf, "--send", 6) == 0)
		{
			char* filename = strrchr(writebuf, ' ') + 1;
			int file_fd = open(filename, O_RDONLY);
			sendfile(fd_client, file_fd, NULL, FILESIZE);
			close(file_fd);
		}
		else if( strncmp(writebuf, "--dm", 4) == 0)
		{
			char* message = strrchr(writebuf, ' ') + 1;
			char* to = strchr(writebuf, ' ') + 1;
			to[strchr(to, ' ') -to] = '\0';
			printf("[To %s]%s: %s\n", to, username, message);
		}
	}
}

int main(int argc, char** argv)
{
	struct addrinfo hints, *server_info, *client_info;
	char prompt[USERNAMESIZE+4];
	int status;
	pthread_t thread;

	if( argc < 3)
	{
		printf("Usage: ./chatClient ip_address port_number\n");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(argv[1], argv[2], &hints, &server_info)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
	printf("server info set\n");

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(NULL, argv[2], &hints, &client_info)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
	printf("client info set\n");

	if((fd_client = socket(client_info->ai_family, client_info->ai_socktype, client_info->ai_protocol)) == -1)
	{
		perror("client: socket");
		exit(EXIT_FAILURE);
	}
	printf("socket success\n");

	connectToServer(fd_client, server_info);
	
	printf("Enter user name: ");
	fgets(username, USERNAMESIZE, stdin);
	username[strlen(username) - 1] = '\0';
	strcpy(prompt, username);
	strcat(prompt, "> ");
	write(fd_client, username, USERNAMESIZE);

	//write(fd_client, prompt, USERNAMESIZE);

	pthread_create(&thread, NULL, read_other, NULL);


	while(true)
	{
		sendMessage();
				
	}


	close(fd_client);
	freeaddrinfo(client_info);
	freeaddrinfo(server_info);
	pthread_exit(NULL);

	return 0;
}
