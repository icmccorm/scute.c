# thanks to Job Vranish for his tutorial on makefiles for medium sized projects!
# https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

BUILD ?= ./build
SRC_DIR ?= ./src

C_ENTRY = ./src/main.c 
EXEC_FILE = scute

SRC_FILES := $(shell find $(SRC_DIR) -name *.c ! -name "*main.c")
HEAD_FILES := $(shell find $(SRC_DIR) -name *.h)

OBJS := $(SRC_FILES:%=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_HEADS := $(addprefix -I , $(HEAD_FILES))
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

CC = gcc
WASMC = emcc

FLAGS = $(INC_FLAGS) -g
D_FLAGS = -MMD -MP

END_FLAGS = -lm
RM = rm
MKDIR = mkdir

all : ./$(EXEC_FILE)

./$(EXEC_FILE) : $(OBJS) $(C_ENTRY)
	@$(CC) -D DEBUG $(FLAGS) $(C_ENTRY) $(OBJS) -o $(@) $(END_FLAGS)

$(BUILD)/%.c.o : %.c 
	@$(MKDIR) -p $(dir $@)
	@$(CC) -D DEBUG $(FLAGS) $(D_FLAGS) -c $< -o $@ 

.PHONY : clean

clean:
	@$(RM) -r $(BUILD)

-include $(DEPS)

EM_FLAGS = --js-library ./library.js
EM_JS_FLAGS = -O2 $(EM_JS_EXPORTS) -s WASM=1 -s ENVIRONMENT='worker' -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=InterpreterModule
EM_JS_EXPORTS = -s EXPORTED_FUNCTIONS='["_runCode"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "intArrayFromString", "UTF8ToString"]'
EM_ENTRY = ./src/em_main.c

emcc : $(SRC_FILES) $(EM_ENTRY)
	@$(WASMC) $(FLAGS) -D EM_MAIN $(EM_ENTRY) $(SRC_FILES) -o ./$(EXEC_FILE).js $(EM_FLAGS) $(EM_JS_FLAGS) ; \
	sed -i "s#'$(EXEC_FILE).wasm'#require('./$(EXEC_FILE).wasm')#g" ./scute.js

