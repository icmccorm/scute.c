#ifndef scute_compiler_h
#define scute_compiler_h
#include "chunk.h"
#include "vm.h"
#include "package.h"

bool compile(char* source, CompilePackage* package);
CompilePackage* currentResult();
#endif