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

all : scanner ./$(EXEC_FILE) test web node

./$(EXEC_FILE) : $(OBJS) $(C_ENTRY) ./src/scanner/constants.txt ./src/scanner/keywords.txt ./autoscanner.py
	@$(CC) -g -D DEBUG $(INC_FLAGS) $(C_ENTRY) $(OBJS) -o $(@) $(END_FLAGS)

$(BUILD)/%.c.o : %.c 
	@$(MKDIR) -p $(dir $@)
	@$(CC) -g -D DEBUG $(INC_TEST_FLAGS) $(INC_FLAGS) $(D_FLAGS) -c $< -o $@

.PHONY : clean
 
scanner:
	python3 autoscanner.py -d ./src/scanner -c constants.txt -k keywords.txt 

/scanner/parsemap.c /scanner/parsemap.h /scanner/tokenizer.c /scanner/tokenizer.h: 
	python3 autoscanner.py -d ./src/scanner -c constants.txt -k keywords.txt 

clean:
	@$(RM) -r $(BUILD)
	@$(RM) *.wasm *.map $(EXEC_FILE).js $(EXEC_FILE) $(EXEC_TEST_FILE) $(EXEC_FILE)-test.js
	

-include $(DEPS)

EM_FLAGS = --js-library ./library.js --extern-pre-js ./library-interop.js --pre-js ./pre.js
EM_WEB_FLAGS = $(EM_EXPORTS) -s ASSERTIONS=1 -s SAFE_HEAP=1 -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s STRICT=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s USE_ES6_IMPORT_META=0  -s EXPORT_NAME="'scute'" -s FILESYSTEM=0 -s ENVIRONMENT='worker'
EM_NODE_FLAGS = $(EM_EXPORTS) -O0 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT='node' -s EXPORT_NAME="'scute'" -s USE_ES6_IMPORT_META=0
EM_EXPORTS = -s EXPORTED_FUNCTIONS='["_free", "_malloc", "_runCode", "_compileCode", "_freeCompilationPackage", "_renderAnimationBlocks"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["intArrayFromString", "ccall", "UTF8ToString"]'
EM_ENTRY = ./src/em_main.c

web : $(SRC_FILES) $(EM_ENTRY)
	@$(WASMC) $(EM_MAP) $(EM_FLAGS) $(EM_WEB_FLAGS) $(INC_FLAGS) -D EM_MAIN $(EM_ENTRY) $(SRC_FILES) -o ./$(EXEC_FILE).js

web-prod: $(SRC_FILES) $(EM_ENTRY)
	@$(WASMC) $(EM_MAP) $(EM_FLAGS) -O3 $(EM_WEB_FLAGS) $(INC_FLAGS) -D EM_MAIN $(EM_ENTRY) $(SRC_FILES) -o ./$(EXEC_FILE).js 

node: $(SRC_FILES) $(EM_ENTRY)
	@$(WASMC) $(EM_MAP) $(EM_FLAGS) $(EM_NODE_FLAGS) -g $(INC_FLAGS) -D EM_MAIN $(EM_ENTRY) $(SRC_FILES) -o ./$(EXEC_FILE)-test.js 

test :  $(OBJS) $(TEST_OBJS) $(TEST_ENTRY)
	@$(CC) $(TEST_ENTRY) $(TEST_OBJS) $(OBJS) $(INC_FLAGS) $(INC_TEST_FLAGS) -o $(EXEC_TEST_FILE)

make grind: ./$(EXEC_FILE)
	valgrind --leak-check=full --track-origins=yes ./$(EXEC_FILE) ./test 