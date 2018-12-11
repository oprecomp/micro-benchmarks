VPATH = $(dir $(firstword $(MAKEFILE_LIST)))

CC = g++
FC = gfortran
F2C = f2c
FLAGS = -O2 -Wno-write-strings -fopenmp -I$(VPATH)../common # -DFIXED_POINT # -fopenmp

# Standalone version
#
CFLAGS = -DNO_PULP
LDFLAGS = -lm -lrt
BLAS = saxpy.o sscal.o scopy.o sswap.o sger.o ssyr2.o
LAPACK = sstemr.o slarrc.o slae2.o slaev2.o slanst.o slarrr.o slarre.o slarrv.o slarrj.o slasrt.o slassq.o slarra.o slarrd.o slarrk.o slarnv.o slarrb.o slasq2.o slaset.o slarrf.o slar1v.o slaruv.o slaneg.o slasq3.o slasq4.o slasq5.o slasq6.o slaebz.o \
ssytd2.o slarfg.o slapy2.o \
sorm2l.o slarf.o ilaslc.o ilaslr.o sdot.o sgemv.o ssymv.o snrm2.o

# ESSL version
#
# CFLAGS = -DUSE_ESSL -DNO_PULP
# LDFLAGS = -lessl -lm -lrt
# BLAS =
# LAPACK = sstemr.o slarrc.o slae2.o slaev2.o slanst.o slarrr.o slarre.o slarrv.o slarrj.o slasrt.o slassq.o slarra.o slarrd.o slarrk.o slarnv.o slarrb.o slasq2.o slaset.o slarrf.o slar1v.o slaruv.o slaneg.o slasq3.o slasq4.o slasq5.o slasq6.o slaebz.o \
ssytd2.o slarfg.o slapy2.o \
sorm2l.o slarf.o ilaslc.o ilaslr.o

# MKL version
#
# CFLAGS = -DUSE_MKL -I$(MKLROOT)/include -DNO_PULP
# LDFLAGS = -L${MKLROOT}/lib/intel64 -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -ldl -lm -lrt
# BLAS =
# LAPACK =

# Reference lapack version
#
# LAPACKDIR = $(HOME)/lapack-3.7.0
# CFLAGS = -DUSE_LAPACK -I$(LAPACKDIR)/CBLAS/include -I$(LAPACKDIR)/LAPACKE/include -DNO_PULP
# LDFLAGS = -L$(LAPACKDIR) -llapacke -llapack -lcblas -lrefblas -lgfortran -lm -lrt
# BLAS =
# LAPACK =

OBJ = test.o pca_mrrr.o pca_.o oprecomp.o

.PHONY: build test

build: test_pca

test: test_pca
	./test_pca 9 1

test_pca: $(OBJ) $(BLAS) $(LAPACK)
	$(CC) $(FLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f test_pca *.o *.P

.SECONDARY:

%.o: %.c pca_.h f2c.h mrrr.h fixed_point.h
	$(CC) $(FLAGS) $(CFLAGS) -c $<

oprecomp.o: $(VPATH)../common/oprecomp.c
	$(CC) $(FLAGS) $(CFLAGS) -c $<

# Commented so the GitLab checks do not fail trying to rebuild .c files already present
#
#%.o: %.f
#
#%.c: %.f
#	$(F2C) -R -a $<
#
#%.P: %.f
#	$(F2C) -!c -P -R -a $<
#
#mrrr.h: sstemr.P ssytd2.P sorm2l.P
#	cat $^ > $@

