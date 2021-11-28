#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>      //Memory allocation in Kernel
//#include <linux/delay.h>   //Used for creating delays between producer/consumer
#include <asm/uaccess.h>     //copy_to_user and copy_from_user functions for safe kernel->user and user->kernel datastreams 
#include "buffer.h"          //Buffer definitions - includes linux/semaphore.h (used for mutual exclusion) already

static ring_buffer_421_t buffer;
static struct semaphore mutex;
static struct semaphore fill_count;
static struct semaphore empty_count;


SYSCALL_DEFINE0(init_buffer_421)
{
    // Note: You will need to initialize semaphores in this function.
    // Ensure we're not initializing a buffer that already exists.
    if (buffer.read || buffer.write) 
    {
        printk("init_buffer_421(): Buffer already exists. Aborting.\n");
        return -1;
    }

    // Create the root node.
    node_421_t *node;
    node = (node_421_t *) kmalloc(sizeof(node_421_t), GFP_KERNEL);
    // Create the rest of the nodes, linking them all together.
    node_421_t *curr;
    int i;
    curr = node;
    // Note that we've already created one node, so i = 1.
    for (i = 1; i < SIZE_OF_BUFFER; i++) 
    {
        curr->next = (node_421_t *) kmalloc(sizeof(node_421_t), GFP_KERNEL);
        curr = curr->next;
    }
    
    // Complete the chain.
    curr->next = node;
    buffer.read = node;
    buffer.write = node;
    buffer.length = 0;

    // Initialize your semaphores here.
    // TODO - Read API to convert to Kernel semaphore
    sema_init(&mutex, 1); 
    sema_init(&fill_count, 0);
    sema_init(&empty_count, 20); 

    return 0;
}

SYSCALL_DEFINE1(enqueue_buffer_421, char*, data){
    // NOTE: You have to modify this function to use semaphores.
    if (!buffer.write) 
    {
        printk("write_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }

    //Item is already "produced" by loop, tell consumer to wait until producer is finished & decrement empty count
    down(&empty_count);
    down(&mutex);

    //Take produced item and put into array indices in node

    if(copy_from_user(buffer.write->data, data, DATA_LENGTH) == 0) //Safely copy data from userspace to kernel space
    {
    	printk(":: Enqueueing element into buffer. ::\n"); 
    	printk("%c%c%c%c%c...", data[0], data[1], data[2], data[3], data[4]);
    	
    	buffer.write = buffer.write->next;
   	buffer.length++;
   	
   	printk("%d items in buffer.", buffer.length); 
    
    	//Signal consumer that producer is finished, increment fill count and buffer length
    	up(&mutex);
    	up(&fill_count);
 
    	return 0;
    } 
 
    //Re-increment mutex and fill_count since no operation was performed; return -1 for error
    up(&mutex);
    up(&fill_count);
    printk("Data could not be copied from userspace to kernel.\n"); 
    return -1;
}

SYSCALL_DEFINE1(dequeue_buffer_421, char*, data) 
{
    if (!buffer.write) 
    {
        printk("write_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }
    
    //Signal that mutex is locked, decrement fill count
    down(&fill_count);
    down(&mutex);

    //Remove item from buffer
    if(copy_to_user(data, buffer.read->data, DATA_LENGTH) == 0) //Kernel implementation to copy data from kernel to userspace safely
    {
    	printk("::  Dequeueing element into buffer. ::\n"); 
    	printk("%c%c%c%c%c...", data[0], data[1], data[2], data[3], data[4]);
    	
    	buffer.read = buffer.read->next;
    	buffer.length--;
    	
    	printk("%d items in buffer.", buffer.length); 
    
    	//Signal that consumer is finished
    	up(&mutex);
    	up(&empty_count);
    	
    	return 0;
    } 

    //Re-increment mutex and empty_count since no operation was performed; return -1 for error
    up(&mutex);	
    up(&empty_count);
    printk("Data could not be copied from kernel to userspace.\n"); 
    
    return -1; //Returns -1 if data could not be copied from kernel to userspace 
}

SYSCALL_DEFINE0(delete_buffer_421) {
	// Tip: Don't call this while any process is waiting to enqueue or dequeue.
	if (!buffer.read) 
	{
		printk("delete_buffer_421(): The buffer does not exist. Aborting.\n");
		return -1;
	}
	
	// Get rid of all existing nodes.
	node_421_t *temp;
	node_421_t *curr;
	curr = buffer.read->next;
	
	while (curr != buffer.read) 
	{
		temp = curr->next;
		kfree(curr);
		curr = temp;
	}
	
	// Free the final node.
	kfree(curr);
	curr = NULL;
	
	// Reset the buffer.
	buffer.read = NULL;
	buffer.write = NULL;
	buffer.length = 0;
	return 0;
}
