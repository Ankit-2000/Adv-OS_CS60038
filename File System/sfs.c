/**********************************************
Adhikansh Singh 17CS30002
Ankit Bagde 17CS30009
**********************************************/

#include "disc.h"
#include "sfs.h"

#define SetBit(A,k)     ( A |= (1 << k) )
#define ClearBit(A,k)   ( A &= ~(1 << k) )
#define TestBit(A,k)    ( A & (1 << k) )
#define min(x,y) ((x<y)?x:y)
#define max(x,y) ((x>y)?x:y)



uint32_t ceil_(uint32_t a, uint32_t b) {
    if (a % b == 0) return a / b;
    return a / b + 1;
}

super_block super_block_attr_init(disk *diskptr)
{
	super_block super;
	super.magic_number = MAGIC;
	super.blocks = diskptr->blocks-1;

	uint32_t i,ib,r,dbb,db;
	i = 0.1*super.blocks;
	// check this
	ib = ceil_(i*128,8*4096);
	r = super.blocks-i-ib;
	dbb = ceil_(r,8*4096);
	db = r-dbb;

	
	super.inode_blocks = i;
	super.inodes = i*128;

	super.inode_bitmap_block_idx = 1;
	super.inode_block_idx = 1+ib+dbb;
	super.data_block_bitmap_idx  = 1+ib;
	super.data_block_idx = 1+ib+dbb+i;
	super.data_blocks =  db;
	return super; 

}


// debugging function, remove from the final
void printD_superblock(super_block *sb)
{
    printf("Contents of super_block\n");
    printf("Magic number = %u\n", sb->magic_number);
    printf("blocks = %u\n", sb->blocks);
    printf("inode_blocks = %u\n", sb->inode_blocks);
    printf("inodes = %u\n", sb->inodes);
    printf("inode_bitmap_block_idx = %u\n", sb->inode_bitmap_block_idx);
    printf("data_block_bitmap_idx = %u\n", sb->data_block_bitmap_idx);
    printf("data_block_idx = %u\n", sb->data_block_idx);
    printf("data_blocks = %u\n\n", sb->data_blocks);
}

int format(disk *diskptr)
{
	if(!diskptr)
	{
		printf("Disk object not defined!\n");
		return -1;
	}

	super_block sb = super_block_attr_init(diskptr);
	printD_superblock(&sb);


	// writing the super block to the disk
	void *buf = malloc(sizeof(char)*BLOCKSIZE);	
	memcpy(buf,&sb,sizeof(super_block));
	write_block(diskptr,0,buf);


	// initialize the bitmaps for inodes and datablocks respectively
	memset(buf,0,sizeof(char)*BLOCKSIZE);
	for(int i= sb.inode_bitmap_block_idx;i<sb.data_block_bitmap_idx;i++)
		write_block(diskptr,i,buf); 

	for(int i= sb.data_block_bitmap_idx;i<sb.inode_block_idx;i++)
		write_block(diskptr,i,buf);

	// initializing the inodes
	inode temp[128];
	for(int i=0;i<128;i++)
		temp[i].valid = 0;
	// printf("sizeof the inode initializing block is %ld\n",sizeof(temp));
	
	// since there are 128 inodes in each block
	for(int i=sb.inode_block_idx;i<sb.data_block_idx;i++)
		write_block(diskptr,i,(void *)temp);
	return 0;
}

disk *mounted_diskptr;
super_block *mounted_sb;

int mount(disk *diskptr)
{
	if(!diskptr){
		printf("Disk pointer is empty\n");
		return -1;
	}
/////////////////////////////////////////////////////////////////////////////////////////////why NULL (if not verified)
	mounted_diskptr = NULL;
	mounted_sb = NULL;
	
	// verify the superblock
	void *buff =  malloc(BLOCKSIZE*sizeof(char));
	read_block(diskptr,0,buff);
	super_block *sb = (super_block*)malloc(sizeof(super_block));
	memcpy(sb,buff,sizeof(super_block));

	if(sb->magic_number!= MAGIC)
	{
		printf("MAGIC number on disk does not match\n");
		return -1;
	}
	mounted_diskptr = diskptr;
	mounted_sb = sb;
	create_file(); 
	return 0;
}

