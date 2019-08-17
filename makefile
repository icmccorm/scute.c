targets = debug value chunk memory
OBJ = $(targets:%=%.o)
DEPS = $(targets:%=%.h)
CC = gcc
MAIN = main.c

scute: $(OBJ) $(DEPS) $(MAIN)
	$(CC) main.c $(OBJ) $(DEPS) -o scute

%.o : %.c 
	$(CC) -c $^ -o $@

all : $(OBJ) scute

clean: 
	rm -rf *.o *.h.gch *.out scute