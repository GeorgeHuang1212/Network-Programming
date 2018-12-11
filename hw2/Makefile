all: chatServer.o
	gcc -pthread -o chatServer chatServer.o

chatServer.o: chatServer.c
	gcc -Wall -g -c chatServer.c

clean:
	rm chatServer chatServer.o 

