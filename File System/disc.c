#include "disc.h"
#include <sys/stat.h>
#include <fcntl.h>


disk *create_disk(int nbytes)
{
	disk *new_disk = (disk *)malloc(sizeof(disk));

	new_disk->size = nbytes;

	new_disk->blocks =  (nbytes%BLOCKSIZE == 0)?nbytes/BLOCKSIZE-1:nbytes/BLOCKSIZE;

	new_disk->reads = 0;
	new_disk->writes = 0;
	new_disk->block_arr = (char **)malloc(new_disk->blocks*sizeof(char *));

	for(int i=0;i<new_disk->blocks;i++)
	{
		new_disk->block_arr[i] = (char * )malloc(BLOCKSIZE*sizeof(char));
	}

	return new_disk;
}

int read_block( disk *diskptr,int blocknr,void *block_data)
{
	if(!diskptr)
	{
		printf("disk pointer is NULL\n");
		return -1;
	}
	else if(blocknr <0 || blocknr>= diskptr->blocks)
	{
		printf("READERR:blocknr %d out of bounds\n",blocknr);
		return -1;
	} 

	memcpy(block_data,diskptr->block_arr[blocknr],BLOCKSIZE*sizeof(char));
	diskptr->reads++;
	return 0;

}

int write_block( disk *diskptr,int blocknr,void *block_data)
{
	if(!diskptr)
	{
		printf("disk pointer is NULL\n");
		return -1;
	}
	else if(blocknr <0 || blocknr>= diskptr->blocks)
	{
		printf("WRITERR:blocknr %d out of bounds\n",blocknr);
		return -1;
	} 

	memcpy(diskptr->block_arr[blocknr],block_data,BLOCKSIZE*sizeof(char));
	diskptr->writes++;
	return 0;
}

int free_disk(disk *diskptr)
{
	if(!diskptr)
		return 0;
	for(int i=0;i<diskptr->blocks;i++)
	{
		free(diskptr->block_arr[i]);
	}
	free(diskptr->block_arr);
	free(diskptr);
	return 0;
}