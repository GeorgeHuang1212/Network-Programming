#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <fcntl.h>

#define MAXCLI 128
#define PORT 8082
#define BUFSIZE 1024

int clifd[MAXCLI], n = 0;
struct sockaddr_in cliaddr[MAXCLI];
socklen_t clilen = sizeof(cliaddr[0]);
char name[MAXCLI][20];
 
void *chatroom(void *index) {
    int i = *(int *)index;
    char readbuf[BUFSIZE], writebuf[BUFSIZE];

    read(clifd[i], name[i], sizeof(name[i]));

    while(1) {
        bzero(readbuf, sizeof(readbuf));
        read(clifd[i], readbuf, sizeof(readbuf));

        if (!strcmp(readbuf, "--quit")) {
            clifd[i] = 0;
            pthread_exit(0);
        }
        else if (!strcmp(readbuf, "--who")) {
            bzero(writebuf, sizeof(writebuf));
            for (int j = 0; j < n; ++j)
                if (clifd[j]) {
                    strcat(writebuf, name[j]);
                    strcat(writebuf, "\n");
                }
            write(clifd[i], writebuf, sizeof(writebuf));
        }
        else if (!strncmp(readbuf, "--send", 6)) {
            int tofd;
            char *filename = strrchr(readbuf, ' ') + 1;
            char *to = strchr(readbuf, ' ') + 1;
            to[strchr(to, ' ') - to] = '\0';
            sprintf(writebuf, "[From %s]%s\n", name[i], filename);
            for (int j = 0; j < n; ++j)
                if (!strcmp(name[j], to) && clifd[j]) {
                    write(clifd[j], writebuf, sizeof(writebuf));
                    tofd = clifd[j];
                    break;
                }
            bzero(readbuf, sizeof(readbuf));
            read(clifd[i], readbuf, sizeof(readbuf));
            write(tofd, readbuf, sizeof(readbuf));
        }
        else if (!strncmp(readbuf, "--dm", 4)) {
            char *message = strrchr(readbuf, ' ') + 1;
            char *to = strchr(readbuf, ' ') + 1;
            to[strchr(to, ' ') - to] = '\0';
            sprintf(writebuf, "[To you]%s: %s\n", name[i], message);
            for (int j = 0; j < n; ++j)
                if (!strcmp(name[j], to) && clifd[j]) {
                    write(clifd[j], writebuf, sizeof(writebuf));
                    break;
                }
        }
        else {
            sprintf(writebuf, "%s: %s\n", name[i],readbuf);
            for (int j = 0; j < n; ++j)
                if (clifd[j])
                    write(clifd[j], writebuf, sizeof(writebuf));
        }
    }
}

int main(int argc, char *argv[]) {
    int                servfd;
    struct sockaddr_in servaddr;
    pthread_t          tid;

    servfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    bind(servfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(servfd, MAXCLI);

    int index = 0;
    while(1) {
        clifd[n] = accept(servfd, (struct sockaddr *) &cliaddr[n], &clilen);
        index = n;
        ++n;
        pthread_create(&tid, NULL, chatroom, &index);
    }
}
