// CS60038: Assignment 1 part(b(1)) : Implement a loadable kernel module that provides the functionality of a heap inside the kernel mode.
// 17CS30009 : Ankit Bagde
// 17CS30002 : Adhikansh Singh
// kernel version - 5.2.6

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

typedef struct pcb_{
    pid_t _pid;			// stores the processID of the owner process. 
    char buff[2];		// stores the type and the maximum size of the heap [passed initially to initialize the LKM]
    int32_t buffer[100];	// buffer to store heap
    int buffer_len;		// store current heap size
    bool is_init;
    struct pcb_* next;		// stores link to next Block in the List 
}pcb;

// global file operations variable
static struct file_operations file_ops;
static DEFINE_MUTEX(pcb_mutex);
static pcb* pcb_head = NULL;


/*---------------                        PCB Functions                    ----------------*/
static pcb* pcb_create_node(pid_t pid) {
	pcb* node = (pcb*)kmalloc(sizeof(pcb), GFP_KERNEL);
	node->_pid = pid;
	node->buffer_len = 0;
	node->next = NULL;
	node->is_init = false;
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
		if (curr->buff[0]==(char)0xF0 && curr->buffer[index] > curr->buffer[parent]) { 
			swap(curr->buffer[index], curr->buffer[parent]); 
			heapify_insert(curr, parent); 
		} 
		if(curr->buff[0]==(char)0xFF && curr->buffer[index] < curr->buffer[parent]){
			swap(curr->buffer[index], curr->buffer[parent]); 
			heapify_insert(curr, parent); 
		}
	} 
}

static void heapify_delete(pcb* curr, int index){ 
	int temp = index; 
	int l = 2 * index + 1; 
	int r = 2 * index + 2;  
  

	if(curr->buff[0]==(char)0xF0){
		if (l < curr->buffer_len && curr->buffer[l] > curr->buffer[temp]) 
			temp = l; 
		if (r < curr->buffer_len && curr->buffer[r] > curr->buffer[temp]) 
			temp = r; 
	}
	else{
		if (l < curr->buffer_len && curr->buffer[l] < curr->buffer[temp]) 
			temp = l; 
		if (r < curr->buffer_len && curr->buffer[r] < curr->buffer[temp]) 
			temp = r; 
	}  
	
	// If temp is not root 
	if (temp != index) { 
		swap(curr->buffer[index], curr->buffer[temp]); 
		heapify_delete(curr, temp); 
	} 
}
/*------------------------------------------------------------------------------------------*/

/*---------------                        File Operations                    ----------------*/

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

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) { 
	pid_t pid = current->pid;
	pcb* curr = NULL;
	
	if(!buf || !count)
		return -EINVAL;
	
	//fetch the correct pcb node
	while (!mutex_trylock(&pcb_mutex));
	curr= pcb_get_node(pid);
	mutex_unlock(&pcb_mutex);
	
	// handle corner case - if file not opened by process
	if (curr == NULL) {
		printk(KERN_ERR "Process : %d has not Opened File\n", pid);
		return -EACCES;
	}
	
	if(!curr->is_init){
		// copies data from userspace buffer to kernal space buffer
		if(copy_from_user(curr->buff, buf, count))
			return -ENOBUFS;
			
		// validate the type of heap
		if(!(curr->buff[0]==(char)0xFF || curr->buff[0]==(char)0xF0)){
			printk(KERN_ERR "'Type of heap' for LKM left uninitialized for Process : %d\n", pid);
			return -EINVAL;
		}
		
		// validate the size of the heap
		if((int)curr->buff[1]<1 || (int)curr->buff[1]>100){
			printk(KERN_ERR "'Size' for LKM left uninitialized for Process : %d\n", pid);
			return -EINVAL; 
		}
		
		printk(KERN_INFO "LKM heap initialized for Process : %d\n", pid);
		curr->is_init = true;
	}
	else{
		// check if it is of required type [DOUBT]
		if(count != 4)
			return -EINVAL;
		
		// overflow check
		if(curr->buffer_len + 1 > (int)curr->buff[1]){
			printk(KERN_ERR "ELEMENTS OVERFLOW for Process : %d\n", pid);
			return -EACCES;		
		}
		
		if(copy_from_user((curr->buffer)+curr->buffer_len, buf, count))
			return -ENOBUFS;
		
		curr->buffer_len++;
		printk(KERN_INFO "For Process : %d, Inserted -- %d\n", pid, curr->buffer[(curr->buffer_len)-1]);
		printk(KERN_INFO "For Process : %d, Total elements in heap - %d\n", pid, curr->buffer_len);
		
		// heapify
		heapify_insert(curr, curr->buffer_len-1);
	}
	return count;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
	pid_t pid = current->pid;
	pcb* curr = NULL;
	
	//fetch the correct pcb node
	while (!mutex_trylock(&pcb_mutex));
	curr= pcb_get_node(pid);
	mutex_unlock(&pcb_mutex);
	
	// handle corner case - if file not opened by process
	if (curr == NULL) {
		printk(KERN_ERR "Process : %d has not Opened File\n", pid);
		return -EACCES;
	}
	
	int ret = count;
	if(!buf || !count)
		return -EINVAL;
	
	// handle corner case - excess read calls (when heap size is 0)
	if(!curr->buffer_len){
		printk(KERN_ERR "EXCESS READ CALL for Process : %d\n", pid);
		return -EACCES;	
	}
	
	if(copy_to_user(buf, curr->buffer, count))
		return -ENOBUFS;
		
	printk(KERN_INFO "For Process : %d, Extracted %d bytes\n", pid, count);
	
	curr->buffer[0] = curr->buffer[curr->buffer_len-1];
	curr->buffer_len--;
	heapify_delete(curr, 0);
	return ret;
}
/*----------------------------------------------------------------------------------------*/


// insert the module in the kernel
static int hello_init(void){
	
	// create a file in /proc during initialization
	struct proc_dir_entry *entry = proc_create("partb_1_17CS30009", 0777, NULL, &file_ops);
	if(!entry) return -ENOENT;
	
	// set ownership of file_ops and write the function pointer
	file_ops.owner = THIS_MODULE; 
	file_ops.write = write;
	file_ops.read = read;
	file_ops.open = open;
	file_ops.release = release;
	
	// if ioctl is to be used
	// file_ops.unlocked_ioct = ioctl;
	mutex_init(&pcb_mutex);

	printk(KERN_ALERT "LKM Heap inserted : '/proc/partb_1_17CS30009'\n");
	return 0;
}

// remove the module from the kernel
static void hello_exit(void){
	// remove the file from /proc during the exit
	remove_proc_entry("partb_1_17CS30009", NULL);
	printk(KERN_ALERT "LKM Heap removed : '/proc/partb_1_17CS30009'\n");
}

// binds the insertion and removal functions
module_init(hello_init);
module_exit(hello_exit);
