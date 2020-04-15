#ifndef SCUTE_TESTS_H
#define SCUTE_TESTS_H

#include "common.h"
#include "value.h"
#include "string.h"

#define IS_TRUE(val, message) (val) ? pass(message) : fail(message))
#define IS_FALSE(val, message) (!val) ? pass(message) : fail(message))
#define EQUALS(val1, val2, message) (val1 == val2 ? pass(message) : fail(message))
#define CHARS_EQUAL(array1, array2, message) (strcmp(array1, array2) == 0 ? pass(message) : fail(message))

typedef bool (*TestFn)();
typedef struct{
	TestFn test;
} Test;

extern Test scannerTestSuite[];
extern Test chunkTestSuite[];
extern Test compilerTestSuite[];
extern Test valueTestSuite[];
extern Test objectTestSuite[];

bool fail(const char* message);
bool pass(const char* message);
bool runSuite(const char* message, Test* suite);
#endif