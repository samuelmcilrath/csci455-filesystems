#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "e2util.h"


// Switch all of the values in the superblock structure from ext2 little-endian
// to the host's byte order.
void byteswap_superblock(struct superblock *sb)
{
	SWAP_LE(sb->s_inodes_count, 32);
	SWAP_LE(sb->s_blocks_count, 32);
	SWAP_LE(sb->s_first_data_block, 32);
	SWAP_LE(sb->s_log_block_size, 32);
	SWAP_LE(sb->s_blocks_per_group, 32);
	SWAP_LE(sb->s_inodes_per_group, 32);
	SWAP_LE(sb->s_state, 16);
}

// Display a formatted output of the superblock parameters.
void print_superblock(struct superblock *sb)
{
	printf("Inodes: %u\n"
	       "Blocks: %u\n"
	       "First data block: %u\n"
	       "Block size: %u\n"
	       "Blocks/group: %u\n"
	       "Inodes/group: %u\n"
	       "State: %s\n"
		   "Total Block Groups: %d\n",
			sb->s_inodes_count, sb->s_blocks_count,
			sb->s_first_data_block, blocksize(sb),
			sb->s_blocks_per_group, sb->s_inodes_per_group,
			sb->s_state == 1 ? "Clean" : "Dirty",
			(sb->s_blocks_count + sb->s_blocks_per_group - 1) / sb->s_blocks_per_group);
}

// Switch all of the values in the inode structure from ext2 little-endian to
// the host's byte order.
void byteswap_bgdesc(struct bgdesc *bg)
{
	SWAP_LE(bg->bg_block_bitmap, 32);
	SWAP_LE(bg->bg_inode_bitmap, 32);
	SWAP_LE(bg->bg_inode_table, 32);
}

// Switch all of the values in the inode structure from ext2 little-endian to
// the host's byte order.
void byteswap_inode(struct inode *i)
{
	int j;

	SWAP_LE(i->i_mode, 16);
	SWAP_LE(i->i_uid, 16);
	SWAP_LE(i->i_size, 32);
	SWAP_LE(i->i_atime, 32);
	SWAP_LE(i->i_ctime, 32);
	SWAP_LE(i->i_mtime, 32);
	SWAP_LE(i->i_dtime, 32);
	for (j = 0; j < 12; j++)
		SWAP_LE(i->i_block_d[j], 32);
	SWAP_LE(i->i_block_1i, 32);
	SWAP_LE(i->i_block_2i, 32);
	SWAP_LE(i->i_block_3i, 32);
}

// Display a formatted output of the inode parameters.
void print_inode(struct inode *i)
{
	time_t t;

	printf("Mode: %o\n"
	       "User ID: %u\n"
	       "Size: %u\n",
			i->i_mode, i->i_uid, i->i_size);
	t = i->i_atime;
	printf("Access time: %s", ctime(&t));
	t = i->i_ctime;
	printf("Change time: %s", ctime(&t));
	t = i->i_mtime;
	printf("Modification time: %s", ctime(&t));
	t = i->i_dtime;
	printf("Deletion time: %s", ctime(&t));
	printf("First direct block: %u\n", i->i_block_d[0]);
}

// Print out all the data in the file represented by a certain inode.
// Return 0 on success, 1 on error.
int print_inode_data(struct superblock *sb, struct inode *i)
{
	int fullblocks = i->i_size / blocksize(sb);
	int j;
	char *block;

	block = malloc(blocksize(sb));
	if (block == NULL)
		return 1;

	for (j = 0; j < fullblocks; j++) {
		if (get_inode_block(sb, i, j, block))
			return 1;
		if (fwrite(block, blocksize(sb), 1, stdout) != 1)
			return 1;
	}
	if (i->i_size % blocksize(sb)) {
		if (get_inode_block(sb, i, j, block))
			return 1;
		if (fwrite(block, i->i_size % blocksize(sb), 1, stdout) != 1)
			return 1;
	}

	free(block);
	return 0;
}

// Switch all of the values in an indirect block from ext2 little-endian to the
// host's byte order.
void byteswap_iblock(struct superblock *sb, char *block)
{
	int i;
	uint32_t *entry = (uint32_t *) block;
	for (i = 0; i < blocksize(sb) / 4; i++)
		SWAP_LE(entry[i], 32);
}

// Returns the block size of the filesystem
int blocksize(struct superblock *sb)
{
	return 1024 << sb->s_log_block_size;
}

// --- end provided code --- //


