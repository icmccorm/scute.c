BUILD_DIR = ./build
CC = gcc
WASMC = emcc
EXEC_FILE = scute

LOCAL_ENTRY = ./src/main.c
EM_ENTRY = ./src/em_main.c

SRC_PATHS = $(shell find ./src -name "*.c" ! -name "*main.c")
OBJ_PATHS = $(notdir $(SRC_PATHS:%.c=%.o))
SHARED_OBJS = $(notdir $(SRC_PATHS:%.c=%.so))
TMP_OBJ_PATHS = $(SHARED_OBJS:%=$(BUILD_DIR)/%)

EM_FLAGS = --pre-js ./pre.js
EM_JS_FLAGS = -O2 $(EM_JS_EXPORTS) -s WASM=1 -s ENVIRONMENT='worker' -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=InterpreterModule
EM_JS_EXPORTS = -s EXPORTED_FUNCTIONS='["_runCode"]'
DEV_FLAGS = -lm -g -shared
PROD_FLAGS = -lm

DEP_PATHS = $(shell find ./src -type f -iname "*.h" -printf "%h\n" | sort -u)

INCLUDES = $(DEP_PATHS:%=-I %)
FLAGS = $(INCLUDES) -lm -g

.PHONY : all clean

$(BUILD_DIR):
	@mkdir $(BUILD_DIR)

ifdef PROD
scute : $(SRC_PATHS)
	@$(CC) $(FLAGS) $(LOCAL_ENTRY) $(SRC_PATHS) -o ./$(EXEC_FILE) -lm
else
scute : $(SRC_PATHS) $(BUILD_DIR) $(TMP_OBJ_PATHS) 
	@$(CC) $(FLAGS) $(LOCAL_ENTRY) $(TMP_OBJ_PATHS) -o ./$(EXEC_FILE) -lm
endif

all: scute

scute.o : $(SRC_PATHS) 
	@$(CC) $(FLAGS) -c $(SRC_PATHS) -o ./scute.o -lm

$(BUILD_DIR)/%.so :
	@$(CC) $(FLAGS) -c $(filter %/$*.c, $(SRC_PATHS)) -o $(@D)/$(notdir $@) $(DEV_FLAGS)

emcc : $(SRC_PATHS) $(EM_ENTRY)
	@$(WASMC) $(FLAGS) $(EM_ENTRY) $(SRC_PATHS) -o ./$(EXEC_FILE).js $(EM_FLAGS) $(EM_JS_FLAGS) ; \
	sed -i "s#'$(EXEC_FILE).wasm'#require('./$(EXEC_FILE).wasm')#g" ./scute.js

clean :	
	rm -rf *.o *.so *.h.gch *.out *.wasm *.wasm.map *.scu ./$(EXEC_FILE)* $(BUILD_DIR)