// start index, the length and the max count of the bitmap is given
// also sets the free bitmap bit it finds
uint32_t get_free_bitmap_idx(disk *diskptr,uint32_t s,uint32_t len,uint32_t max_count)
{
	uint32_t cur_count=0;
	uint32_t *buf = (uint32_t *)malloc(sizeof(char)*BLOCKSIZE);
	int idx=-1;

	for(int i=s;i<s+len;i++)
	{
		read_block(diskptr,i,(void *)buf);
		// SHOULD BE 1024
		for(int j=0;j<1024;j++)
		{
			/////////////////////////////////////////////////////////////////////////////doubt
			for(int k=0;k<32 && cur_count<max_count;k++,cur_count++)
			{
				if(!TestBit(buf[j],k))
				{
					idx = cur_count;
					SetBit(buf[j],k);
					write_block(diskptr,i,(void *)buf);
					break;
				}
			}
			if(idx != -1 || cur_count >=max_count)
				break;
		}
		if(idx != -1 || cur_count >=max_count)
			break;
	}
	
	if(idx == -1)
	{
		printf("No free inodes left!!\n");
		return -1;
	}
	return idx;
}

int create_file()
{
	if(!mounted_diskptr)
	{
		printf("NO disk mounted! exitting\n");
		return -1;
	}

	disk *diskptr = mounted_diskptr;

	// getting the super_block
	super_block *sb = mounted_sb;

	void *buff = malloc(BLOCKSIZE*sizeof(char));
	
	uint32_t free_bitmap_idx = get_free_bitmap_idx(diskptr,sb->inode_bitmap_block_idx,sb->inode_blocks,sb->inodes);

///////////////////////////////////////////////////////////////////check free_bitmap_idx = -1

	// update the inode at the given index
	int block_offset = free_bitmap_idx/128;
	int block_index = free_bitmap_idx%128;

	if(read_block(diskptr,sb->inode_block_idx+block_offset,buff) == -1)
		return -1;

	inode temp; 
	temp.valid = 1;
	temp.size = 0;
	// temp.direct[5] = ??
	// temp.indirect = ?? 

	memcpy(buff+block_index*(sizeof(inode)),&temp,sizeof(inode));
	write_block(diskptr,sb->inode_block_idx+block_offset,buff);

	return free_bitmap_idx;
}

int isvalid_inode(int inumber)
{
	if(inumber < 0 || inumber >=mounted_sb->inodes)
	{	
		printf("Invalid inode\n");
		return 0;
	}
	return 1;
}

int load_inode(int inumber,inode *in)
{
	// getting the super_block
	super_block *sb = mounted_sb;
	void *buff = malloc(BLOCKSIZE*sizeof(char));

	int block_offset = inumber/128;
	int block_index = inumber%128;

	if(read_block(mounted_diskptr,sb->inode_block_idx+block_offset,buff) == -1)
		return -1;

	memcpy(in,buff+block_index*sizeof(inode),sizeof(inode));
	return 0;
}

int stat(int inumber)
{
	inode in;
	if(!mounted_diskptr || !isvalid_inode(inumber) || load_inode(inumber,&in) <0)
	{
		printf("\t\tDisk is not mounted!!\n");
		return -1;
	}
	printf("\t\tvalid %d\n",in.valid );
	printf("\t\tLogical size: %d\n", in.size);
	uint32_t num_blocks = ceil_(in.size,BLOCKSIZE);
	uint32_t ndir_ptr = (num_blocks>5)?5:num_blocks; 

	printf("\t\tNumber of data blocks in use: %d\n", num_blocks);
	printf("\t\tNumber of direct pointers in use: %d\n", ndir_ptr);
	printf("\t\tNumber of indirect pointers in use: %d\n", num_blocks-ndir_ptr);
	
	return 0; 	
}



