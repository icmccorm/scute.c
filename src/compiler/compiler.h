#ifndef scute_compiler_h
#define scute_compiler_h
#include "chunk.h"
#include "vm.h"
#include "package.h"

bool compile(char* source, CompilePackage* package);
CompilePackage* currentResult();

void variable(bool canAssign);
void constant(bool canAssign);
void native(bool canAssign);
void binary(bool canAssign);
void unary(bool canAssign);
void literal(bool canAssign);
void grouping(bool canAssign);
void array(bool canAssign);
void scopeDeref(bool canAssign);
void deref(bool canAssign);
void and_(bool canAssign);
#endif