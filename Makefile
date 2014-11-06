CC = gcc

CFLAG =-g -std=gnu99 -pedantic -Wall

CPFLAG =-g -std=gnu99 -pedantic -Wall -pthread

prev = ./previous

VPATH = /previous

all: 2310client 2310serv

sError.o:	sError.c sError.h
	$(CC) -c $(CFLAG) sError.c

$(prev)/deck.o:		$(prev)/deck.c $(prev)/deck.h
	$(CC) -c $(CFLAG)  $(prev)/deck.c

player.o:	player.c player.h
		$(CC) -c $(CFLAG) player.c

game.o:		game.c game.h $(prev)/cards.h
		$(CC) -c $(CFLAG) game.c

waitList.o:	waitList.c waitList.h
		$(CC) -c $(CFLAG) waitList.c

2310serv:	2310server.c sError.o game.o waitList.o $(prev)/deck.o
		$(CC) -o 2310serv $(CPFLAG) 2310server.c sError.o game.o waitList.o $(prev)/deck.o


clientError.o:		clientError.c clientError.h
		$(CC) -c $(CFLAG) clientError.c

2310client:	2310client.c clientError.o player.o
		$(CC) -o 2310client  $(CFLAG) 2310client.c clientError.o player.o


clean:
	rm -rf *.o 2310client
cleantest:
	rm Client.test_* Server.test*