// Retrieve the interesting parts of the superblock and store it in the struct.
// Return 0 on success, 1 on error.
int get_superblock(FILE *f, struct superblock *out)
{
	// Save the file so other functions can use it
	out->file = f;

	// Code here...
	int err = fseek(f, 1024, SEEK_SET);
	if (err == -1) {
		perror("fseek");
		return 1;
	}

	//get the inodes count and block count
	if (fread(&out->s_inodes_count, sizeof(uint32_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}
	//get the inodes count and block count
	if (fread(&out->s_blocks_count, sizeof(uint32_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}

	//seek to first block field 
	if (fseek(f, 12, SEEK_CUR)){
		perror("fseek");
		return 1;
	}
	//get block that belongs to filesystem and log block size
	if (fread(&out->s_first_data_block, sizeof(uint32_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}
	if (fread(&out->s_log_block_size, sizeof(uint32_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}

	//seek to blocks per group
	if (fseek(f, 4, SEEK_CUR)){
		perror("fseek");
		return 1;
	}
	//set field
	if (fread(&out->s_blocks_per_group, sizeof(uint32_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}

	//seek and set to inodes per group
	if (fseek(f, 4, SEEK_CUR)){
		perror("fseek");
		return 1;
	}
	if (fread(&out->s_inodes_per_group, sizeof(uint32_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}
	//seek and set file sys state
	if (fseek(f, 14, SEEK_CUR)){
		perror("fseek");
		return 1;
	}
	if (fread(&out->s_state, sizeof(uint16_t), 1, f) != 1) {
		perror("fread");
		return 1;
	}
	// Convert the superblock from little-endian format to whatever the
	// host system is.  Leave this at the end of get_superblock.
	byteswap_superblock(out);

	return 0;
}

// Fetch the data from the specified block into the provided buffer.
// Return 0 on success, 1 on error.
	int get_block_data(struct superblock *sb, int blk, char *out)
	{
		int bs = blocksize(sb);//get block size

		//seek to block
		if(fseek(sb->file, bs*blk, SEEK_SET) == -1){
			perror("fseek");
			return 1;
		}

		//read to buffer
		if(fread(out, bs, 1, sb->file)  != 1){
			perror("fread");
			return 1;
		}
		return 0;
	}

// Write the data from the specified block to standard output.
// Return 0 on success, 1 on error.
int print_block_data(struct superblock *sb, int blk)
{
	char *out = malloc(blocksize(sb));

	//read data
	if(get_block_data(sb, blk, out)){
		perror("get_block_data");
		return 1;
	}

	//write to std out
	if(fwrite(out, blocksize(sb), 1, stdout) == 0){
		perror("fwrite");
		return 1;
	}
	free(out);
	return 0;
}

// Return the number of the block group that a certain block belongs to.
int bg_from_blk(struct superblock *sb, int blk)
{	
	return blk / sb->s_blocks_per_group;
}

// Return the index of a block within its block group.
int blk_within_bg(struct superblock *sb, int blk)
{
	return blk % sb->s_blocks_per_group;
}

// Return the number of the block group that a certain inode belongs to.
int bg_from_ino(struct superblock *sb, int ino)
{
	return (ino - 1) / sb->s_inodes_per_group;
}

// Return the index of an inode within its block group
int ino_within_bg(struct superblock *sb, int ino)
{
	return (ino - 1) % sb->s_inodes_per_group;
}

// Retrieve information from the block group descriptor table.
// Return 0 on success, 1 on error.
int get_bgdesc(struct superblock *sb, int bg, struct bgdesc *out)
{
	// Code here...
	//seek to second block where bgdt and the correct block group
	if(fseek(sb->file, blocksize(sb)*2 + 32*bg, SEEK_SET)){
		perror("fseek");
		return 1;
	}
	//read in all fields
	if(fread(out, sizeof(uint32_t), 3, sb->file) != 3){
		perror("fread");
		return 1;
	}

	// Convert the block info from little-endian format to whatever the
	// host system is.  Leave this at the end of get_bgdesc.
	byteswap_bgdesc(out);

	return 0;
}

// Retrieve information from an inode (file control block).
// Return 0 on success, 1 on error.
int get_inode(struct superblock *sb, int ino, struct inode *out)
{
	// Code here...

	//You will first need to determine which block 
	//group an inode belongs to, then find that block group’s inode table, then get the correct
	//inode. Your utility functions will help here.
	
	//get block group
	int bg = bg_from_ino(sb, ino);
	struct bgdesc *bg_out = malloc(sizeof(struct bgdesc));
	get_bgdesc(sb, bg, bg_out);
	//seek to inode table of bg
	if(fseek(sb->file, bg_out->bg_inode_table* (blocksize(sb)), SEEK_SET)){
		perror("fseek");
		return 1;
	}
	//additional seek to block within block group
	
	if(fseek(sb->file, ino_within_bg(sb, ino)*128, SEEK_CUR)){
		perror("fseek");
		return 1;
	}
	
	//read mode, uid, and size and times into out
	if(fread(out, 24, 1, sb->file) != 1){
		perror("fread");
		return 1;
	}

	//seek and read block pointers
	if(fseek(sb->file, 16, SEEK_CUR)){
		perror("fseek");
		return 1;
	}
	if(fread(out->i_block_d, sizeof(uint32_t), 15, sb->file))
	// Convert the inode from little-endian format to whatever the host
	// system is.  Leave this at the end of get_inode.
	byteswap_inode(out);

	return 0;
}

// Retrieves the data from the nth data block of a certain inode.
// Return 0 on success, 1 on error.
int get_inode_block(struct superblock *sb, struct inode *i, uint32_t n, char *out)
{
	//0-11 is direct
	int blockno;
	char indir_out[blocksize(sb)];
	if(n < 12){
		blockno = i->i_block_d[n];
	}
	else if(n < 268){
		//get singly indirect
		get_block_data(sb, i->i_block_1i, indir_out);
		byteswap_iblock(sb,indir_out);

		//get byte blockno
		blockno = ((uint32_t *) indir_out)[n - 12]; 
	}
	else if(n < 65804){
		//get doubly indirect first layer
		get_block_data(sb, i->i_block_2i, indir_out);
		byteswap_iblock(sb,indir_out);

		//get second layer
		n -= 268;
		char indir_out2[blocksize(sb)];

		get_block_data(sb, ((uint32_t *)indir_out)[n/256], indir_out2);
		byteswap_iblock(sb, indir_out2);

		blockno = ((uint32_t *)indir_out2)[n%256];
	}
	else{
		//get triple indirect first layer
		get_block_data(sb, i->i_block_3i, indir_out);
		byteswap_iblock(sb,indir_out);

		//get second layer
		n -= 65804; //NEED TO CHANGE TO NEXT RANGE
		char indir_out2[blocksize(sb)];

		get_block_data(sb, ((uint32_t *)indir_out)[n/(256*256)], indir_out2);
		byteswap_iblock(sb, indir_out2);

		//get third layer
		char indir_out3[blocksize(sb)];

		get_block_data(sb, ((uint32_t *) indir_out2)[(n/256) % 256], indir_out3);
		byteswap_iblock(sb, indir_out3);

		//get blockno
		blockno = ((uint32_t *) indir_out3)[n % 256];
	}

	//seek to blockno and read
	get_block_data(sb, blockno, out);
	
	return 0;
}

//bitmap header is 0x42 0x4D
// Return 1 if a block is free, 0 if it is not, and -1 on error
int is_block_free(struct superblock *sb, int blk)
{
	struct bgdesc *bg = malloc(sizeof(struct bgdesc));
	
	//get block number of bitmap
	if(get_bgdesc(sb, bg_from_blk(sb, blk), bg)){
		perror("get bgdesc");
		free(bg);
		return -1;
	}

	// //load block
	char *out = malloc(blocksize(sb));
	if(get_block_data(sb, bg->bg_block_bitmap, out)){
		perror("get block data");
		free(bg);
		free(out);
		return -1;
	}


	// shift to find block in bitmap
	int idx = blk_within_bg(sb, blk);
    int byte = idx / 8;
    int bit  = idx % 8;
    int allocated = (out[byte] >> bit) & 1; //1 - allocated; 0 - free

	free(bg);
	free(out);
	return !allocated;
	
}

// Return 1 if a block appears to be an indirect block, 0 if it does not, and
// -1 on error.
int looks_indirect(struct superblock *sb, char *block)
{
	//all of the entries need to be less than block count
	//should be 4 bytes to store each block pointer
	//uint32_t *block_ptr = (uint32_t*) block; 
	for(int i = 0; i < blocksize(sb); i+= sizeof(uint32_t)){
		if((uint32_t)block[i] >= sb->s_blocks_count)
			return 0;
	}

	return 1;
}

// Return 1 if a block appears to be a doubly-indirect block, 0 if it does not,
// and -1 on error.
int looks_2indirect(struct superblock *sb, char *block)
{
	//uint32_t *block_ptr = (uint32_t*) block; 
	for(int i = 0; i < blocksize(sb); i+= sizeof(uint32_t)){
		char blocksi[blocksize(sb)];
		
		//make sure valid block
		if((uint32_t)block[i] >= sb->s_blocks_count)
			return 0;
		
		//check buffer
		get_block_data(sb, block[i] ,blocksi);
		if(!looks_indirect(sb, blocksi))
			return 0;
	}
	return 1;
}