// given inumber, read the data into data which is assumed to be of length
// length, offset is the offset from which the values should be started reading from
// the inode
int read_i(int inumber, char *data, int length, int offset)
{
	inode in;
	disk *diskptr = mounted_diskptr;

	// getting the super_block of the mounted disk
	super_block *sb = mounted_sb;

	// the number of indirect addresses which can be stored in a single block
	const int MAX_INDIRECT_COUNT = 1024;

	if (!isvalid_inode(inumber) || load_inode(inumber,&in)<0)
		return -1;
	if(offset < 0 || offset >= in.size)
		return -1;

	length = ( (length+offset ) < in.size )?length+offset:in.size;

	uint32_t reqd = ceil_(length, BLOCKSIZE);
	uint32_t cur = offset/BLOCKSIZE;


	int read_bytes = -1, flag = 0;

    uint32_t p_indirect[MAX_INDIRECT_COUNT];

    while (cur < reqd) {

    	// stores the index of the block which is to be read
        uint32_t block_offset;
        if (cur < 5) 
            block_offset = in.direct[cur];
        else if (cur < (5 + MAX_INDIRECT_COUNT)) {
            if (flag == 0) {
                flag  = 1;
                // printf("1 : %d\n",in.indirect );
                read_block(diskptr, in.indirect + sb->data_block_idx, (void *)p_indirect);
            }
            // p_indirect now stores all the indirect pointers in the block now
            block_offset = p_indirect[cur - 5];
        } 
        
		else {       
            printf("Block val out of range\n");
            return -1;
        }

        char buff[BLOCKSIZE];
        // printf("2 : %d\n",block_offset );
        read_block(diskptr, block_offset + sb->data_block_idx, buff);

        if (read_bytes == -1) {
           	read_bytes = ( ((cur + 1) * BLOCKSIZE) <length)?((cur + 1) * BLOCKSIZE):length - offset;
            strncpy(data, buff + (offset - cur * BLOCKSIZE), read_bytes);
        } 
        else {
            int temp = min(BLOCKSIZE, length - cur * BLOCKSIZE);
            strncpy(data + read_bytes, buff, temp);
            read_bytes += temp;
        }
        cur++;
    }
    return read_bytes;
}
/////////////////////////////////////////////////////////////////////If length is out of range, read only till the end of
// the file and return the length of the data read (in bytes)
int write_inode(int inumber, inode* in)
{
    char buff[BLOCKSIZE];
    if (read_block(mounted_diskptr, inumber / 128 + mounted_sb->inode_block_idx, buff) < 0)
        return -1;

    // check this offset carefully
	memcpy(buff+(inumber%128)*sizeof(inode),in,sizeof(inode));

    int ret = write_block(mounted_diskptr, inumber/128 + mounted_sb->inode_block_idx, buff);
    return ret;
}


int write_i(int inumber, char *data, int length, int offset)
{
	inode in;
	disk *diskptr = mounted_diskptr;

	// getting the super_block of the mounted disk
	super_block *sb = mounted_sb;

	// the number of indirect addresses which can be stored in a single block
	const int MAX_INDIRECT_COUNT = 1024;
	if(offset < 0)
		return -1;
	if (!isvalid_inode(inumber) || load_inode(inumber,&in) == -1)
		return -1;

	length +=offset;

	uint32_t reqd = ceil_(length,BLOCKSIZE);
	uint32_t cur = offset/BLOCKSIZE;

	// printf("reqd %d     cur %d\n",reqd,cur );
	
	// maintains the maximum no of blocks available now
	uint32_t max_cur_blocks = ceil_(in.size,BLOCKSIZE);

	// if offset > in.size allocate upto cur

	int bytes_written = -1;
    uint32_t p_indirect[MAX_INDIRECT_COUNT];  

    while(cur < reqd)
    {
    	uint32_t data_block_offset;
    	uint32_t ret = 1;
    	uint32_t new_entry;

    	if(cur >= max_cur_blocks)
    	{

			new_entry = get_free_bitmap_idx(diskptr,sb->data_block_bitmap_idx,sb->inode_block_idx-sb->data_block_bitmap_idx,sb->data_blocks);
			ret = (new_entry == -1);
			printf("Got new fd for data block :%d\n",new_entry);
			if(new_entry == -1)
				return -1;
    	}
    	// all the blocks come in the direct pointers of inode
    	if(cur < 5)
    	{
    		// printf("cur < 5\n");
    		if(!ret){
    			// printf("!ret of cur < 5\n");
    			max_cur_blocks++;
    			in.direct[cur] = new_entry;
    		}

    		data_block_offset = in.direct[cur];
    	}
    	// all the blocks come in the indirect pointers of the given block
    	else if(cur < (1029)){
    		// if exactly 5 blocks used indirect pointer should be alloted
    		if(cur == 5 && !ret)
    		{
    			// printf("cur == 5 and !ret\n");
    			in.indirect = new_entry;
    			new_entry = get_free_bitmap_idx(diskptr,sb->data_block_bitmap_idx,sb->inode_block_idx-sb->data_block_bitmap_idx,sb->data_blocks);
    			ret = (new_entry == -1);
    			if(ret < 0) 
    				return -1;
    		}

    		if(!ret)
    		{
    			// printf("case 3\n");
    			max_cur_blocks++;
                uint32_t ind_ptr_buff[1024];
                // printf("1\n");
                
                read_block(diskptr,in.indirect+sb->data_block_idx,(void *)ind_ptr_buff);
                ind_ptr_buff[cur-5] = new_entry;
                // printf("2\n");
                write_block(diskptr,in.indirect+sb->data_block_idx,(void *)ind_ptr_buff);
    		}

    		data_block_offset = new_entry;
    	}
    	else{
    		printf("File capacity is full, failed to write\n");
    		return -1;
    	}

    	char buff[BLOCKSIZE];
    	// temp here stores the index of the data block to be read


    	// printf("3: %d\n",data_block_offset);
    	
    	read_block(diskptr,data_block_offset+sb->data_block_idx,buff);

    	// bytes_written stores the chars written so far
    	// temp here stores characters written in this iteration if there was also
    	// a write in the previous iteration
    	if(bytes_written == -1)
    	{
    		// printf("inside: bytes_written== -1\n");
    		bytes_written = min( (cur+1)*BLOCKSIZE,length)-offset;
    		strncpy(buff+(offset-BLOCKSIZE*cur),data,bytes_written);
    		// printf("wrote %d bytes to the buffer\n",bytes_written);
    	}
    	else{

    		// printf("inside else of bytes_written== -1\n");
    		int temp = min(BLOCKSIZE,length - BLOCKSIZE*cur);
    		strncpy(buff,data+bytes_written,temp);
    		// printf("wrote %d bytes to the buffer\n",temp);
    		bytes_written+=temp;
    	}
    	// printf("4 : %d\n", data_block_offset);
    	write_block(diskptr,data_block_offset+sb->data_block_idx,buff);
    	cur++;
    }

    in.size = max(in.size,length);
    
    write_inode(inumber,&in);
    return bytes_written;
}

