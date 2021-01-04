#include "common.h"
#include "tests.h"

int main(){
	bool t1 = runSuite("Scanner", scannerTestSuite);
	//bool t2 = runSuite("Values", valueTestSuite);
	return !(t1 && true);
}