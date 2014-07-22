all: ui.o fetcher.o
	gcc -o tracer ui.o fetcher.o -lncurses -lpthread

fetcher.o: fetcher.c ui.c fetcher.h packet.h ui.h
	gcc -c fetcher.c

ui.o: ui.c ui.h fetcher.h
	gcc -c ui.c
