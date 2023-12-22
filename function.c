#include <stdio.h>

typedef struct CacheBlock
{
        // valid:1 / dirty:1 / lru:2 / tag:4 / data:32 -> 총합 40.
        unsigned char valid : 1;
        unsigned char dirty : 1;
        unsigned char lru : 2;
        unsigned char tag : 4;
        unsigned long data : 32;
} CacheBlock;
typedef CacheBlock* CacheSet;
typedef CacheSet* CacheMemory;

typedef char Byte;
typedef short HalfWord;
typedef int Word;
typedef unsigned char Address;

typedef unsigned char* MainMemory;

void CacheAccess(CacheMemory cache_memory, Address address, int size);
unsigned char GetIndex(Address address);
unsigned char GetTag(Address address);

// 주어진 address를 이용해 cache 메모리에 접근할 함수.
void CacheAccess(CacheMemory cache_memory, Address address, int size)
{
	printf("%x\n", address);
	unsigned char tag = GetTag(address);
	cache_memory[GetIndex(address)][0].tag = tag;
	printf("%x\n", cache_memory[GetIndex(address)][0].tag);
	printf("%x\n", cache_memory[GetIndex(address)][0].data);
}

// cache memory에서의 set index를 찾을 hash function.
unsigned char GetIndex(Address address)
{
	return (address >> 2) & 0x03;
}

unsigned char GetTag(Address address)
{
	return (address >> 4) & 0xf;
}
