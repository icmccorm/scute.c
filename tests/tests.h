#ifndef scute_tests_h
#define scute_tests_h

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct {
	const char* description;
} TestSuite;

void assertTrue(const char* that, bool isTrue, const char* orElsePrint);

void assertCharEquals(const char* that, char* stringOne, char* stringTwo, const char* orElsePrint);

#endif