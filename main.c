#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// **************************************************************************
// 캐시 메모리 2개와 메인 메모리의 데이터 교환 구현

// 16-bit address를 이용함.
// big endian 방식을 이용함.

// cache memory의 size는 매 실행마다 입력

// evict를 위해 LRU 알고리즘을 이용함
// double linked list 이용 (LRU block이 가장 뒤에 연결)

// 메인 메모리의 크기는 64MB. (16-bit address -> (2^16)B = (2^6 * 2 ^10)B = 64MB = 65536B)
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
	unsigned char* data;

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
	int access_count, hit_count, miss_count;
	CacheSet* set;
} CacheMemory;

// 모든 cache memory의 정보를 저장할 배열과, cache memory의 총 레벨
CacheMemory* cache_memory;
int level;

// main memory
typedef unsigned short Address;
typedef unsigned char* MainMemory;
MainMemory main_memory;

void MakeCache();
void FreeCache();
void PrintCache();

unsigned long long Read(Address address, int size);
int Write(Address address, int size, unsigned long long data);
CacheBlock* CacheAccess(Address address, int now_level);
CacheBlock* MakeBlock(Address address, int now_level);
void FreeBlock(CacheBlock* upper_block, Address address, int now_level);
unsigned short GetIndex(Address address, int now_level);
unsigned short GetTag(Address address, int now_level);

int main(void)
{
	// main memory 할당 후 값 초기화
	main_memory = (unsigned char*)calloc(65536, sizeof(unsigned char));
	for (int i = 0; i <= 0xffff; i++) main_memory[i] = i % 0x100;

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

		if (isatty(0)) printf("What is the max level of cache memory?: ");
		scanf("%d", &level);
		if (isatty(0)) printf("\n");
		MakeCache();

		char type;
		int temp_address;
		Address address;
		int size;
		unsigned long long data;
	
		while (1)
		{
			if (isatty(0)) printf("Input the access type [R / W / Q]: ");
			scanf(" %c", &type);
			if (type == 'Q' || type == 'q') break;

			if (isatty(0)) printf("What is the address of data?: ");
			scanf("%x", &temp_address);
			address = temp_address & 0xffff;

			if (isatty(0)) printf("What is the size of data to access?: ");
			scanf("%d", &size);
			
			switch (type)
			{
				case 'R': case 'r':
					printf("data: %llx\n", Read(address, size));
					break;

				case 'W': case 'w':
					printf("What is the data to save?: ");
					scanf("%llx", &data);

					Write(address, size, data);
					break;

				default:
					printf("Wrong input!\n");	
			}

			printf("\n");
		}

		PrintCache();

		// 캐시 메모리와 해제
		FreeCache();
	}

	free(main_memory);

	return 0;
}

