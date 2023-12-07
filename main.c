#include <stdio.h>
#include <stdlib.h>

typedef struct cache_block
{
	unsigned valid : 1;
	unsigned tag : 2;
	unsigned count : 2;
	unsigned data : 64;
} cache_block;
