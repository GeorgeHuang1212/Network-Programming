#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/sendfile.h>
#include <stdlib.h>

#define BUFSIZE 1024
#define PORT 8082

int servfd, control = 0;
char op = '\0';

void *read_other(void *arg) {
    char readbuf[BUFSIZE];
    while(1) {
        bzero(readbuf, sizeof(readbuf));
        read(servfd, readbuf, sizeof(readbuf));
        if (!strncmp(readbuf, "[From", 5)) {
            printf("%s", readbuf);
            fprintf(stderr, "receive? (-y or -n) ");
            control = 1;
            while (!op);
            if (op == 'y') {
                char *filename = strrchr(readbuf, ']') + 1;
                filename[strlen(filename) - 1] = '\0';
                int filefd = open(filename, O_WRONLY | O_CREAT, 0777);
                bzero(readbuf, sizeof(readbuf));
                read(servfd, readbuf, sizeof(readbuf));
                write(filefd, readbuf, sizeof(readbuf));
                close(filefd);
            }
            else {
                bzero(readbuf, sizeof(readbuf));
                read(servfd, readbuf, sizeof(readbuf));
            }
            control = 0;
            op = '\0';
        }
        else
            printf("%s", readbuf);
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in servaddr;
    pthread_t          tid;
    char writebuf[BUFSIZE], name[20];

    servfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    connect(servfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    printf("Enter your user name: ");
    fgets(name, sizeof(name), stdin);
    name[strlen(name) - 1] = '\0';
    write(servfd, name, strlen(name));

    pthread_create(&tid, NULL, read_other, NULL);

    while(1) {
        fgets(writebuf, BUFSIZE, stdin);
        writebuf[strlen(writebuf) - 1] = '\0';
        if (control == 1) {
            if (!strcmp(writebuf, "-y")) {
                op = 'y';
                continue;
            }
            else {
                op = 'n';
                continue;
            }
        }
        write(servfd ,writebuf, strlen(writebuf)); 
        if (!strcmp(writebuf, "--quit"))
            return 0;
        else if (!strcmp(writebuf, "--who"))
            ;
        else if (!strncmp(writebuf, "--send", 6)) {
            char *filename = strrchr(writebuf, ' ') + 1;
            int filefd = open(filename, O_RDONLY);
            sendfile(servfd, filefd, NULL, BUFSIZE);
            close(filefd); 
        }
        else if (!strncmp(writebuf, "--dm", 4)) {
            char *message = strrchr(writebuf, ' ') + 1;
            char *to = strchr(writebuf, ' ') + 1;
            to[strchr(to, ' ') - to] = '\0';
            printf("[To %s]%s: %s\n", to,name,message);
        }
    }
}
