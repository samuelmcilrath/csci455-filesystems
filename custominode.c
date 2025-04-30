#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "e2util.h"

int main(int argc, char *argv[])
{
	FILE *f;
	struct superblock sb;

	// Fill in the number of required arguments and usage here
	if (argc < 4) {
		printf("Usage: %s <image file> <starting direct block> <dbl indr block>\n", argv[0]);
		return 1;
	}

	f = fopen(argv[1], "r");
	if (f == NULL) {
		perror("fopen");
		return 1;
	}

	if (get_superblock(f, &sb))
		return 1;

	//need to fill in the inode
	long int dir_block = strtol(argv[2], NULL, 0);
	struct inode *pwd_node = malloc(sizeof(struct inode));
	memset(pwd_node, 0, sizeof(struct inode));  // Initialize to zero

	pwd_node->i_size = 0x001978DA; //these are the bytes following the magic #


	//we need the direct blocks
	for(int i = 0; i < 12; i++){
		pwd_node->i_block_d[i] = dir_block++;
	}

	//individual block 
	pwd_node->i_block_1i = 6043;

	//double block
	pwd_node->i_block_2i = strtol(argv[3], NULL, 0);
	
	print_inode_data(&sb, pwd_node);
	free(pwd_node);
	fclose(f);
	return 0;
}
