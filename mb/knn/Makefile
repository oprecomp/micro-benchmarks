VPATH = $(dir $(firstword $(MAKEFILE_LIST)))

CC = gcc
CFLAGS = -Wall -O3 -std=gnu99 -I$(VPATH)../common
LDFLAGS = -lm -lrt

build: knn

knn: knn.o oprecomp.o
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	/bin/rm -rf tags core *.o knn

tags: *.c *.h
	ctags *.c *.h

test: knn
	./knn 10 data/prepared/mb/knn/adult.data 1

%.o: %.c
	$(CC) $(CFLAGS) -c $<

oprecomp.o: $(VPATH)../common/oprecomp.c
	$(CC) $(CFLAGS) -c $<