// 캐시 메모리를 생성할 함수
// 블럭의 사이즈, 한 셋의 블럭 수, 셋의 개수를 입력받은 후 유효한 입력이라면 메모리 할당
// 입력한 수가 2의 제곱수인지 확인
// 블럭 사이즈가 상위 메모리보다 큰지 확인
// 총 캐시 사이즈를 구했을 때 상위 메모리보다 크고, 메인메모리보다 작은지 확인
void MakeCache()
{
	// 모든 cache memory를 담을 배열 선언
	cache_memory = (CacheMemory*)calloc(level + 1, sizeof(CacheMemory));
	
	// 상위의 cache memory부터 차례로 선언
	// now_level: 현재 캐시메모리의 level
        for (int now_level = 1; now_level <= level; now_level++)
        {
		// now_level의 cache memory
		CacheMemory* now_cache = &cache_memory[now_level];

		// 입력이 잘못되었을 시 continue를 통해 다시 입력
                while (1)
		{
			// level에 따라 만들 캐시메모리 결정
			if (isatty(0)) printf("Let's make a level %d cache memory!\n", now_level);

			// block size 입력 후 확인 (offset bit의 수 구하기)
			if (isatty(0)) printf("Please input the size of cache block: ");
        		scanf("%d", &now_cache->block_size);
			now_cache->offset_bit = 0;
        		for (int i = now_cache->block_size; i > 1; i /= 2, now_cache->offset_bit++)
        		{
        			if (i % 2 == 1) now_cache->block_size = 0;
			}

			// block 개수 입력 후 확인
        		if (isatty(0)) printf("Please input the number of cache block in one set: ");
        		scanf("%d", &now_cache->block_num);
			for (int i = now_cache->block_num; i > 1; i /= 2)
			{
				if (i % 2 == 1) now_cache->block_num = 0;
			}

			// set 개수 입력 후 확인 (index bit의 수 구하기)
			if (isatty(0)) printf("Please input the number of cache set: ");
			scanf("%d", &now_cache->set_num);
			now_cache->index_bit = 0;
			for (int i = now_cache->set_num; i > 1; i /= 2, now_cache->index_bit++)
			{
				if (i % 2 == 1) now_cache->set_num = 0;
			}

			// 구해놓은 index bit, offset bit를 이용해 tag bit의 수 구하기
			now_cache->tag_bit = 16 - (now_cache->index_bit + now_cache->offset_bit);

			// 틀린 입력이 있다면 출력을 통해 명시한 후 재입력
			if (now_cache->block_size <= 0 || now_cache->block_num <= 0 || now_cache->set_num <= 0)
			{
				printf("Wrong input: ");
				if (now_cache->block_size <= 0) printf("block size");
				if (now_cache->block_num <= 0)
				{
					if (now_cache->block_size <= 0) printf(" / ");
					printf("block num");
				}
				if (now_cache->set_num <= 0)
				{
					if (now_cache->block_size <= 0 || now_cache->block_num <= 0) printf(" / ");
					printf("set num");
				}
				printf("\n");

				continue;
			}

			// block size와 cache size 확인
			// block size가 상위 메모리보다 작거나, cache size가 메인 메모리보다 크다면 재입력
			now_cache->cache_size = now_cache->block_size * now_cache->block_num * now_cache->set_num;
			int wrong = 0;
			if (now_level > 1)
        		{
                		if (now_cache->block_size < cache_memory[now_level - 1].block_size)
                		{
                        		printf("Level %d cache block is smaller than level %d cache!\n", now_level, now_level - 1);
                        		wrong = -1;
                		}
        		}
			if (now_cache->cache_size > 65536)
			{
				printf("Level %d cache size is bigger than the size of main memory!\n", now_level);
				wrong = -1;
			}
			if (wrong == -1) continue;

			break;
		}

		// 모두 가능한 입력임을 확인했으므로 현재 cache memory에 set의 메모리 선언
		now_cache->set = (CacheSet*)calloc(now_cache->set_num, sizeof(CacheSet));
		
		if (isatty(0)) printf("\n");
	}

	return;
}

// cache 메모리를 해제할 함수
// 현재 cache의 set 개수를 구한 후, 각 set의 모든 block의 메모리 해제 후 모든 set의 메모리 해제
// 모든 cache의 set 메모리 해제가 끝났을 때 cache 전체의 메모리 해제
void FreeCache()
{
        // now_level: 현재 cache memory의 level
        for (int now_level = 1; now_level <= level; now_level++)
        {
                // set 개수를 구한 후 set 0부터 차례로 block의 메모리 해제
                int set_num = cache_memory[now_level].set_num;
                for (int i = 0; i < set_num; i++)
                {
			// 맨 앞의 block부터 차례로 메모리 해제
                        CacheSet* now_set = &cache_memory[now_level].set[i];
                        int num = now_set->num;
                        for (char j = 0; j < num; j++)
                        {
                                CacheBlock* now_block = now_set->first;
                                now_set->first = now_block->right;
				free(now_block->data);
                                free(now_block);
                        }
                }

                // set의 메모리 해제
                free(cache_memory[now_level].set);
        }

        // 모든 cache의 메모리 해제
        free(cache_memory);
        return;
}

