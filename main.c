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
typedef struct cache_block
{
	// valid:1 / dirty:1 / count:2 / tag:4 / data:32 -> 총합 40.
	unsigned long long data : 40;
} cache_block;
typedef struct cache_block* cache_set;
typedef struct cache_block** cache_memory;

typedef char byte;
typedef short halfword;
typedef int word;

typedef word* main_memory;

cache_memory Cache;
main_memory Main;

int main(void)
{
	Cache = (cache_memory)calloc(4, sizeof(cache_set));
	for (int i = 0; i < 4; i++) Cache[i] = (cache_set)calloc(4, sizeof(cache_block));
	Main = (main_memory)calloc(256, sizeof(byte));

	printf("Project Start!\n");
	Cache[0][0].data = 0xfffff;
	printf("%d\n", (word)(Cache[0][0].data & 0xffff));
	Cache[0][0].data = Cache[0][0].data & 0xf0000;
	printf("%d\n", (word)(Cache[0][0].data & 0xffff));
	Cache[0][0].data = Cache[0][0].data | 0x0f0f0;
	printf("%d\n", (word)(Cache[0][0].data & 0xffff));

	for (int i = 0; i < 4; i++) free(Cache[i]);
	free(Cache);
	free(Main);

	return 0;
}
