#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tests.h"

void pass(const char* message, va_list arglist){
	printf("\033[0;32m");
	printf("[PASSED]");
	printf("\033[0m");
	printf(" - ");

	vprintf(message, arglist);
	printf("\n");
}

void fail(const char* message, va_list arglist){
	printf("\033[0;31m");
	printf("[FAILED]");
	printf("\033[0m");
	printf(" - ");

	vprintf(message, arglist);
	printf("\n");
}

bool runSuite(const char* title, Test* suite){
	printf("--------<| %s |>--------", title);
	bool passed = true;
	int testIndex = 0;
	while((suite + testIndex) != NULL){
		bool result = (*(suite + testIndex)).test();
		if(!result && passed) passed = result; 
	}
	return passed;
}