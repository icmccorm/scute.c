#ifndef scute_memory_h
#define scute_memory_h

#include "common.h"

#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * 1.5)

#define GROW_ARRAY(array, type, oldCount, newCount) \
	(type*) reallocate(array, sizeof(type) * (oldCount), \
		sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
	reallocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, count) \
	(type*) reallocate(NULL, 0, sizeof(type)*count)

#define FREE(type, pointer) \
	reallocate(pointer, sizeof(type), 0)

void* reallocate(void* previous, size_t oldSize, size_t newSize);

#endif