int bitmap_clear(disk *diskptr,uint32_t index, uint32_t len,  uint32_t block_offset)
{
    if (index < 0 || index >= len)
        return -1;

    const int int_per_block = 1024;
    uint32_t block_num = (index / 32) / int_per_block;
    uint32_t offset = (index / 32) % int_per_block;
    uint32_t bit_pos = index % 32;

    uint32_t buff[int_per_block]; // disk_bmap

    if (read_block(diskptr, block_num + block_offset, buff) < 0)
        return -1;

    // if(TestBit(buff[offset],bit_pos) == 0)
    // {
    // 	printf("Given bit is already unset");
    // 	return -1;
    // }

    ClearBit(buff[offset],bit_pos);
    int ret = write_block(diskptr, block_num + block_offset, buff);

    return ret;
}

// flag=0 => clear_data_block_id
// flag=1 => clear_inode_bitmap_id
int clear_helper(uint32_t id, int flag){
	if(flag)
   		return bitmap_clear(mounted_diskptr,id, mounted_sb->inodes, mounted_sb->inode_bitmap_block_idx);
   return bitmap_clear(mounted_diskptr, id,mounted_sb->data_blocks , mounted_sb->data_block_bitmap_idx);
}

int remove_file(int inumber)
{
	inode in;
	disk *diskptr = mounted_diskptr;

	// getting the super_block of the mounted disk
	super_block *sb = mounted_sb;

	if (!isvalid_inode(inumber) || load_inode(inumber,&in) == -1)
		return -1;

	uint32_t cur = 0;
	uint32_t reqd = ceil_(in.size,BLOCKSIZE),flag = 0;
	const int MAX_INDIRECT_COUNT = 1024;

	uint32_t p_indirect[MAX_INDIRECT_COUNT];
	int temp;

	for(;cur < reqd;cur++)
	{
		if(cur<5)
			temp = in.direct[cur];
		else if(cur < 1029)
		{
			if(!flag){
				read_block(diskptr,in.indirect+sb->data_block_idx,p_indirect );
				flag =1;
			}
			temp = p_indirect[cur-5];
			if(cur == (4+MAX_INDIRECT_COUNT)){
				if( clear_helper(in.indirect,0) < 0)
					return -1;
			}
		}
		else{
			printf("BLOCK VALUE IS OUT OF RANGE!!\n");
			return -1;
		}
		if(clear_helper(temp,0) < 0)
			return -1;
	}
	in.valid = 0;
	in.size = 0;

	if(write_inode(inumber,&in) < 0)
		return -1;
	if(clear_helper(inumber,1) < 0)
		return -1;
	return 0;
}


