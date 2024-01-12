#include <stdio.h>

int main(void)
{
	FILE* input = fopen("input.txt", "w");
	for (int i = 0; i < 256; i++) fprintf(input, "R %x\n", i);
	fprintf(input, "Q");
	fclose(input);

	return 0;
}
