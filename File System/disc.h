#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const static int BLOCKSIZE = 4*1024;

typedef struct disk{
	uint32_t size;			// size of the disk
	uint32_t blocks;		// number of usable blocks (except stat block)
	uint32_t reads;			// number of block reads performed
	uint32_t writes;		// number of block writes performed
	char ** block_arr;      // array of pointers to the 4KB blocks. size is blocks

}disk;

disk * create_disk(int nbytes);

int read_block( disk *diskptr,int blocknr,void *block_data);

int write_block( disk *diskptr,int blocknr,void *block_data);

int free_disk(disk *diskptr);








