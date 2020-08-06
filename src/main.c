#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"
#include "hashmap.h"
#include "obj.h"
#include "output.h"
#include "package.h"
#include "scanner.h"

bool DEBUG_STACK = false;

#ifndef EM_MAIN
	int numBytesAllocated = 0;
#endif

static char* readFile(const char* path){
	FILE* file = fopen(path, "rb");
	
	if(file == NULL){
		print(O_ERR, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);
	
	char* buffer = GROW_ARRAY(NULL, char, 0, fileSize + 1);

	if(buffer == NULL){
		print(O_ERR, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}
	
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

	if(bytesRead < fileSize){
		print(O_ERR, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}
	
	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}

static InterpretResult runCode(char* source){
	CompilePackage* compiled = initCompilationPackage();
	
	#ifndef EM_MAIN
		printMem("after init");
	#endif

	runCompiler(compiled, source);
	InterpretResult result = interpretCompiled(compiled, -1);
	
	freeCompilationPackage(compiled);

	int length = strlen(source) + 1;
	FREE_ARRAY(char, source, length);

	#ifndef EM_MAIN
		printMem("after fcompile");
	#endif

	return result;
}

static InterpretResult runFile(const char* path){
	#ifndef EM_MAIN
		printMem("before init");
	#endif
	char* source = readFile(path);
	return runCode(source);
}

static InterpretResult runInput(){
	#ifndef EM_MAIN
		printMem("before init");
	#endif

	#define CHUNK (256)
	char* source = NULL;
	char* buffer = GROW_ARRAY(NULL, char, 0, CHUNK);

	if(buffer == NULL){
		print(O_ERR, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}

	FILE* file = stdin;
	if(file == NULL){
		print(O_ERR, "Could not receive from standard input.\n");
		exit(74);
	}

	int currentSourceLength = 0;
	int currentBuffLength = 0;
	do{
		fgets(buffer, CHUNK, stdin);
		currentBuffLength = strlen(buffer);

		source = reallocate(source, currentSourceLength, currentSourceLength + currentBuffLength); //void* reallocate(void* previous, size_t oldSize, size_t newSize) {
		
		if(buffer == NULL){
			print(O_ERR, "Not enough memory to read \"%s\".\n", path);
			exit(74);
		}
		
		strcpy(source + currentSourceLength, buffer);

		currentSourceLength += currentBuffLength;

	}while(currentBuffLength == CHUNK-1 && buffer[CHUNK-2]!='\n');
	
	InterpretResult result = runCode(buffer);

	return result;
}

int main(int argc, const char* argv[]){
	const char* sFlag = "-s";
	InterpretResult result = INTERPRET_OK;
	switch(argc){
		case 1: ;
			if(!isatty(STDIN_FILENO)) {
				result = runInput();
			}else{
				print(O_ERR, "Usage: scute [path] [-s], OR <input> | ./scute\n");
				exit(64);

			}
			break;
		case 2: ;
			result = runFile(argv[1]);
			break;
		case 3: ;
			const char* flag = argv[2];
			if(memcmp(sFlag, flag, 2) == 0){
				DEBUG_STACK = true;
				result = runFile(argv[1]);
			}else{
				print(O_ERR, "Usage: scute [path] [-s], OR <input> | ./scute\n");
				exit(64);
			}
			break;
		default: ;
			print(O_ERR, "Usage: scute [path] [-s], OR <input> | ./scute\n");
			exit(64);
			break;
	}	
	if(result == INTERPRET_COMPILE_ERROR) exit(65);
	if(result == INTERPRET_RUNTIME_ERROR) exit(70);
	return 0;
}