// 현재 캐시 메모리의 데이터를 프린트할 함수
void PrintCache()
{
        printf("\n");
        printf("********** Current Cache Status **********\n");
        printf("\n");

	// 상위 cache memory부터 차례로 출력
	// now_level: 현재 cache memory의 level
        for (int now_level = 1; now_level <= level; now_level++)
        {
		// 현재 level의 cache 주소를 변수에 담은 후, 메모리의 정보 출력
		CacheMemory* now_cache = &cache_memory[now_level];
		printf("        ***** Level %d Cache *****\n", now_level);
		printf("block size: %d / block num: %d / set num: %d\n", now_cache->block_size, now_cache->block_num, now_cache->set_num);
		printf("bit count [tag / index / offset]: %d / %d / %d\n", now_cache->tag_bit, now_cache->index_bit, now_cache->offset_bit);
		printf("\n");

                // set 0부터 차례로 block의 정보 출력
		int block_size = now_cache->block_size;
		int set_num = now_cache->set_num;
		// index: set index
                for (int index = 0; index < set_num; index++)
                {
			// 현재 set의 주소와 block의 개수를 찾은 후, block의 개수 출력
			CacheSet* now_set = &now_cache->set[index];
			int num = now_set->num;
			printf("Set %d has %d blocks.\n", index, num);

			// 가장 앞의 block부터 차례로 base address와 data 출력
                        CacheBlock* now_block = now_set->first;
                        for (char i = 0; i < num; i++)
                        {
				printf("block %d\n", i);
				printf("base address: %x / dirty: %x / data: ", ((now_block->tag << (now_cache->index_bit + now_cache->offset_bit)) + (index << (now_cache->offset_bit))), now_block->dirty);
				for (int j = 0; j < block_size; j++) printf("%3x ", now_block->data[j]);
				printf("\n");
                                now_block = now_block->right;
                        }

			printf("\n");
                }
		
		// 마지막으로 현재 level cache의 access 정보 출력
		printf("access count: %d / hit count: %d / miss count : %d\n", now_cache->access_count, now_cache->hit_count, now_cache->miss_count);
        	printf("\n");
        	printf("******************************************\n");
        	printf("\n");
        }
}

// 메모리를 읽고 리턴할 함수
// address: 접근할 데이터의 주소, size: 읽을 데이터의 크기
// level 1 cache memory에서 읽어오기. data의 size가 너무 크다면 여러번 접근해 읽어오기.
// 읽어온 데이터들을 1byte씩 왼쪽으로 밀면서 리턴값에 더하기.
unsigned long long Read(Address address, int size)
{
	// 유효한 주소로의 접근인지 확인
	// size가 1~8이 아니거나 2의 제곱수(1 포함)가 아니라면 잘못된 접근
	// address가 size의 배수가 아니거나, 해당 address에 접근 시 main memory를 벗어나면 잘못된 접근
	 if (size < 1 || size > 8)
        {
                printf("Wrong data size!\n");
                return 0;
        }
	for (int i = size; i > 1; i /= 2)
        {
		if (i % 2 == 1)
		{
			printf("Wrong data size!\n");
			return 0;
		}
	}
	if (address % size || (address > (0xffff - (size - 1))))
	{
		printf("Wrong address!\n");
		return 0;
	}

	// 리턴값: 0으로 초기화
	unsigned long long ret = 0;
       
	int block_size = cache_memory[1].block_size;  // level 1 cache block의 size
	int access_num = (size / block_size) + (size % block_size > 0);  // 캐시 메모리 접근 횟수 (block size의 배수일 때는 뒤의 항은 더해지지 않음)
	int base_offset = (address % block_size);  // block에서 data를 출력할 때 사용할 base offset (data size가 block size 이상일 때에는 0, 아닐 때에는 address의 뒤 몇자리.)

	// 접근할 data의 size가 block size보다 작다면, 한 번만 접근.
        // 접근할 data의 size가 block size보다 크다면, address에 block size를 계속 더해가며 cache에 접근.
	CacheBlock* block;  // cache에 접근해 가져온 block의 주소
	for (int i = 0; i < access_num; i++)
	{
		// size만큼 데이터 가져오기
		block = CacheAccess(address, 1);  // 데이터를 가져올 block
		for (int j = 0; j < size; j++)
		{
			// 데이터 복사
			// 블럭의 마지막 data에 도달한 경우
			//   - j를 0으로 초기화: 다음 블럭의 첫 data부터 복사하기 위함.
			//   - size에서 block size를 차감: 복사한 만큼 차감
			//   - 이 동작은 블럭을 2개 이상 가져와야 할 때 유효함 (블럭을 2개 이상 필요로 하는 경우, 처음부터 base address가 0이므로 초기화 필요 X)
			if (base_offset + j == block_size)
			{
				size -= block_size;
				break;  // 현재 block에서 마지막 data에 도달했을 때, 다음 block으로 이동하기 위해 break
			}
			ret += block->data[base_offset + j];
			if (j < size - 1) ret <<= 8;  // 가장 주소가 작은 곳의 data가 msb쪽에 위치함. (big endian).
		}
		address += block_size;
	}

	return ret;
}

