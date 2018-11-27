SHELL:=/bin/bash
CC=g++
PIN_DIR=/home/1bp/Software/pin-3.5-97503-gac534ca30-gcc-linux
SRC_DIR=src
OBJ_DIR=obj
INC_DIR=include
SOURCES=$(filter-out $(SRC_DIR)/rthms_profiler.cpp  $(SRC_DIR)/rthms_parser.cpp , $(wildcard $(SRC_DIR)/*.cpp))
OBJECTS=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
EXE=RTHMS
EXE_test=rthms_test
PARSER=rthms_parser
USR_FLAGS=-DPIN -DVERBOSE #-DCPU_TRACE #-DDEBUG #-DDEBUG_CACHE -DF77 -DCLASS_C 


CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-unused-variable -D__PIN__=1 -DPIN_CRT=1 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2  -I${PIN_DIR}/source/include/pin -I${PIN_DIR}/source/include/pin/gen -isystem ${PIN_DIR}/extras/stlport/include -isystem ${PIN_DIR}/extras/libstdc++/include -isystem ${PIN_DIR}/extras/crt/include -isystem ${PIN_DIR}/extras/crt/include/arch-x86_64 -isystem ${PIN_DIR}/extras/crt/include/kernel/uapi -isystem ${PIN_DIR}/extras/crt/include/kernel/uapi/asm-x86 -I${PIN_DIR}/extras/components/include -I${PIN_DIR}/extras/xed-intel64/include/xed -I${PIN_DIR}/source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing ${USR_FLAGS} -I${INC_DIR}


CLINKS=-shared -Wl,--hash-style=sysv ${PIN_DIR}/intel64/runtime/pincrt/crtbeginS.o -Wl,-Bsymbolic -Wl,--version-script=${PIN_DIR}/source/include/pin/pintool.ver -fabi-version=2 -L${PIN_DIR}/intel64/runtime/pincrt -L${PIN_DIR}/intel64/lib -L${PIN_DIR}/intel64/lib-ext -L${PIN_DIR}/extras/xed-intel64/lib -lpin -lxed ${PIN_DIR}/intel64/runtime/pincrt/crtendS.o -lpin3dwarf  -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic


default:${EXE}


${EXE}:$(SRC_DIR)/rthms_profiler.cpp ${OBJECTS}
	${CC} ${CFLAGS} -o $@ $(SRC_DIR)/rthms_profiler.cpp  ${OBJECTS}  ${CLINKS}
	@(echo "#########################")
	@(echo "Compilation is done!")
	@(echo "Run 'simple tests located in /test/scripts'")
	@(echo "#########################")

$(OBJ_DIR)/%o:$(SRC_DIR)/%cpp
	@mkdir -p $(OBJ_DIR)
	${CC} ${CFLAGS} $< -c -o $@

parser:$(SRC_DIR)/rthms_parser.cpp ${OBJECTS}
	${CC} -o $@  $(SRC_DIR)/rthms_parser.cpp


$(EXE_test):
	${CC} -fopenmp -O0 -g -o $@  test/$@.cpp -I$(SRC_DIR)

clean:
	rm -rf *.o  *.so *~ *.exe ${EXE} $(EXE_test) pintrace.* *.log ${OBJ_DIR} parser
