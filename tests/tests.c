#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tests.h"

bool DEBUG_STACK = false;
int numBytesAllocated = 0;


bool pass(const char* message){
	printf("\033[0;32m");
	printf("[PASSED]");
	printf("\033[0m");
	printf(" - ");

	printf("%s", message);
	printf("\n");
	return true;
}

bool fail(const char* message){
	printf("\033[0;31m");
	printf("[FAILED]");
	printf("\033[0m");
	printf(" - ");

	printf("%s", message);
	printf("\n");
	return false;
}

bool runSuite(const char* title, Test* suite){
	printf("\n--------<| %s |>--------\n", title);
	bool passed = true;
	int testIndex = 1;

	//run setup function
	if(!(*(suite)).test()) return fail("Unable to setup testing environment.");
	while((*(suite + testIndex + 1)).test != NULL){
		Test currentTest = (*(suite + testIndex));
		bool result = (*(suite + testIndex)).test();
		if(!result && passed) passed = result; 
		++testIndex;
	}
	if(!(*(suite + testIndex)).test()) return fail("Unable to teardown testing environment.\n");
	printf("\n");
	return passed;

}



