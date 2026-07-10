#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int num, result;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		return 1;
	}

	num = atoi(argv[1]);
	result = num * num;

	printf("%d\n", result);
	return 0;
}
