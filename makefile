targets = vm debug value chunk memory compiler scanner
OBJ = $(targets:%=%.o)
DEPS = $(targets:%=%.h)
CC = gcc
MAIN = main.c
FLAGS = -lm

scute: $(OBJ) $(DEPS) $(MAIN)
	$(CC) main.c $(OBJ) $(DEPS) -o scute $(FLAGS)

%.o : %.c 
	$(CC) -c $^ -o $@

all : $(OBJ) scute

clean: 
	rm -rf *.o *.h.gch *.out scute
