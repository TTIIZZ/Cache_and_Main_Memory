#include <stdio.h>
#include <stdlib.h>

// **************************************************************************
// 캐시 메모리와 메인 메모리의 데이터 교환을 비트별로 관리하면서 구현하기

// 8-bit address를 이용함.

// 4-way associative cache이며, block size는 4B이고 총 4개의 set으로 구성됨.
// 총 캐시 사이즈는 4B*4*4 = 64B.

// evict를 위해 LRU 알고리즘을 이용함
// double linked list 이용 (LRU block이 가장 뒤에 연결)

// 메인 메모리의 크기는 256B. (8-bit address -> (2^8)B = 256B)
// **************************************************************************

// cache block
// valid: double linked list를 이용하기 때문에 필요 없음.
// dirty: 기존 의미와 같음.
// tag: 8-2-2 = 4. (offset bit 2개, index bit 2개 빼기)
typedef struct CacheBlock
{
	// dirty:1 / tag:4 / data:32 -> 총합 37.
	unsigned char dirty : 1;
	unsigned char tag : 4;
	unsigned long data : 32;

	struct CacheBlock* left;
	struct CacheBlock* right;
} CacheBlock;

typedef struct CacheSet
{
	char num;
	struct CacheBlock* first;
	struct CacheBlock* last;
} CacheSet;

typedef CacheSet* CacheMemory;

typedef char Byte;
typedef short HalfWord;
typedef int Word;
typedef unsigned char Address;

typedef unsigned char* MainMemory;

CacheMemory cache_memory;
MainMemory main_memory;
int access_count;
int hit_count;
int miss_count;

void Read(Address address, int size);
CacheBlock* CacheAccess(Address address, int size);
CacheBlock* MakeBlock(Address address);
void FreeBlock(CacheBlock* temp_block, unsigned char index);
unsigned char GetIndex(Address address);
unsigned char GetTag(Address address);
void PrintCache();
void FreeCache(CacheMemory cache_memory);

int main(void)
{
	// 캐시 메모리와 메인 메모리 할당
	cache_memory = (CacheMemory)calloc(4, sizeof(CacheSet));
	main_memory = (MainMemory)calloc(256, sizeof(Byte));
	for (int i = 0x0; i <= 0xff; i++) main_memory[i] = i;  // address와 data를 같은 2진수로 초기화

	char type;
	int temp_address;
	Address address;
	
	while (1)
	{
		scanf(" %c", &type);
		if (type == 'Q' || type == 'q') break;

		scanf("%x", &temp_address);
		address = temp_address & 0xff;
		switch (type)
		{
			case 'R': case 'r':
				Read(address, 4);
				break;

			/*
			case 'W': case 'w':
				Write(address, 4);
				break;
			*/

			default:
				printf("유효하지 않은 입력입니다.\n");
				
		}
		// PrintCache();
	}

	PrintCache();

	// 캐시 메모리와 메인 메모리 해제
	FreeCache(cache_memory);
	free(main_memory);

	return 0;
}

void Read(Address address, int size)
{
        CacheBlock* block = CacheAccess(address, size);

	int offset = address & 0x3;
	unsigned char data = (block->data >> (24 - 8 * offset)) & 0xff;
        printf("%x\n", data);

	return;
}

