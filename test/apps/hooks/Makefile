CC=g++
CFLAGS= -O3 -g -mcmodel=medium -fopenmp
CFLAGS += -DNTIMES=3 -DSTREAM_ARRAY_SIZE=1024

INC=../../../include
DEPS=$(INC)/rthms_hooks.h

EXE=rthms_hooks
SRC_DIR=.
OBJ_DIR=.
SRC=$(filter-out $(SRC_DIR)/$(EXE).cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJ=$(patsubst   $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

default: clean all test

all:$(DEPS) $(EXE).cpp
	$(CC) $(CFLAGS) -o $(EXE) $(EXE).cpp -I$(INC)

$(OBJ_DIR)/%o:$(SRC_DIR)/%c
	${CC} ${CFLAGS} $< -c -o $@

test: $(EXE)
	export OMP_NUM_THREADS=16;./$(EXE)

clean:
	rm -rf *.o *.so *.a *~ $(EXE)

