
scute : memory.o chunk.o debug.o main.c
	gcc -o scute memory.o chunk.o debug.o main.c

memory.o: memory.c memory.h common.h
	gcc -c memory.c
chunk.o: chunk.c chunk.h memory.h common.h
	gcc -c chunk.c
debug.o: debug.c debug.h chunk.h
	gcc -c debug.c

clean:
	rm scute *.o