int Write(Address address, int size, unsigned long long data)
{
	// 유효한 주소로의 접근인지 확인
        // size가 1~8이 아니거나 2의 제곱수(1 포함)가 아니라면 잘못된 접근
        // address가 size의 배수가 아니거나, 해당 address에 접근 시 main memory를 벗어나면 잘못된 접근
         if (size < 1 || size > 8)
        {
                printf("Wrong data size!\n");
                return -1;
        }
        for (int i = size; i > 1; i /= 2)
        {
                if (i % 2 == 1)
                {
                        printf("Wrong data size!\n");
                        return -1;
                }
        }
        if (address % size || (address > (0xffff - (size - 1))))
        {
                printf("Wrong address!\n");
                return -1;
        }

        int block_size = cache_memory[1].block_size;  // level 1 cache block의 size
        int access_num = (size / block_size) + (size % block_size > 0);  // 캐시 메모리 접근 횟수 (block size의 배수일 때는 뒤의 항은 더해지지 않음)
        int base_offset = (address % block_size);  // block에서 data를 출력할 때 사용할 base offset (data size가 block size 이상일 때에는 0, 아닐 때에는 address의 뒤 몇자리.)

        // 접근할 data의 size가 block size보다 작다면, 한 번만 접근.
        // 접근할 data의 size가 block size보다 크다면, address에 block size를 계속 더해가며 cache에 접근.
	// lsb부터 메모리에 넣어야 하기 때문에, 큰 주소부터 접근.
        CacheBlock* block;  // cache에 접근해 가져온 block의 주소
	address += block_size * (access_num - 1);  // 맨 뒤 block부터 가져오기
        for (int i = access_num - 1; i >= 0; i--)
        {
		// data를 입력할 block을 가져온 후 dirty bit에 1 대입
                block = CacheAccess(address, 1);
		block->dirty = 1;
                for (int j = size - 1; j >= 0; j--)
                {
			// j가 block size를 넘어갔을 때 블럭에 맞춰 넣기 (예: 전체 8칸 중에서 뒤 4칸만 접근하도록 조정)
			if (j >= block_size) j = block_size - 1;

                        // 데이터 복사
                        // lsb쪽의 data를 가장 큰 주소에 넣기 (bit endian)
                        block->data[base_offset + j] = data & 0xff;
                        data >>= 8;
                }

		// block에 data를 다 복사한 다음 이전 주소의 block을 가져오기 위해 address 조정
		// access_num이 1일 때는 효과 X.
		size -= block_size;
                address -= block_size;
        }
	
	return 0;
}

