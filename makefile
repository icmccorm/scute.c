# thanks to Job Vranish for his tutorial on makefiles for medium sized projects!
# https://spin.atomicobject.com/2016/08/26/makefile-c-projects/


CC = clang
WASMC = emcc

D_FLAGS = -MMD -MP   

END_FLAGS = -lm
RM = rm
MKDIR = mkdir

EXEC_FILE = scute
EXEC_TEST_FILE = scute-test

BUILD ?= ./build
SRC_DIR ?= ./src
TEST_DIR ?= ./tests

C_ENTRY ?= ./src/main.c 
TEST_ENTRY ?= ./tests/test-main.c

SRC_FILES := $(shell find $(SRC_DIR) -name *.c ! -name "*main.c")
HEAD_FILES := $(shell find $(SRC_DIR) -name *.h)

TEST_FILES := $(shell find $(TEST_DIR) -name *.c ! -name "*main.c")
INC_TEST_DIRS := $(shell find $(TEST_DIR) -type d)
INC_TEST_FLAGS :=  $(addprefix -I, $(INC_TEST_DIRS)) 
TEST_OBJS := $(TEST_FILES:%=$(BUILD)/%.o)

OBJS := $(SRC_FILES:%=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I, $(INC_DIRS)) 

all : ./$(EXEC_FILE) test emcc

./$(EXEC_FILE) : $(OBJS) $(C_ENTRY)
	@$(CC) -g -D DEBUG $(INC_FLAGS) $(C_ENTRY) $(OBJS) -o $(@) $(END_FLAGS)

$(BUILD)/%.c.o : %.c 
	@$(MKDIR) -p $(dir $@)
	@$(CC) -g -D DEBUG $(INC_TEST_FLAGS) $(INC_FLAGS)  $(D_FLAGS) -c $< -o $@

.PHONY : clean

clean:
	@$(RM) -r $(BUILD)
	@$(RM) *.wasm $(EXEC_FILE).js $(EXEC_FILE) $(EXEC_TEST_FILE)
	
-include $(DEPS)

EM_FLAGS = --js-library ./library.js --pre-js ./pre.js -O1
EM_JS_FLAGS = $(EM_JS_EXPORTS) -s ASSERTIONS=3 -s WASM=1 -s MODULARIZE=1 -s STRICT=1 -s FILESYSTEM=0 -s EXPORT_ES6=1 -s USE_ES6_IMPORT_META=0 -s ENVIRONMENT='worker' -s EXPORT_NAME="'Scute'"
EM_JS_EXPORTS = -s EXPORTED_FUNCTIONS='["_runCode", "_compileCode", "_freeCompilationPackage"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "intArrayFromString", "UTF8ToString"]' -s ALLOW_MEMORY_GROWTH=1
EM_ENTRY = ./src/em_main.c

emcc : $(SRC_FILES) $(EM_ENTRY)
	@$(WASMC) $(EM_MAP) $(EM_FLAGS) $(EM_JS_FLAGS) -g $(INC_FLAGS) -D EM_MAIN $(EM_ENTRY) $(SRC_FILES) -o ./$(EXEC_FILE).js 

test :  $(OBJS) $(TEST_OBJS) $(TEST_ENTRY)
	@$(CC) $(TEST_ENTRY) $(TEST_OBJS) $(OBJS) $(INC_FLAGS) $(INC_TEST_FLAGS) -o $(EXEC_TEST_FILE)