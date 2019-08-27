BUILD_DIR = ./build
CC = gcc
EXEC_FILE = ./scute

SRC_PATHS = $(shell find ./src -name "*.c")
OBJ_PATHS = $(notdir $(SRC_PATHS:%.c=%.o))
SHARED_OBJS = $(notdir $(SRC_PATHS:%.c=%.so))
TMP_OBJ_PATHS = $(SHARED_OBJS:%=$(BUILD_DIR)/%)

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
	@$(CC) $(FLAGS) $(SRC_PATHS) -o $(EXEC_FILE).prod -lm
else
scute : $(SRC_PATHS) $(BUILD_DIR) $(TMP_OBJ_PATHS) 
	@$(CC) $(FLAGS) $(TMP_OBJ_PATHS) -o $(EXEC_FILE) -lm
endif

all: scute

clean :	
	rm -rf *.o *.so *.h.gch *.out scute* $(BUILD_DIR)

$(BUILD_DIR)/%.so :
	@$(CC) $(FLAGS) -c $(filter %/$*.c, $(SRC_PATHS)) -o $(@D)/$(notdir $@) $(DEV_FLAGS)