all: chatServer.o chatClient.o
	gcc -pthread -o chatServer chatServer.o
	gcc -pthread -o chatClient chatClient.o

chatServer.o: chatServer.c
	gcc -Wall -g -c chatServer.c

chatClient.o: chatClient.c
	gcc -Wall -g -c chatClient.c

clean:
	rm chatServer chatServer.o chatClient chatClient.o

