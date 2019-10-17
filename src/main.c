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

static char* readFile(const char* path){
	FILE* file = fopen(path, "rb");
	
	if(file == NULL){
		print(O_ERR, "Could not open file\"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);
	
	char* buffer = (char*) malloc(fileSize + 1);

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
	char* source = readFile(path);
	CompilePackage* compiled = initCompilationPackage();
	
	runCompiler(compiled, source);
	InterpretResult result = interpretCompiled(compiled, -1);
	
	freeCompilationPackage(compiled);
	free(source);

	if(result == INTERPRET_COMPILE_ERROR) exit(65);
	if(result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]){
	if(argc == 2) {
		runFile(argv[1]);
	} else {
		print(O_ERR, "Usage: scute [path]\n");
		exit(64);
	}
	return 0;
}