#include <stdio.h>
#include <stdlib.h>

// **************************************************************************
// 캐시 메모리와 메인 메모리의 데이터 교환을 비트별로 관리하면서 구현하기

// 8-bit address를 이용함.

// 4-way associative cache이며, block size는 4B이고 총 4개의 set으로 구성됨.
// 총 캐시 사이즈는 4B*4*4 = 64B.

// 메인 메모리의 크기는 256B. (8-bit address -> 2^8 = 256)
// **************************************************************************

// cache block
// valid, dirty: 기존 의미와 같음.
// count: LRU를 위한 bit. 2bits 할당.
// tag: 8-2-2 = 4. (offset bit 2개, index bit 2개 빼기)
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

CacheMemory cache_memory;
MainMemory main_memory;

void CacheAccess();
unsigned char GetHash();

int main(void)
{
	cache_memory = (CacheMemory)calloc(4, sizeof(CacheSet));
	for (int i = 0; i < 4; i++) cache_memory[i] = (CacheSet)calloc(4, sizeof(CacheBlock));
	main_memory = (MainMemory)calloc(256, sizeof(Byte));

	CacheAccess(0x0c);

	for (int i = 0; i < 4; i++) free(cache_memory[i]);
	free(cache_memory);
	free(main_memory);

	return 0;
}