// parse the path and return the sub parts and number of sub parts
// checks if len of name is <= 249 and >0, sets n_parts=0 if not 
char** path_parser(char *dirpath, int *n_parts){

	// check path format
	int len = strlen(dirpath);

	if( len > 0 && dirpath[0]=='/')
		dirpath++,len--;

	if( len > 1 && dirpath[len-1] == '/'){
		dirpath[len-1] = '\0';
	}
	
	// calculate the number of parts in path
	*n_parts=1, len=strlen(dirpath);

	for (int i = 0; i < len; ++i)
		*n_parts += (dirpath[i]=='/') ? 1 : 0; 

	// store sub parts
	char** parts = (char **) malloc(sizeof(char *) * *n_parts);
	for (int i = 0; i < *n_parts; i++)
        parts[i] = (char *) malloc(sizeof(char) * MAX_LEN);

    if(len==0){
    	printf("length of dir name should be >0\n");
    	*n_parts=0;
    	return parts;
    }

    // printf("dir path is %s\n", dirpath);
    int j = 0;
    for (int i = 0; i < *n_parts; i++) {
        int k = 0;
        while (k<MAX_LEN && j < len && dirpath[j] != '/'){
            parts[i][k] = dirpath[j];
            k++,j++;
        }
        if(k>=MAX_LEN ){
        	printf("MAX length of dir name should be <= 245\n");
        	*n_parts=0;
        	return parts;
        }
        parts[i][k] = '\0';
        j++;
    }

    // printf("n_parts is %d\n", *n_parts);
    // for (int i = 0; i < *n_parts; ++i)
    // {
    // 	printf("%s\n", parts[i]);
    // }
    // printf("parsing done.....\n\n");

    return parts;
}

int createblock_dir(int inumber, int off, inode *in){

	// getting the mounted disk the super_block of the mounted disk
	disk *diskptr = mounted_diskptr;
	super_block *sb = mounted_sb;
	int num_ls = in->size / sizeof(dir_struct);
  
    if (off < 5 * DIRS_PER_BLOCK) {
       int blocknr = off / DIRS_PER_BLOCK;
       int offset = off % DIRS_PER_BLOCK; 
       int ret;

       if (offset == 0){
       		uint32_t free_idx =  get_free_bitmap_idx(diskptr,sb->data_block_bitmap_idx,sb->inode_block_idx-sb->data_block_bitmap_idx,sb->data_blocks);
            if (free_idx < 0)
                return -1;
            in->direct[blocknr] = free_idx;
            in->valid=1;
            write_inode(inumber, in);
       }

       return in->direct[blocknr];
    }
    return -1;
}

int getblock_dir(int inumber, int off, inode *in)
{
    int num_ls = in->size / sizeof(dir_struct);
    if (num_ls <= off)
        return -1;

    if (off < 5 * DIRS_PER_BLOCK) {
       int blocknr = off / DIRS_PER_BLOCK;
       return in->direct[blocknr];
    }
    return -1;
}


dir_struct* read_dir(int inumber, int off)
{
	inode in;

	// getting the mounted disk the super_block of the mounted disk
	disk *diskptr = mounted_diskptr;
	super_block *sb = mounted_sb;


	load_inode(inumber, &in);

	int blocknr = getblock_dir(inumber, off, &in);
	if (blocknr < 0)
		return NULL;

	dir_struct dir[DIRS_PER_BLOCK];

	if (read_block(diskptr, blocknr + sb->data_block_idx, dir) < 0)
	    return NULL;

	dir_struct* d = (dir_struct *) malloc(sizeof(dir_struct));
	*d = dir[off % DIRS_PER_BLOCK];

	return d;
}


// helper function - search for name in dir
// 		get_offset=true - returns the offset
// 		else returns the found structure
helper search_dir_get_off(int inumber, char* name, int get_offset){
    dir_struct *item;
    helper ret;
    int off = 0;

    while (1){
    	item = read_dir(inumber, off);
    	if(!item)	break;
    	// printf("]]] got inumber %d\n", item->inum);
    	// printf("]]] got for offset %d\n", off);
    	// printf("]]] got dir  %s\n", item->name);
    	// printf("]]] expected %s\n", name);
    	if(strcmp(name, item->name) == 0){
    		if(get_offset){
    			ret.off = off;
    			return ret;
    		}
    		else{
    			ret.dir = item;
    			return ret;
    		}
    	}
        free(item);
        off++;
    }

    if(get_offset)
    	ret.off = -1;
    else
    	ret.dir=item;
    return ret;
}

