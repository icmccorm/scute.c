#include "tests.h"
#include "scanner.h"

bool ensureInit (){
	char code[] = "hello world!";
	initScanner(code);
	if(scanner.line != 1) return fail("Scanner line counter not properly set.");
	if(scanner.lastNewline != NULL) return fail("Scanner's last newline should initially be NULL.");
	if(scanner.origin != code) return fail("Scanner's origin not properly set.");
	if(scanner.start != scanner.origin) return fail("Scanner's start should point to the origin.");
	if(scanner.current != scanner.origin) return fail("Scanner's current should point to the origin.");
	return pass("Scanner is correctly initialized");
}

Test scannerTestSuite[] = {
	{ensureInit, "initScanner()"},
	{NULL, ""}
};