// 주어진 address를 이용해 cache 메모리에 접근한 후, cache block의 주소를 반환할 함수.
// address: 데이터의 주소, now_level: cache memory의 level
// tag와 index를 구한 후, set의 첫 block부터 tag를 비교하며 같은지 확인
// 같은 tag를 발견했다면 해당 block을 맨 앞으로 옮긴 후 리턴
// 같은 tag를 발견하지 못했다면 구하려는 주소의 data를 갖는 block을 만들어서 맨 앞에 연결 후 리턴
CacheBlock* CacheAccess(Address address, int now_level)
{
	// 현재 level에서의 cache memory 주소와, index, tag
	CacheMemory* now_cache = &cache_memory[now_level];
        Address index = GetIndex(address, now_level), tag = GetTag(address, now_level);
	now_cache->access_count++;

        // 해당하는 set과 block의 개수
	CacheSet* now_set = &now_cache->set[index];
        int num = now_set->num;

	CacheBlock* ret_block = now_set->first;  // 최종적으로 반환할 블럭
        // 확인 후 다음 블럭
        // 마지막 블럭까지 이동 (마지막 블럭의 확인은 X)
        for (int i = 0; i < num - 1; i++)
        {
                if (ret_block->tag == tag) break;
		ret_block = ret_block->right;
        }
	
        // 블럭이 하나도 없는 경우 -> 새로 만들어서 set에 연결하기
        if (!ret_block)
        {
		now_cache->miss_count++;

                ret_block = MakeBlock(address, now_level);
                now_set->first = ret_block;
		now_set->last = ret_block;
		now_set->num++;

                return ret_block;
        }
	
	// 블럭이 존재하는 경우

	// 태그가 일치하면 해당하는 블럭을 맨 앞으로 이동 후 리턴
	if (ret_block->tag == tag)
	{
		now_cache->hit_count++;

		// 이미 맨 앞에 있는 경우 바로 리턴
		if (!ret_block->left) return ret_block;

		// 앞 뒤 블럭을 연결
		ret_block->left->right = ret_block->right;
		if (ret_block->right) ret_block->right->left = ret_block->left;  // 뒤 블럭이 있는 경우에만
		
		// 맨 앞으로 이동
		if (!ret_block->right) now_set->last = ret_block->left;  // 태그가 일치하는 블럭이 맨 마지막 블럭이었을 경우, last를 앞의 블럭으로 갱신
		ret_block->left = NULL;
		ret_block->right = now_set->first;
		ret_block->right->left = ret_block;
		now_set->first = ret_block;

		return ret_block;
	}

	now_cache->miss_count++;
	
	// 태그가 일치하지 않으면 새로운 데이터 가져온 후 맨 앞에 연결
	// set이 꽉 차지 않았다면 num을 1 늘리고, 꽉 찼다면 맨 마지막 블럭 제거
	CacheBlock* new_block = MakeBlock(address, now_level);
	new_block->right = now_set->first;
	new_block->right->left = new_block;
	now_set->first = new_block;

	if (now_set->num == now_cache->block_num)
	{
		ret_block->left->right = NULL;
		now_set->last = ret_block->left;

		Address temp_address = (ret_block->tag << (16 - now_cache->tag_bit)) + (index << (now_cache->offset_bit));
		FreeBlock(ret_block, temp_address, now_level);
	}
	else now_set->num++;

	return new_block;
}

