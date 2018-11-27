# Common deps
SRC=stream.c
INC=../../../include
DEPS=$(INC)/rthms_hooks.h

default: clean all test

all: $(DEPS) $(SRC)
	gcc -mcmodel=medium  -fopenmp -O0 -g -o stream $(SRC) -I$(INC) -DNTIMES=3 -DSTREAM_ARRAY_SIZE=8192000

test: stream
	export OMP_NUM_THREADS=16;./stream
clean:
	rm -rf *.o *.so *.a *~ stream

