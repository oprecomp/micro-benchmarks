#
# makefile for jacobi solver
#
#

# 
# precision
#
SINGLE=1

ifdef SINGLE
   #OPTS=-DSINGLE -Wdouble-promotion -fsingle-precision-constant
   OPTS=-DSINGLE -fsingle-precision-constant
endif

CC=gcc
CFLAGS=-O2 -g -fopenmp 
LDFLAGS= -lm -lrt -fopenmp

#
##  Get microbenchmark directory
#

BMDIR = $(dir $(firstword $(MAKEFILE_LIST)))
COMMON_DIR=$(BMDIR)/../common

#
## SRC directory
JACOBI_SRC=jacobi.c
JACOBI_OBJ=jacobi.o
COMMON_OBJ=oprecomp.o

SRC=$(addprefix $(BMDIR),$(JACOBI_SRC))
OBJ=$(JACOBI_OBJ) $(COMMON_OBJ)

# INCLUDEs
INCS=$(COMMON_DIR)

build:jacobi

jacobi: $(OBJ) 
	$(CC) -o $@ $(OBJ) $(LDFLAGS) 

$(COMMON_OBJ):$(COMMON_DIR)/oprecomp.c
	$(CC) $(CFLAGS) -c $<

%.o:$(BMDIR)/%.c
	$(CC) $(CFLAGS) $(OPTS) -I$(INCS) -c  $<


clean:
	rm *.o jacobi
