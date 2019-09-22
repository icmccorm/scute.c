CC = gcc
WASMC = emcc
EXEC_FILE = scute

LOCAL_ENTRY = ./src/main.c
EM_ENTRY = ./src/em_main.c

SRC = $(shell find ./src -name "*.c" ! -name "*main.c")
OBJ = $(SRC:%.c=%.o)

EM_FLAGS = --pre-js ./pre.js --js-library ./library.js
EM_JS_FLAGS = -O2 $(EM_JS_EXPORTS) -s WASM=1 -s ENVIRONMENT='worker' -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=InterpreterModule
EM_JS_EXPORTS = -s EXPORTED_FUNCTIONS='["_runCode", "_getShapePointer"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "intArrayFromString", "UTF8ToString"]'

HEAD_PATHS = $(shell find ./src -type f -iname "*.h" -printf "%h\n" | sort -u)

INCLUDES = $(HEAD_PATHS:%=-I %)
FLAGS = $(INCLUDES) -g -lm

.PHONY : clean

%.o : %.c
	@$(CC) $(FLAGS) -c $< -o $@

scute : $(LOCAL_ENTRY) $(SRC) $(OBJ)
	@$(CC) $(FLAGS) $(LOCAL_ENTRY) $(OBJ) -o ./$(EXEC_FILE) -lm

emcc : $(SRC_PATHS) $(EM_ENTRY)
	@$(WASMC) $(FLAGS) $(EM_ENTRY) $(SRC_PATHS) -o ./$(EXEC_FILE).js $(EM_FLAGS) $(EM_JS_FLAGS) ; \
	sed -i "s#'$(EXEC_FILE).wasm'#require('./$(EXEC_FILE).wasm')#g" ./scute.js

clean :	
	rm -rf $(OBJ) *.wasm *.wasm.map ./$(EXEC_FILE)*

