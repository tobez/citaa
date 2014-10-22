OPTIMIZE?=-O2 -g
CFLAGS?=-Wall -Werror -fno-strict-aliasing
CAIRO_CFLAGS = `pkg-config --cflags gtk+-2.0`
CAIRO_LFLAGS = `pkg-config --libs gtk+-2.0`
CC?=cc

citaa: main.o carp.o read.o image.o graph.o trace.o paint.o component.o text.o blur.o
	$(CC) $(CFLAGS) $(OPTIMIZE) -lm $(CAIRO_LFLAGS) -o citaa \
	    main.o carp.o read.o image.o graph.o trace.o paint.o component.o text.o blur.o

clean:
	rm citaa main.o carp.o read.o image.o graph.o trace.o paint.o component.o text.o blur.o

main.o: main.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o main.o main.c

carp.o: carp.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o carp.o carp.c

read.o: read.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o read.o read.c

image.o: image.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o image.o image.c

component.o: component.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o component.o component.c

text.o: text.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o text.o text.c

graph.o: graph.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o graph.o graph.c

trace.o: trace.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) -o trace.o trace.c

paint.o: paint.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) $(CAIRO_CFLAGS) -o paint.o paint.c

blur.o: blur.c citaa.h
	$(CC) -c $(CFLAGS) $(OPTIMIZE) $(CAIRO_CFLAGS) -o blur.o blur.c

