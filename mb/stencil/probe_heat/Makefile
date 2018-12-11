#
# makefile for jacobi solver
#
#

CC=gcc
CFLAGS=-O2 -g
LDFLAGS=-lm

SRC=jacobi.c
OBJ=jacobi.o

jacobi: $(OBJ)
	$(CC) -o $@ $< $(LDFLAGS) 

%.o:%.c
	$(CC) $(CFLAGS) -c $(SRC) $<

clean:
	rm *.o
