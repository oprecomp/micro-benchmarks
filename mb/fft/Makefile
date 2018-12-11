# 
# FFT microbenchmark for Oprecomp
#
#  A. Emerson
#  Based on FFTW2 for Quantum Espresso

#
#  Get microbenchmark directory
#

BMDIR = $(dir $(firstword $(MAKEFILE_LIST)))

# INCLUDEs
COMMON_DIR=$(BMDIR)/../common
INCS=$(COMMON_DIR)


#
# FLAGS
#
CC=gcc
CFLAGS=-O2

LIB_SRC=$(addprefix $(BMDIR)/,fftw.c fftw.h konst.h)
SRC=$(addprefix $(BMDIR)/,fft_stick.c run_fftw.c)
COMMON_SRC=$(addprefix $(COMMON_DIR)/, oprecomp.c)

TEST=$(addprefix $(BMDIR)/,test.sh)

OBJ=fft_stick.o run_fftw.o oprecomp.o

build: run_fftw

$(SRC):$(LIB_SRC)
	touch $(SRC)

%.o:$(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCS) -c  $<

%.o:$(BMDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCS) -c  $<

run_fftw: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ -lm -lrt

test: run_fftw
	$(TEST)

clean:
	rm -f *.o run_fftw
