#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "e2util.h"

int main(int argc, char *argv[])
{
	FILE *f;
	struct superblock sb;

	// Fill in the number of required arguments and usage here
	if (argc <= 100) {
		printf("Usage: %s <image file> ARGUMENTS\n", argv[0]);
		return 1;
	}

	f = fopen(argv[1], "r");
	if (f == NULL) {
		perror("fopen");
		return 1;
	}

	if (get_superblock(f, &sb))
		return 1;

	// Code here...

	fclose(f);
	return 0;
}