// 주어진 address를 이용해 cache 메모리에 접근한 후, cache block의 주소를 반환할 함수.
CacheBlock* CacheAccess(Address address, int size)
{
	access_count++;

        // 주어진 address에서 index와 tag 구하기.
        unsigned char index = GetIndex(address), tag = GetTag(address);

        // 해당하는 set에서의 block의 개수와, 목표로 하는 block
        char num = cache_memory[index].num;
        CacheBlock* ret_block = cache_memory[index].first;

        // 확인 후 다음
        // 마지막 블럭까지 이동 (마지막 블럭의 확인은 X)
        for (int i = 0; i < num - 1; i++)
        {
                if (ret_block->tag == tag) break;
		ret_block = ret_block->right;
        }

        // 블럭이 하나도 없는 경우 -> 새로 만들어서 set에 연결하기
        if (!ret_block)
        {
		miss_count++;

                ret_block = MakeBlock(address);
                cache_memory[index].first = ret_block;
		cache_memory[index].last = ret_block;
		cache_memory[index].num++;

                return ret_block;
        }

	// 블럭이 존재하는 경우

	// 태그가 일치하면 해당하는 블럭을 맨 앞으로 이동 후 리턴
	if (ret_block->tag == tag)
	{
		hit_count++;

		// 이미 맨 앞에 있는 경우 바로 리턴
		if (!ret_block->left) return ret_block;

		// 앞 뒤 블럭을 연결
		ret_block->left->right = ret_block->right;
		if (ret_block->right) ret_block->right->left = ret_block->left;  // 뒤 블럭이 있는 경우에만
		
		// 맨 앞으로 이동
		if (!ret_block->right) cache_memory[index].last = ret_block->left;  // 태그가 일치하는 블럭이 맨 마지막 블럭이었을 경우, last를 앞의 블럭으로 갱신
		ret_block->left = NULL;
		ret_block->right = cache_memory[index].first;
		ret_block->right->left = ret_block;
		cache_memory[index].first = ret_block;

		return ret_block;
	}

	miss_count++;
	
	// 태그가 일치하지 않으면 새로운 데이터 가져온 후 맨 앞에 연결
	// 원래 개수가 3개 이하였다면 num을 1 늘리고, 4개였다면 맨 마지막 블럭 제거
	CacheBlock* new_block = MakeBlock(address);
	new_block->right = cache_memory[index].first;
	new_block->right->left = new_block;
	cache_memory[index].first = new_block;

	if (cache_memory[index].num == 4)
	{
		ret_block->left->right = NULL;
		cache_memory[index].last = ret_block->left;
		FreeBlock(ret_block, index);
	}
	else cache_memory[index].num++;

	return new_block;
}

// 주어진 address를 이용해 새로운 cache block를 만들 함수
CacheBlock* MakeBlock(Address address)
{
        CacheBlock* new_block = (CacheBlock*)calloc(1, sizeof(CacheBlock));
        new_block->tag = GetTag(address);

        Address base_address = address & 0xfc;
	for (int i = 0; i < 4; i++) new_block->data += main_memory[base_address + i] << (24 - 8 * i);

        return new_block;
}

// 블럭의 메모리를 해제할 함수 (evict)
void FreeBlock(CacheBlock* temp_block, unsigned char index)
{
	// dirty bit가 1인 경우 데이터를 메인 메모리에 덮어쓰기
	if (temp_block->dirty)
	{
		Address base_address = (temp_block->tag << 4) + (index << 2);
		for (int i = 0; i < 4; i++) main_memory[base_address + i] = (temp_block->data >> (24 - 8 * i)) & 0xff;
	}
	free(temp_block);
}

// 주어진 address로 cache memory에서의 set index를 찾을 hash function.
unsigned char GetIndex(Address address)
{
        return (address >> 2) & 0x03;
}

// 주어진 address로 tag를 찾을 hash function.
unsigned char GetTag(Address address)
{
        return (address >> 4) & 0xf;
}

// 현재 캐시 메모리의 데이터를 프린트할 함수
void PrintCache()
{
	printf("\n");
	printf("***** Current Cache Status *****\n");
	printf("\n");

	// 각 set의 block의 개수를 출력 후, 블럭의 원소를 하나하나 프린트하기
	for (int i = 0; i < 4; i++)
        {
                char num = cache_memory[i].num;
		printf("Set %d has %d blocks.\n", i, num);

		CacheBlock* now_block = cache_memory[i].first;	
                for (char j = 0; j < num; j++)
                {
			printf("dirty:%d / tag:%x / data:%x\n", now_block->dirty, now_block->tag, now_block->data);
                	now_block = now_block->right;
                }
		printf("\n");
        }
	
	printf("access count: %d / hit count: %d / miss count : %d\n", access_count, hit_count, miss_count);
	printf("\n");

	printf("********************************\n");
	printf("\n");
}

// cache 메모리를 해제할 함수
void FreeCache(CacheMemory cache_memory)
{
        for (int i = 0; i < 4; i++)
        {
                char num = cache_memory[i].num;
                for (char j = 0; j < num; j++)
                {
                        CacheBlock* now_block = cache_memory[i].first;
                        cache_memory[i].first = now_block->right;
                        free(now_block);
                }
        }

	free(cache_memory);
}