// 주어진 address를 이용해 새로운 cache block를 만들 함수
// address: 접근할 데이터의 주소, now_level: cache memory의 level
CacheBlock* MakeBlock(Address address, int now_level)
{
	// 현재 레벨의 cache memory와, block data에 관한 정보들
	CacheMemory* now_cache = &cache_memory[now_level];
	int block_size = now_cache->block_size;
	int offset_bit = now_cache->offset_bit;
	Address base_address = (address >> offset_bit) << offset_bit;

	// 새롭게 만들 block (tag 갱신, data를 담을 메모리 할당)
	CacheBlock* new_block = (CacheBlock*)calloc(1, sizeof(CacheBlock));
        new_block->tag = GetTag(address, now_level);
	new_block->data = (unsigned char*)calloc(block_size, sizeof(unsigned char));

	// 최하위 cache memory가 아닌 경우
	// 한 단계 아래의 cache에서 데이터를 찾은 후, data 복사
	if (now_level < level)
	{
		CacheBlock* lower_block = CacheAccess(address, now_level + 1);
		int base_offset = base_address % (now_cache + 1)->block_size;
		for (int i = 0; i < block_size; i++) new_block->data[i] = lower_block->data[base_offset + i];
	}

	// 최하위 cache memory인 경우
	// main memory에서 data 복사
	else
	{
		for (int i = 0; i < block_size; i++) new_block->data[i] = main_memory[base_address + i];
	}

        return new_block;
}

// 블럭의 메모리를 해제할 함수 (evict)
void FreeBlock(CacheBlock* upper_block, Address address, int now_level)
{
	// dirty bit가 1인 경우 데이터를 하위 메모리에 덮어쓰기
	// 같은 address의 block을 가진 하위 메모리 중 가장 상위 메모리에만 덮어쓰기
	// 캐시 메모리에 데이터가 없을 때에는 메인 메모리에 덮어쓰기
	if (upper_block->dirty)
	{
		// 하위 block에 데이터를 옮기기 위한 값 가져오기
		int offset_bit = cache_memory[now_level].offset_bit;
		int upper_size = cache_memory[now_level].block_size;
		Address base_address = (address >> offset_bit) << offset_bit;

		// 하위의 모든 cache memory 살펴보기
		for (int i = now_level + 1; i <= level; i++)
		{
			// 해당 하위 cache에서 block을 찾기 위한 정보
			Address tag = GetTag(address, i), index = GetIndex(address, i);
			CacheSet* now_set = &cache_memory[i].set[index];

			// 첫 번째 block부터 시작, 같은 tag의 block이 나올 때까지 이동
			CacheBlock* lower_block = now_set->first;
			int num = now_set->num;
			for (int j = 0; j < num - 1; j++)
			{
				if (lower_block->tag == tag) break;
				lower_block = lower_block->right;
			}

			// tag가 같은 block이 있다면 data 복사 후 메모리 해제
			if (lower_block && (lower_block->tag == tag))
			{
				lower_block->dirty = 1;  // dirty bit를 1로 갱신

				// base offset을 찾은 후, data를 갱신하기
				Address base_offset = base_address % cache_memory[i].block_size;
				for (int j = 0; j < upper_size; j++) lower_block->data[base_offset + j] = upper_block->data[j];
				
				free(upper_block->data);
				free(upper_block);
				return;
			}
		}

		// 하위 cache memory에서 같은 주소의 block을 발견하지 못했을 때는 main memory에 복사
		for (int i = 0; i < upper_size; i++) main_memory[upper_size + i] = upper_block->data[i];
	}

	// 메모리 해제 후 종료
	free(upper_block->data);
	free(upper_block);
	return;
}

// 주어진 address로 cache memory에서의 set index를 찾을 hash function.
unsigned short GetIndex(Address address, int now_level)
{
	int index_bit = cache_memory[now_level].index_bit;
	int offset_bit = cache_memory[now_level].offset_bit;

	Address filter = 0;
	for (int i = 0; i < index_bit; i++)
	{
		filter <<= 1;
		filter += 1;
	}

        return (address >> offset_bit) & filter;
}

// 주어진 address로 tag를 찾을 hash function.
unsigned short GetTag(Address address, int now_level)
{
	int index_bit = cache_memory[now_level].index_bit;
	int offset_bit = cache_memory[now_level].offset_bit;
	return address >> (index_bit + offset_bit);
}
