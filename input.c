#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	int level;
	int block_size, block_num, set_num;
	int size = 0, step = 0;
	int *block_size_arr, *cache_size;

	FILE* input = fopen("input.txt", "w");

	fprintf(input, "Y\n");

	printf("What is the max level of the cache memory?: "); 
	scanf("%d", &level);
	fprintf(input, "%d\n", level);
	block_size_arr = (int*)calloc(level, sizeof(int));
	cache_size = (int*)calloc(level, sizeof(int));

	for (int now_level = 1; now_level <= level; now_level++)
	{
		while (1)
		{
			printf("\n");
			printf("Let's make a level %d cache memory!\n", now_level);

                        // block size, block 개수, set 개수 입력
                        printf("Please input the size of cache block: ");
                        scanf("%d", &block_size);
                        printf("Please input the number of cache block in one set: ");
                        scanf("%d", &block_num);
                        printf("Please input the number of cache set: ");
                        scanf("%d", &set_num);

                        // 세 변수의 값 확인 
                        for (int i = block_size; i > 1; i /= 2)
                        {
                                if (i % 2) block_size = 0;
                        }
                        for (int i = block_num; i > 1; i /= 2)
                        {
                                if (i % 2) block_num = 0;
                        }
                        for (int i = set_num; i > 1; i /= 2)
                        {
                                if (i % 2) set_num = 0;
                        }

                        // 틀린 입력이 있다면 출력을 통해 명시한 후 재입력
                        if (block_size <= 0 || block_num <= 0 || set_num <= 0)
                        {
                                printf("Wrong input: ");
                                if (block_size <= 0) printf("block size");
                                if (block_num <= 0)
                                {
                                        if (block_size <= 0) printf(" / ");
                                        printf("block num");
                                }
                                if (set_num <= 0)
                                {
                                        if (block_size <= 0 || block_num <= 0) printf(" / ");
                                        printf("set num");
                                }
                                printf("\n");

                                continue;
                        }

                        // block size와 cache size 확인
                        // block size 또는 cache size가 상위 메모리보다 작거나, cache size가 메인 메모리보다 크다면 재입력
			block_size_arr[now_level] = block_size;
                        cache_size[now_level] = block_size * block_num * set_num;
                        int wrong = 0;
                        if (now_level > 1)
                        {
				if (block_size_arr[now_level] < block_size_arr[now_level - 1])
                                {
                                        printf("Level %d cache block is smaller than level %d cache block!\n", now_level, now_level - 1);
                                        wrong = -1;
                                }
                                if (cache_size[now_level] < cache_size[now_level - 1])
                                {
                                        printf("Level %d cache size is smaller than level %d cache size!\n", now_level, now_level - 1);
                                        wrong = -1;
                                }
                        }
                        if (cache_size[now_level] > 65536)
                        {
                                printf("Level %d cache size is bigger than the size of main memory!\n", now_level);
                                wrong = -1;
                        }
                        if (wrong == -1) continue;

			fprintf(input, "%d\n", block_size);
			fprintf(input, "%d\n", block_num);
			fprintf(input, "%d\n", set_num);
                        break;
		}
	}

	// size, step 입력
	// 틀린 값 입력 시 재입력
	while (!size)
	{
		printf("\n");
		printf("What is the number of data size?: ");
		scanf("%d", &size);
		if (!(size == 1 || size == 2 || size == 4 || size == 8))
		{
			printf("Wrong size!\n");
			size = 0;
		}
	}
	while (!step)
	{
		printf("\n");
		printf("What is the number of step?: ");
		scanf("%d", &step);
		if (step % size || step > 32768)
		{
			printf("Wrong number!\n");
			step = 0;
		}
	}

	for (int i = 0; i < step; i += size)
	{
		for (int j = i; j < 65536; j += step) fprintf(input, "R %x %d\n", j, size);
	}

	// 프로그램 종료	
	fprintf(input, "P\n");
	fprintf(input, "Q\n");
	fprintf(input, "N\n");

	// 파일을 닫은 후 종료
	fclose(input);
	free(block_size_arr);
        free(cache_size);
	return 0;
}
