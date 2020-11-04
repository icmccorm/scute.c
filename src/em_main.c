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
#include "package.h"

void runCode(CompilePackage* code){
	InterpretResult result = interpretCompiled(code, 0);
}

CompilePackage* compileCode(const char* code){
	CompilePackage* package = initCompilationPackage();
	runCompiler(package, (char*) code);
	return package;
}
