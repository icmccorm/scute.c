DEPS = ./src/headers
HEADERS = $(shell find ./src -name "*.h")
SRC = $(shell find ./src -name "*.c")
OBJ = $(SRC:%.c=%.o)
DEP_DIRS = $(shell find ./src -type f -iname "*.h" -printf "%h\n" | sort -u)
INCLUDES = $(DEP_DIRS:%=-I %)
FLAGS = $(INCLUDES)
CC = gcc

.PHONY : all clean
.SILENT : scute

test:
	echo $(SOURCES)

scute : $(SRC) 
	$(CC) $(FLAGS) $^ main.c -o ./scute -lm

all: scute tmp

tmp:
	mkdir tmp

clean :	
	rm -rf *.o *.h.gch *.out scute
