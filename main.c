#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// **************************************************************************
// 캐시 메모리 2개와 메인 메모리의 데이터 교환 구현

// 16-bit address를 이용함.

// cache memory의 size는 매 실행마다 입력

// evict를 위해 LRU 알고리즘을 이용함
// double linked list 이용 (LRU block이 가장 뒤에 연결)

// 메인 메모리의 크기는 64MB. (16-bit address -> (2^16)B = (2^6 * 2 ^10)B = 64MB)
// **************************************************************************

// cache block
// valid: double linked list를 이용하기 때문에 필요 없음.
// dirty: 기존 의미와 같음.
// tag는 최대 16bit이므로 unsigned char 이용
// data의 size는 매 실행 달라지므로 그때그때 동적 할당
typedef struct CacheBlock
{
	unsigned char dirty : 1;
	unsigned short tag;
	char* data;

	struct CacheBlock* left;
	struct CacheBlock* right;
} CacheBlock;

// cache set
// num: 해당 set의 block의 수와
// first: 바로 연결된 블럭(가장 최근에 사용한 블럭)
// last: 가장 먼 블럭(사용한지 가장 오래된 블럭)
typedef struct CacheSet
{
	int num;
	struct CacheBlock* first;
	struct CacheBlock* last;
} CacheSet;

// cache memory의 정보
typedef struct CacheMemory
{
	int block_size;
	int block_num;
	int set_num;
	int cache_size;
	int tag_bit, index_bit, offset_bit;
	CacheSet* set;
} CacheMemory;

CacheMemory cache_memory[3];

// main memory
typedef unsigned short Address;
typedef char* MainMemory;
MainMemory main_memory;

int access_count;
int hit_count;
int miss_count;

int MakeCache(int level);
void Read(Address address, int size);
CacheBlock* CacheAccess(Address address, int size);
CacheBlock* MakeBlock(Address address);
void FreeBlock(CacheBlock* temp_block, unsigned char index);
unsigned char GetIndex(Address address);
unsigned char GetTag(Address address);
void PrintCache();
void FreeCache(int level);

int main(void)
{
	// 캐시 메모리의 정보 입력 후 메모리 생성 -> 잘못된 사이즈 입력 시 다시 입력
	while (1)
	{
		char execution;
		if (isatty(0)) printf("Will you start a new execution? [Y / N]: ");
		scanf(" %c", &execution);
		switch (execution)
		{
			case 'Y': case 'y':
				break;

			case 'N': case 'n':
				execution = 0;
				break;

			default:
				printf("Wrong input!\n");
				continue;
		}
		if (!execution) break;

		while (!MakeCache(1)) break;
		while (!MakeCache(2)) break;

/*

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

				
				case 'W': case 'w':
					Write(address, 4);
					break;
				

				default:
					printf("유효하지 않은 입력입니다.\n");
				
			}
			// PrintCache();
		}

		PrintCache();
*/

		// 캐시 메모리와 메인 메모리 해제
		FreeCache(1);
		FreeCache(2);
		//free(main_memory);

	}

	return 0;
}

// 캐시 메모리를 생성할 함수
// level 변수를 통해 level 1, 2 캐시메모리를 구분
// 블럭의 사이즈, 한 셋의 블럭 수, 셋의 개수를 입력받은 후 유효한 입력이라면 메모리 할당
// 입력한 수가 2의 제곱수인지 확인, 총 캐시 사이즈를 구했을 때 메인메모리보다 작은지 확인
int MakeCache(int level)
{
	// level에 따라 만들 캐시메모리 결정
	if (level == 1) printf("Let's make a level 1 cache memory!\n");
	else printf("Let's make a level 2 cache memory!\n");

	// block size 확인
	if (isatty(0)) printf("Please input the size of cache block: ");
        scanf("%d", &cache_memory[level].block_size);
	cache_memory[level].offset_bit = 0;
        for (int i = cache_memory[level].block_size; i > 1; i /= 2, cache_memory[level].offset_bit++)
        {
        	if (i % 2 == 1) cache_memory[level].block_size = 0;
	}

	// block 개수 확인
        if (isatty(0)) printf("Please input the number of cache block in one set: ");
        scanf("%d", &cache_memory[level].block_num);
	for (int i = cache_memory[level].block_num; i > 1; i /= 2)
	{
		if (i % 2 == 1) cache_memory[level].block_num = 0;
	}

	// set 개수 확인
	if (isatty(0)) printf("Please input the number of cache set: ");
	scanf("%d", &cache_memory[level].set_num);
	cache_memory[level].index_bit = 0;
	for (int i = cache_memory[level].set_num; i > 1; i /= 2, cache_memory[level].index_bit++)
	{
		if (i % 2 == 1) cache_memory[level].set_num = 0;
	}

	cache_memory[level].tag_bit = 16 - (cache_memory[level].index_bit + cache_memory[level].offset_bit);

	// 어떤 입력이 틀렸는지 출력
	if (cache_memory[level].block_size <= 0 || cache_memory[level].block_num <= 0 || cache_memory[level].set_num <= 0)
	{
		printf("Wrong input: ");
		if (cache_memory[level].block_size <= 0) printf("block size");
		if (cache_memory[level].block_num <= 0)
		{
			if (cache_memory[level].block_size <= 0) printf(" / ");
			printf("block num");
		}
		if (cache_memory[level].set_num <= 0)
		{
			if (cache_memory[level].block_size <= 0 || cache_memory[level].block_num <= 0) printf(" / ");
			printf("set num");
		}
		printf("\n");

		return -1;
	}

	// cache size 확인
	cache_memory[level].cache_size = cache_memory[level].block_size * cache_memory[level].block_num * cache_memory[level].set_num;
	if (level == 1)
	{
		if (cache_memory[level].cache_size > 65536)
		{
			printf("Level 1 cache size is too big!\n");
			return -1;
		}
	}
	else
	{
		int wrong = 0;
		if (cache_memory[2].block_size < cache_memory[1].block_size)
		{
			printf("Level 2 cache block is smaller than level 1 cache!\n");
			wrong = -1;
		}
		if (cache_memory[2].cache_size > 65536)
		{
			printf("Level 2 cache size is too big!\n");
			wrong = -1;
		}
		if (wrong < 0) return -1;
	}

	cache_memory[level].set = (CacheSet*)calloc(cache_memory[level].set_num, sizeof(CacheSet));

	printf("%d %d %d\n", cache_memory[level].block_size, cache_memory[level].block_num, cache_memory[level].set_num);
	printf("%d %d %d\n", cache_memory[level].tag_bit, cache_memory[level].index_bit, cache_memory[level].offset_bit);

	return 0;
}

/*
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
*/

// cache 메모리를 해제할 함수
void FreeCache(int level)
{
	int set_num = cache_memory[level].set_num;
        for (int i = 0; i < set_num; i++)
        {
		CacheSet* now_set = &cache_memory[level].set[i];
                int num = now_set->num;
                for (char j = 0; j < num; j++)
                {
                        CacheBlock* now_block = now_set->first;
                        now_set->first = now_block->right;
                        free(now_block);
                }
        }

	free(cache_memory[level].set);
}

