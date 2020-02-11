#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"
#include "hashmap.h"
#include "obj.h"
#include "output.h"
#include "package.h"

bool DEBUG_STACK = false;

int numBytesAllocated = 0;

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

static void runFile(const char* path){
	#ifndef EM_MAIN
		print(O_OUT, "[A] before init: %d bytes\n", numBytesAllocated);
	#endif

	char* source = readFile(path);

	CompilePackage* compiled = initCompilationPackage();
	
	#ifndef EM_MAIN
		print(O_OUT, "[A] after init: %d bytes\n", numBytesAllocated);
	#endif

	runCompiler(compiled, source);
	InterpretResult result = interpretCompiled(compiled, -1);
	
	freeCompilationPackage(compiled);


	int length = strlen(source) + 1;
	FREE_ARRAY(char, source, length);
	#ifndef EM_MAIN
		print(O_OUT, "[A] after fcompile: %d bytes\n", numBytesAllocated);
	#endif
	if(result == INTERPRET_COMPILE_ERROR) exit(65);
	if(result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]){
	const char* sFlag = "-s";

	switch(argc){
		case 2: ;
			runFile(argv[1]);
			break;
		case 3: ;
			const char* flag = argv[2];
			if(memcmp(sFlag, flag, 2) == 0){
				DEBUG_STACK = true;
				runFile(argv[1]);
			}else{
				print(O_ERR, "Usage: scute [path] [-s]\n");
			}
			break;
		default: ;
			print(O_ERR, "Usage: scute [path] [-s]\n");
			exit(64);
			break;
	}
	return 0;
}