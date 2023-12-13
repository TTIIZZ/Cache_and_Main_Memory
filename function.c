#include <stdio.h>

typedef struct CacheBlock
{
        // valid:1 / dirty:1 / count:2 / tag:4 / data:32 -> 총합 40.
        unsigned long long data : 40;
} CacheBlock;
typedef CacheBlock* CacheSet;
typedef CacheSet* CacheMemory;

typedef char Byte;
typedef short HalfWord;
typedef int Word;
typedef unsigned char Address;

typedef unsigned char* MainMemory;

void CacheAccess(CacheMemory cache_memory, Address address);
unsigned char GetIndex(Address address);

// 주어진 address를 이용해 cache 메모리에 접근할 함수.
void CacheAccess(CacheMemory cache_memory, Address address)
{
	printf("%x\n", address);
	printf("%x\n", (Word)cache_memory[GetIndex(address)][0].data & 0xffffffff);
}

// cache memory에서의 set index를 찾을 hash function.
unsigned char GetIndex(Address address)
{
	return (address >> 2) & 0x03;
}
