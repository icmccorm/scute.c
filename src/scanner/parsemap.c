#include "parsemap.h"
#include "compiler_defs.h"
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
void stringLiteral(bool canAssign);
void and_(bool canAssign);


ParseRule rules[] = {
	{variable, NULL, PC_NONE},		//TK_ID
	{constant, NULL, PC_NONE},		//TK_CONSTANT
	{native, NULL, PC_NONE},		//TK_SHAPE
	{native, NULL, PC_NONE},		//TK_NATIVE
	{NULL, binary, PC_TERM},		//TK_PLUS
	{unary, binary, PC_TERM},		//TK_MINUS
	{NULL, binary, PC_FACTOR},		//TK_TIMES
	{NULL, binary, PC_FACTOR},		//TK_DIVIDE
	{NULL, binary, PC_FACTOR},		//TK_MODULO
	{NULL, binary, PC_EQUALS},		//TK_EQUALS
	{NULL, binary, PC_EQUALS},		//TK_BANG_EQUALS
	{NULL, binary, PC_COMPARE},		//TK_LESS_EQUALS
	{NULL, binary, PC_COMPARE},		//TK_GREATER_EQUALS
	{NULL, binary, PC_COMPARE},		//TK_LESS
	{NULL, binary, PC_COMPARE},		//TK_GREATER
	{NULL, binary, PC_ASSIGN},		//TK_ASSIGN
	{NULL, binary, PC_ASSIGN},		//TK_INCR_ASSIGN
	{NULL, binary, PC_ASSIGN},		//TK_DECR_ASSIGN
	{unary, NULL, PC_UNARY},		//TK_BANG
	{unary, NULL, PC_UNARY},		//TK_INCR
	{unary, NULL, PC_UNARY},		//TK_DECR
	{NULL, binary, PC_POWER},		//TK_POWER
	{literal, NULL, PC_NONE},		//TK_REAL
	{literal, NULL, PC_NONE},		//TK_INTEGER
	{grouping, NULL, PC_NONE},		//TK_L_PAREN
	{array, NULL, PC_NONE},		//TK_L_BRACK
	{scopeDeref, deref, PC_CALL},		//TK_DEREF
	{stringLiteral, NULL, PC_NONE},		//TK_STRING
	{NULL, and_, PC_AND},		//TK_AND
	{literal, NULL, PC_NONE},		//TK_FALSE
	{literal, NULL, PC_NONE},		//TK_TRUE
	{literal, NULL, PC_NONE},		//TK_NULL
};