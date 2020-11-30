/**********************************************
Adhikansh Singh 17CS30002
Ankit Bagde 17CS30009
**********************************************/


#include "disc.h"
#include "sfs.h"
int main()
{	
	// create disk of nbytes = BLOCKSIZE*30
	printf("------------------PART A: Testing-------------------\n\n\n");
	int n_blocks = 50;
	disk *d = create_disk(BLOCKSIZE*n_blocks+24);
	int block_no = 3;
	if(!d)
		printf("DISK creation failed\n");
	else
		printf("DISK created\n");

	char *temp = (char *)malloc(BLOCKSIZE*sizeof(char));
	for(int i=0;i<BLOCKSIZE;i++)
	{
		temp[i] = 'a'+i%26;
	}
	
	write_block(d,block_no,(void*)temp);
	char *buf = (char *)malloc(BLOCKSIZE*sizeof(char));

	read_block(d,block_no,(void *)buf);
	read_block(d,11,(void *)buf);

	// printf("AFTER --------\n");
	// printf("READS : %d\n",d->reads);
	// printf("WRITES : %d\n",d->writes);
	// printf("BLOCKS : %d\n",d->blocks);

	printf("\n--------------TESTING PART B:--------------------\n\n");

	// formatting and mounting the disk
	format(d);
	mount(d);


	printf("\nCreating file\n");
	int fd = create_file();
	printf("File created\n");


	int len = 25000;
	char ch[len] ;//= "hello world";
	memset(ch,'a',len);
	printf("written %d char in file %d\n" , write_i(fd,ch,len,0),fd ); 



	char read_buf[len];	
	printf("read %d char in file %d\n" , read_i(fd,read_buf,len,0),fd ); 

	// checking whether written value is equal to read value

	for (int i = 0; i < len; ++i)
	{
		if(read_buf[i] != ch[i])
		{
			printf("different characters found (index,expected,found) %d %c %c\n",i,ch[i] ,read_buf[i] );
			break;
		}
	}

	printf("Calling the stat function on current file\n");
	stat(fd);

	printf("Removing current the file\n");
	remove_file(fd);
	stat(fd);

	printf("\n-----------------------------TESTING THE FILE SYSTEM------------------------	\n\n");

	printf("--------------------------------------TEST CREATE DIR --------------------------\n");
	// create a dir
	char path1[] = "/dir1/";
	if(create_dir(path1)<0)
		printf("NOT CREATED %s\n",path1);
	else
		printf("SUCC %s \n", path1);
	printf("======================================================================\n");
	
	// no path
	char path3[] = "";
	if(create_dir(path3)<0)
		printf("NOT CREATED %s\n",path3);
	else
		printf("SUCC %s \n", path3);
	printf("======================================================================\n");

	// invalid dir -  dir2 doesnt exist
	char path2[] = "/dir2/dir1";
	if(create_dir(path2)<0)
		printf("NOT CREATED %s\n",path2);
	else
		printf("SUCC %s \n", path2);
	printf("======================================================================\n");

	// nested path
	char path4[] = "dir1/dir1";
	if(create_dir(path4)<0)
		printf("NOT CREATED %s\n",path4);
	else
		printf("SUCC %s \n", path4);
	printf("======================================================================\n");

	// nested path
	char path7[] = "dir2";
	if(create_dir(path7)<0)
		printf("NOT CREATED %s\n",path7);
	else
		printf("SUCC %s \n", path7);
	printf("======================================================================\n");

	// nested path - already exists
	char path5[] = "/dir1/dir60/";
	if(create_dir(path5)<0)
		printf("NOT CREATED %s\n",path5);
	else
		printf("SUCC %s \n", path5);
	printf("======================================================================\n");

	// >MAX_LEN name
	char path6[250];
	for(int i=0; i<249; i++)	path6[i]='a';
	path6[249]='\0';
	if(create_dir(path6)<0)
		printf("NOT CREATED %s\n",path6);
	else
		printf("SUCC %s \n", path6);
	printf("======================================================================\n");

	int length,ret;
	printf("--------------------------------------TEST WRITE FILE --------------------------\n");

	// write 4500 bytes to dir2
	char data[4500];
	for (int i = 0; i < 4500; ++i)
		data[i]='a';
	data[4549]='\0';

	length = strlen(data);
	char filepath[] = "dir2/new_file";
	ret = write_file(filepath, data, length, 0);
	if(ret<0)
		printf("WRITE failed\n");
	else
		printf("SUCCESSFUL write %d \n",ret);
	
	printf("======================================================================\n");

	// write 4500 bytes to new file
	char data2[4500];
	for (int i = 0; i < 4500; ++i)
	{
		data2[i]='a';
	}
	data2[4549]='\0';

	length = strlen(data2);
	char filepath2[] = "dir1/new_file";
	ret = write_file(filepath2, data2, length, 0);
	if(ret<0)
		printf("WRITE failed\n");
	else
		printf("SUCCESSFUL write %d \n",ret);

	printf("--------------------------------------TEST READ FILE --------------------------\n");

	// write 4500 bytes to dir2
	length = 8;
	char data3[10];
	
	char filepath3[] = "dir2/new_file";
	ret = read_file(filepath3, data3, length, 0);
	if(ret<0)
		printf("READ failed\n");
	else{
		printf("SUCCESSFUL READ %d \n",ret);
		printf("%s\n", data3);
	}

	printf("======================================================================\n");





	printf("--------------------------------------TEST REMOVE DIR --------------------------\n");
	// removing empty
	if(remove_dir(path3)<0)
		printf("NOT REMOVED %s\n", path3);
	else
		printf("REMOVED %s\n", path3);
	printf("======================================================================\n");

	if(remove_dir(path7)<0)
		printf("NOT REMOVED %s\n", path7);
	else
		printf("REMOVED %s\n", path7);

	printf("======================================================================\n");

	// check recursive
	if(remove_dir(path1)<0)
		printf("NOT REMOVED %s\n", path1);
	else
		printf("REMOVED %s\n", path1);
	printf("======================================================================\n");

	// invalid path
	if(remove_dir(path4)<0)
		printf("NOT REMOVED %s\n", path4);
	else
		printf("REMOVED %s\n", path4);
	printf("======================================================================\n");

}