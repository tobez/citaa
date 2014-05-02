OPTIMIZE?=-O2 -g
CFLAGS?=-Wall -Werror -fno-strict-aliasing
CAIRO_CFLAGS != pkg-config --cflags gtk+-3.0
CAIRO_LFLAGS != pkg-config --libs gtk+-3.0
CC?=cc

citaa: main.o carp.o read.o image.o graph.o trace.o
	$(CC) $(CFLAGS) $(OPTIMIZE) $(CAIRO_LFLAGS) -o citaa \
	    main.o carp.o read.o image.o graph.o trace.o

clean:
	rm citaa main.o carp.o read.o image.o graph.o

main.o: main.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) $(CAIRO_CFLAGS) -o main.o main.c

carp.o: carp.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o carp.o carp.c

read.o: read.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o read.o read.c

image.o: image.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o image.o image.c

graph.o: graph.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o graph.o graph.c

trace.o: trace.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o trace.o trace.c

