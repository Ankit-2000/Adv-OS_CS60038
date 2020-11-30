// CS60038: Assignment 1 part(b(2)) : Implement a loadable kernel module that provides the functionality of a heap inside the kernel mode and supports various ioctl calls
// 17CS30009 : Ankit Bagde
// 17CS30002 : Adhikansh Singh
// kernel version - 5.2.6

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/current.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/slab.h>         


MODULE_LICENSE("GPL");

// ioctl with write parameter
#define PB2_SET_TYPE  _IOW(0x10,0x31,int32_t*)
#define PB2_INSERT    _IOW(0x10,0x32,int32_t*)

// ioctl with read parameters
#define PB2_GET_INFO  _IOR(0x10,0x33,int32_t*)
#define PB2_EXTRACT   _IOR(0x10,0x34,int32_t*)

struct pb2_set_type_arguments{
	int32_t heap_size;
	int32_t heap_type;
};

struct obj_info{
	int32_t heap_size;
	int32_t heap_type;
	int32_t root;
	int32_t last_inserted;
};

struct result{
	int32_t result;
	int32_t heap_size;
};

typedef struct pcb_{
    pid_t _pid;			// stores the processID of the owner process. 
    int32_t *buffer;	// buffer to store the heap
    struct pb2_set_type_arguments h_attr;	//  maximum size of the heap
    struct obj_info info;		// store heap info [current size of heap]
    struct pcb_* next;		// stores link to next Block in the List 
}pcb;


static struct file_operations file_ops;
static DEFINE_MUTEX(pcb_mutex);
static pcb* pcb_head = NULL;


/*---------------      PCB Functions     ----------------*/
static pcb* pcb_create_node(pid_t pid) {
	pcb* node = (pcb*)kmalloc(sizeof(pcb), GFP_KERNEL);
	node->_pid = pid;
	node->next = NULL;
	node->buffer = NULL;
	(node->h_attr).heap_size=0;
	(node->h_attr).heap_type=-1;  		// heap type = -1 denotes that it is not initialized yet
	(node->info).heap_size=0;
	(node->info).heap_type=-1;
	(node->info).last_inserted=NULL;
	(node->info).root=0;
	return node;
}

static pcb* pcb_insert_node(pid_t pid) {
	pcb* node = pcb_create_node(pid);
	node->next = pcb_head;
	pcb_head = node;
	return node;
}