// check if all the parts exists in path
int validate_parts(char** parts, int n_parts, int* curr_dir_in){
	for (int i = 0; i < n_parts - 1; i++) {
		helper h = search_dir_get_off(*curr_dir_in, parts[i], 0);		
        if (!h.dir){
        	printf("Invalid path : %s\n", parts[i]);
        	return -1;
        }
        *curr_dir_in = h.dir->inum;
        free(h.dir);
    }
    return 0;
}

/*
steps :
check validity of all the paths (sub parts)
return inum of of parent dir */
int parent_inode(char** parts, int n_parts)
{    
	int curr_dir_in = root_inode;

	if(validate_parts(parts, n_parts, &curr_dir_in)<0)
		return -1;
	return curr_dir_in;
}

// steps :
	// validate the path [parse it and check parts validity]
	// returns the parent indoe
int validate_path_get_parent_inode(char* dirpath, char** basename){
	int n_parts;
	char** parts = path_parser(dirpath, &n_parts);

	if(n_parts==0){
		return -1;
	}

    int parent_in = parent_inode(parts, n_parts);

	if (parent_in < 0){
		return -1;
	}
	*basename = (char *)malloc(sizeof(char) * MAX_LEN);
	strcpy(*basename, parts[n_parts - 1]);

	for (int i = 0; i < n_parts; i++)
    	free(parts[i]);
    
    return parent_in;
}

int addentry_dir(int inumber, char *name, int type)
{
	// getting the mounted disk the super_block of the mounted disk
	// printf("\nentering add dir entry....\n");

	disk *diskptr = mounted_diskptr;
	super_block *sb = mounted_sb;

    inode in;
    load_inode(inumber, &in);
    // printf("stat for parent in after load_inode ------------------------\n");
    // stat(inumber);
    int num_ls = in.size / sizeof(dir_struct);
    
    int nbr = createblock_dir(inumber, num_ls, &in);

    if (nbr < 0) 
        return -1;
    
	dir_struct dir[DIRS_PER_BLOCK];
	if (read_block(diskptr, nbr + sb->data_block_idx, dir))
        return -1;

    dir_struct d;
    d.valid = 1;
  	d.type = type;
    d.len = strlen(name);
    d.inum = create_file();	
    strcpy(d.name, name);

    // printf("\ncreated dir details\n");
    // printf("valid %d\n", d.valid);
    // printf("name %s\n", d.name);
    // printf("type %d\n", d.type);
    // printf("len %d\n", d.len);
    // printf("inum %d\n", d.inum);
    // printf("----------\n");

    dir[num_ls % DIRS_PER_BLOCK] = d;
    printf("nbr for %s is %d\n",d.name, nbr);
    if (write_block(diskptr,sb->data_block_idx+nbr, dir) < 0) 
        return -1;

    in.size += sizeof(dir_struct);
    if(write_inode(inumber, &in)<0)
    	return -1;
    // stat(inumber);
    return d.inum;
}

//	steps :
//		Check validity of the dirpath
// 		Check if dir already exists
//  	add directory entry
int create_dir(char *dirpath){

	char* basename;
	int parent_in = validate_path_get_parent_inode(dirpath, &basename);
	// printf("parent_in is %d\n", parent_in);
	// printf("basename is %s\n", basename);

	if(parent_in<0){
		printf("Invalid path entry\n");
		return -1;
	}

	// check if dir already exists
	helper h = search_dir_get_off(parent_in, basename, 0);
	if(h.dir){
		printf("directory %s already exists\n", basename);
		return -1;
	}

	int ret = addentry_dir(parent_in, basename,1);
	// printf("addentry_dir returned %d\n", ret);
	return ret;
}


int read_file(char *path, char *data, int length, int offset){
    char* basename;	int inum;
    int parent_in = validate_path_get_parent_inode(path, &basename);
    
    if (parent_in < 0)
    	return -1;

	helper h = search_dir_get_off(parent_in, basename, 0);
    if (!h.dir) return -1;

    inum = h.dir->inum;
    free(h.dir);
    return read_i(inum, data, length, offset);
}

