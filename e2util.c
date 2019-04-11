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
	       "State: %s\n",
			sb->s_inodes_count, sb->s_blocks_count,
			sb->s_first_data_block, blocksize(sb),
			sb->s_blocks_per_group, sb->s_inodes_per_group,
			sb->s_state == 1 ? "Clean" : "Dirty");
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

	// Convert the superblock from little-endian format to whatever the
	// host system is.  Leave this at the end of get_superblock.
	byteswap_superblock(out);

	return 0;
}

// Fetch the data from the specified block into the provided buffer.
// Return 0 on success, 1 on error.
int get_block_data(struct superblock *sb, int blk, char *out)
{
	return 0;
}

// Write the data from the specified block to standard output.
// Return 0 on success, 1 on error.
int print_block_data(struct superblock *sb, int blk)
{
	return 0;
}

// Return the number of the block group that a certain block belongs to.
int bg_from_blk(struct superblock *sb, int blk)
{
	return 0;
}

// Return the index of a block within its block group.
int blk_within_bg(struct superblock *sb, int blk)
{
	return 0;
}

// Return the number of the block group that a certain inode belongs to.
int bg_from_ino(struct superblock *sb, int ino)
{
	return 0;
}

// Return the index of an inode within its block group
int ino_within_bg(struct superblock *sb, int ino)
{
	return 0;
}

// Retrieve information from the block group descriptor table.
// Return 0 on success, 1 on error.
int get_bgdesc(struct superblock *sb, int bg, struct bgdesc *out)
{
	// Code here...

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

	// Convert the inode from little-endian format to whatever the host
	// system is.  Leave this at the end of get_inode.
	byteswap_inode(out);

	return 0;
}

// Retrieves the data from the nth data block of a certain inode.
// Return 0 on success, 1 on error.
int get_inode_block(struct superblock *sb, struct inode *i, uint32_t n, char *out)
{
	return 0;
}

// Return 1 if a block is free, 0 if it is not, and -1 on error
int is_block_free(struct superblock *sb, int blk)
{
	return 0;
}

// Return 1 if a block appears to be an indirect block, 0 if it does not, and
// -1 on error.
int looks_indirect(struct superblock *sb, char *block)
{
	return 0;
}

// Return 1 if a block appears to be a doubly-indirect block, 0 if it does not,
// and -1 on error.
int looks_2indirect(struct superblock *sb, char *block)
{
	return 0;
}