static pcb* pcb_get_node(pid_t pid) {
	pcb* temp = pcb_head;
	while(temp){
		if (temp->_pid == pid) {
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

static int pcb_delete_node(pid_t pid) {
	pcb* prev = NULL;
	pcb* curr = NULL;

	if (pcb_head->_pid == pid) {
		curr = pcb_head;
		pcb_head = pcb_head->next;
		kfree(curr);
		return 0;
	}

	prev = pcb_head;
	curr = prev->next;
	while(curr) {
		if (curr->_pid == pid) {
		    prev->next = curr->next;
		    kfree(curr);
		    return 0;
		}
		prev = curr;
		curr = curr->next;
	}
	return -1;
}
/*--------------------------------------------------------------------------------------------------*/


/*---------------                        Required Heap Functions                    ----------------*/
static void heapify_insert(pcb* curr, int index){
	if (index!=0){ 
		// Find parent 
		int parent = (index - 1)/ 2;
		// max heap
		if ( (curr->h_attr).heap_type==1 && curr->buffer[index] > curr->buffer[parent]) { 
			swap(curr->buffer[index], curr->buffer[parent]); 
			heapify_insert(curr, parent); 
		} 
		if((curr->h_attr).heap_type==0 && curr->buffer[index] < curr->buffer[parent]){
			swap(curr->buffer[index], curr->buffer[parent]); 
			heapify_insert(curr, parent); 
		}
	}
}

static void heapify_delete(pcb* curr, int index){ 
	int temp = index; 
	int l = 2 * index + 1; 
	int r = 2 * index + 2;  
  
	// max_heap
	if((curr->h_attr).heap_type==1){
		if (l < (curr->info).heap_size && curr->buffer[l] > curr->buffer[temp]) 
			temp = l; 
		if (r < (curr->info).heap_size && curr->buffer[r] > curr->buffer[temp]) 
			temp = r; 
	}
	else{
		if (l < (curr->info).heap_size && curr->buffer[l] < curr->buffer[temp]) 
			temp = l; 
		if (r < (curr->info).heap_size && curr->buffer[r] < curr->buffer[temp]) 
			temp = r; 
	}  
	
	// If temp is not root 
	if (temp != index) { 
		swap(curr->buffer[index], curr->buffer[temp]); 
		heapify_delete(curr, temp); 
	}
}
/*--------------------------------------------------------------------------------------------------*/


/*---------------                        ioctl cmd functions                      ------------------*/

int set_type(pcb *node, unsigned long arg)
{
    	struct pb2_set_type_arguments *init_args = (struct pb2_set_type_arguments *)kmalloc(sizeof(struct pb2_set_type_arguments), GFP_KERNEL);
	if(copy_from_user(init_args, (struct pb2_set_type_arguments *)arg, sizeof(struct pb2_set_type_arguments)))
		return -ENOBUFS;

	// validate the type of heap
	if(!(init_args->heap_type==0 || init_args->heap_type==1)){
		printk(KERN_ERR "'Type of heap' for LKM left uninitialized for Process : %d\n", current->pid);
		return -EINVAL;
	}
	
	//validate the size of heap, check only if it is -ve 
	if(init_args->heap_size<0){
		printk(KERN_ERR "'Size of heap' cannot be negative for Process : %d\n", current->pid);
		return -EINVAL;
	}	
	
	(node->h_attr).heap_size = init_args->heap_size;
	node->buffer = (int *)kmalloc( init_args->heap_size*sizeof(int32_t),GFP_KERNEL);
	(node->h_attr).heap_type = init_args->heap_type;
	(node->info).heap_type = init_args->heap_type;
	
	kfree(init_args);
	printk(KERN_INFO "LKM heap initialized for Process : %d\n", current->pid);

    return 0;
}

int insert_in_heap(pcb *node,unsigned long arg)
{
	// check whether heap is initialized
	if( (node->h_attr).heap_type == -1){
		printk(KERN_ERR "The heap has not been initialized for the process: %d\n",node->_pid);
		return -EINVAL;
	}
	// check for elemtents overflow
	if((node->info).heap_size >= (node->h_attr).heap_size){
		printk(KERN_ERR "The heap has reached maximum capacity for the process : %d\n",node->_pid);
		return -EACCES;
	}

	int32_t *val = (int32_t *)kmalloc(sizeof(int32_t), GFP_KERNEL);

	if(copy_from_user(val,(int32_t *)arg,sizeof(int32_t)))
		return -ENOBUFS;

	node->buffer[(node->info).heap_size] = *val;
	(node->info).heap_size++;
	heapify_insert(node,(node->info).heap_size-1);

	// updating the required structures
	(node->info).root = node->buffer[0];
	(node->info).last_inserted = *val;
	
	kfree((void *)val);
	return 0;
}


int get_heap_info(pcb* node, unsigned long arg)
{
	// check whether heap is initialized
	if( (node->h_attr).heap_type == -1){
		printk(KERN_ERR "The heap has not been initialized for the process: %d\n",node->_pid);
		return -EINVAL;
	}
	
	struct obj_info *inf = (struct obj_info *)kmalloc(sizeof(struct obj_info), GFP_KERNEL);
	if(copy_from_user(inf, (struct obj_info *)arg, sizeof(struct obj_info)))
		return -ENOBUFS;
	
	inf->heap_size = (node->info).heap_size;
	inf->heap_type = (node->info).heap_type;
	inf->root = (node->info).root;
	inf->last_inserted = (node->info).last_inserted;

	if(copy_to_user((struct obj_info *)arg, inf, sizeof(struct obj_info)))
		return -ENOBUFS;

	kfree((void *)inf);
	return 0;
}

int extract_element(pcb* node,unsigned long arg)
{
	
	// check whether heap is initialized
	if( (node->h_attr).heap_type == -1){
		printk(KERN_ERR "The heap has not been initialized for the process: %d\n",node->_pid);
		return -EINVAL;
	}
	// check if heap size is already zero
	if((node->info).heap_size == 0){
		printk(KERN_ERR "The heap is empty for the process : %d\n",node->_pid);
		return -EACCES;
	}
	
	struct result *obj = (struct result *)kmalloc(sizeof(struct result), GFP_KERNEL);
	
	if(copy_from_user(obj, (struct result *)arg, sizeof(struct result)))
		return -ENOBUFS;
	
	obj->result = node->buffer[0];
	(node->info).heap_size--;
	node->buffer[0] = node->buffer[(node->info).heap_size];
	obj->heap_size = (node->info).heap_size;
	heapify_delete(node,0);

	if(copy_to_user((struct result *)arg, obj, sizeof(struct result)))
		return -ENOBUFS;
		
	printk(KERN_INFO "Extracted element %d from heap for process : %d", node->buffer[0], node->_pid);
	kfree((void *)obj);
	return 0;
}

// cmd is command type,
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	
	while (!mutex_trylock(&pcb_mutex));
	pcb* curr = pcb_get_node(current->pid);
	if (curr == NULL)
	{
		printk(KERN_INFO "could not find process with Pid: %d\n",current->pid);
		mutex_unlock(&pcb_mutex);
		return -EINVAL;
	}

	int ret;
	switch (cmd) {
	case PB2_SET_TYPE:
		ret = set_type(curr, arg);
		break;
	case PB2_INSERT:
		ret =insert_in_heap(curr, arg);
		break;
	case PB2_GET_INFO:
		ret = get_heap_info(curr, arg);
		break;
	// check return type here
	case PB2_EXTRACT:
		extract_element(curr, arg);
		ret = 0;
		break;
	default:
		printk(KERN_INFO "invalid objtype");
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&pcb_mutex);
	return ret;
}

static int open(struct inode *inodep, struct file *filep) {
	pid_t pid = current->pid;
	while(!mutex_trylock(&pcb_mutex));
	
	// check if file is already open
	// reset the LKM if it is reopened
	if (pcb_get_node(pid)) {
		pcb_delete_node(pid);
		pcb_insert_node(pid);
		mutex_unlock(&pcb_mutex);
		printk(KERN_NOTICE "LKM Reset as the file already Open in Process : %d\n", pid);
		return 0;
	}
	
	// insert into pcb list
	pcb_insert_node(pid);
	mutex_unlock(&pcb_mutex);

	printk(KERN_NOTICE "File Opened by Process : %d\n", pid);

	return 0;
}

static int release(struct inode *inodep, struct file *filep) {
	pid_t pid = current->pid;
	pcb* curr = NULL;

	while (!mutex_trylock(&pcb_mutex));
	curr = pcb_get_node(pid);
	mutex_unlock(&pcb_mutex);
	
	// handle corner case - if file not opened by process
	if (curr== NULL) {
		printk(KERN_ERR "Process : %d has not Opened File\n", pid);
		return -EACCES;
	}

	while (!mutex_trylock(&pcb_mutex));
	pcb_delete_node(pid);
	mutex_unlock(&pcb_mutex);

	printk(KERN_NOTICE "File Closed by Process : %d\n", pid);

	return 0;
}


// insert the module in the kernel
static int hello_init(void){
	// create a file in /proc during initialization
	struct proc_dir_entry *entry = proc_create("partb_2_17CS30009", 0777, NULL, &file_ops);
	if(!entry) return -ENOENT;
	
	// set ownership of file_ops and write the function pointer
	file_ops.owner = THIS_MODULE; 
	file_ops.open = open;
	file_ops.release = release;
	file_ops.unlocked_ioctl = ioctl;
	
	printk(KERN_ALERT "LKM Heap inserted : '/proc/partb_2_17CS30009'\n");
	mutex_init(&pcb_mutex);

	return 0;
}

// remove the module from the kernel
static void hello_exit(void){
	// remove the file from /proc during the exit
	remove_proc_entry("partb_2_17CS30009", NULL);
	printk(KERN_ALERT "ENDING CURRENT LKM\n");
}

// binds the insertion and removal functions
module_init(hello_init);
module_exit(hello_exit);