int write_file(char *path, char *data, int length, int offset){
    char* basename;	int inum,ret;
    int parent_in = validate_path_get_parent_inode(path, &basename);
    
    if (parent_in < 0)
    	return -1;

	helper h = search_dir_get_off(parent_in, basename, 0);
    if(h.dir){
    	// printf("found dir %s\n", h.dir->name);
    	inum = h.dir->inum, free(h.dir);
    }
    else{
    	// create file
    	char *bn;
	    parent_in = validate_path_get_parent_inode(path, &bn);
	    if (parent_in < 0)	return -1;
	    inum = addentry_dir(parent_in, bn, 0);
        if (inum < 0) 	return -1;
    }
    printf("BEFORE : stat for inum %d\n",inum);
   	stat(inum);

   	ret = write_i(inum, data, length, offset);
   	printf("AFTER : stat for inum %d\n",inum);
   	stat(inum);
    return ret;
}


int rementry_dir(int inumber, char *filename)
{
	// getting the mounted disk the super_block of the mounted disk
	disk *diskptr = mounted_diskptr;
	super_block *sb = mounted_sb;

	printf("BEFORE : stat for parent_in %d\n",inumber);
   	stat(inumber);

	helper item, h;
	item = search_dir_get_off(inumber, filename, 0);
    if (!item.dir){
    	printf("%s\n",filename);
    	return -1;
    }
    h = search_dir_get_off(inumber, filename,1);

    // printf("item and h received......\n");
    // printf("name of dir received %s\n",item.dir->name);

    // cehck if it is regular file or directory
    if (item.dir->type){
    	// directory - recursively delete
    	// recursively delete all
        
        int off = 0;
        dir_struct *d;
        int try_once=1;

        while (1) {
        	// printf("offset is -------   %d\n",off);
        	d = read_dir(item.dir->inum, off);
        	if(!d && !try_once){
        		// printf("breaking\n"); 
        		break;
        	}
        	if(!d && try_once){
        		try_once=0;
        		off=0;
        		continue;
        	}
        	printf("RECURSIVELY CALLING FOR %s with inum %d\n", d->name, d->inum);
            if (rementry_dir(item.dir->inum, d->name) < 0) {
                printf("FAILED removing %s\n", d->name);
                return -1;
            }
            free(d);
            off++;
        }

        inode in;
        load_inode(item.dir->inum, &in);
        // printf("stat for loaded inode for item.dir->inum  -> %d---------------\n", item.dir->inum);
        // stat(item.dir->inum);

        // clear entries
        for (int i = 0; i < 5; i++)		clear_helper(in.direct[i],0);
        
        clear_helper(item.dir->inum,1);

    }
    else{
    	// file
    	// printf("CALLING FOR FILE\n");
        remove_file(item.dir->inum);        
    }
        
    inode in;
    load_inode(inumber, &in);

    int num_ls = in.size / sizeof(dir_struct);

    dir_struct* lst = NULL;

    // other files are present
    if (num_ls > 1)
        lst = read_dir(inumber, num_ls - 1);

    in.size -= sizeof(dir_struct);
    if(in.size==0)
    	in.valid=0;
    write_inode(inumber, &in);

    if (!lst){
    	printf("EARLY AFTER : stat for parent_in %d\n",inumber);
   	 	stat(inumber);
        return 0;
    }
    
    int nbr = getblock_dir(inumber, h.off, &in);

    dir_struct dir[DIRS_PER_BLOCK];
	if (read_block(diskptr, nbr + sb->data_block_idx, dir)<0)
        return -1;
    
    dir[h.off % DIRS_PER_BLOCK] = *lst;

    if (write_block(diskptr, nbr + sb->data_block_idx, dir) < 0) 
        return -1;
    
    free(lst);

    printf("AFTER : stat for parent_in %d\n",inumber);
    stat(inumber);
    return 0;
}

//	steps :
//		Check validity of the dirpath
//  	remove directory entry
int remove_dir(char *dirpath)
{
    char *basename;
    int parent_in = validate_path_get_parent_inode(dirpath, &basename);
    if (parent_in < 0){
		printf("Invalid path entry\n");
    	return -1;
    }
    printf("passing parent_in %s\n",basename);
    if(rementry_dir(parent_in, basename)<0){
    	printf("NOT SUCCESSFUL REMOVE\n");
    	return -1;
    }
    return 0;    
}
