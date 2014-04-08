OPTIMIZE?=-O2 -g
CFLAGS?=-Wall -Werror -fno-strict-aliasing
CC?=cc

citaa: main.o carp.o read.o image.o graph.o
	$(CC) $(CFLAGS) $(OPTIMIZE) -o citaa \
	    main.o carp.o read.o image.o graph.o

clean:
	rm citaa main.o carp.o read.o image.o graph.o

main.o: main.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o main.o main.c

carp.o: carp.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o carp.o carp.c

read.o: read.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o read.o read.c

image.o: image.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o image.o image.c

graph.o: graph.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o graph.o graph.c

