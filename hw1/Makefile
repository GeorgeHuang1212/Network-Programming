all: fork.o select.o
	gcc -o fork fork.o
	gcc -o select select.o

fork.o: fork.c
	gcc -Wall -g -c  fork.c

select.o: select.c
	gcc -Wall -g -c  select.c

clean:
	rm fork.o select.o fork select

