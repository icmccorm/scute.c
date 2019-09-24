#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"
#include "svg.h"
#include "output.h"

extern void hello();

void runCode(const char* code){
	initVM();
	InterpretResult result = interpret(code);
	freeVM();

	if(result == INTERPRET_COMPILE_ERROR) exit(65);
	if(result == INTERPRET_RUNTIME_ERROR) exit(70);
}