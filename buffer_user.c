#include <stdlib.h>
#include <stdio.h>
#include "buffer.h"

#include <string.h>
#include <pthread.h>

static ring_buffer_421_t buffer;
static sem_t mutex;
static sem_t fill_count;
static sem_t empty_count;
int curr_value = 0;

void semaphore_test()
{
    printf("-----Begin Test-----\n");
    print_semaphores(); //Test to check if they initialized properly (empty = 0, full = 19, semtex = 1)

    enqueue_buffer_421((char *) '0');
}

long init_buffer_421(void) {
    // Note: You will need to initialize semaphores in this function.
    // Ensure we're not initializing a buffer that already exists.
    if (buffer.read || buffer.write) {
        printf("init_buffer_421(): Buffer already exists. Aborting.\n");
        return -1;
    }

    // Create the root node.
    node_421_t *node;
    node = (node_421_t *) malloc(sizeof(node_421_t));
    // Create the rest of the nodes, linking them all together.
    node_421_t *current;
    int i;
    current = node;
    // Note that we've already created one node, so i = 1.
    for (i = 1; i < SIZE_OF_BUFFER; i++) {
        current->next = (node_421_t *) malloc(sizeof(node_421_t));
        current = current->next;
    }
    // Complete the chain.
    current->next = node;
    buffer.read = node;
    buffer.write = node;
    buffer.length = 0;

    // Initialize your semaphores here.
    // TODO
    sem_init(&mutex, 0, 1);
    sem_init(&fill_count, 0, -1);//we should not dequeue if there in nothing filled
    sem_init(&empty_count, 0, 20);//start at 19 to make sure we have a negative value after 20 consecutive insertions

    return 0;
}

long enqueue_buffer_421(char * data) //Writes data to the current array slot -- USED BY PRODUCER
{
    // NOTE: You have to modify this function to use semaphores.
    if (!buffer.write) {
        printf("write_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }

    //if(curr_value > 9) curr_value = 0; //Only insert ints 0-9

    //Item is already "produced" by loop, tell consumer to wait until producer is finished & decrement empty count
    sem_wait(&empty_count);
    sem_wait(&mutex);

    //Take produced item and put into array indices in node
    memcpy(buffer.write->data, data, DATA_LENGTH);
    buffer.write = buffer.write->next;

    //Signal consumer that producer is finished, increment fill count and buffer length
    sem_post(&mutex);
    sem_post(&fill_count);
    buffer.length++;

    return 0;
}

long dequeue_buffer_421(char * data) //Takes the data stored in an array slot and "removes" it (prints it)
{
    // NOTE: Implement this function.
    if (!buffer.write) {
        printf("write_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }
    //printf("inside the dequeue function\n");
    //Signal that mutex is locked, decrement fill count
    sem_wait(&fill_count);
    sem_wait(&mutex);

    //Remove item from buffer, store value in temp variable
    //printf("attempting to copy, value of read->data: %c\n", buffer.read->data);
    memcpy(data, buffer.read->data, DATA_LENGTH);
    //printf("data after copying read->data into it: %c\n", data);
    buffer.read = buffer.read->next;
    //char* temp = "";
    //*temp = *data;

    //Signal that consumer is finished,
    sem_post(&mutex);
    sem_post(&empty_count);
    
    //for(int i=0; i<DATA_LENGTH; i++)
    //printf("value of data at the end of dequeue function: %c\n", data[i]);

    sem_wait(&mutex);
    buffer.length--;
    sem_post(&mutex);
    return 0;
}

long delete_buffer_421(void) {
    // Tip: Don't call this while any process is waiting to enqueue or dequeue.
    if (!buffer.read) {
        printf("delete_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }
    // Get rid of all existing nodes.
    node_421_t *temp;
    node_421_t *current = buffer.read->next;
    while (current != buffer.read) {
        temp = current->next;
        free(current);
        current = temp;
    }
    // Free the final node.
    free(current);
    current = NULL;
    // Reset the buffer.
    buffer.read = NULL;
    buffer.write = NULL;
    buffer.length = 0;
    return 0;
}

void print_semaphores(void) {
    // You can call this method to check the status of the semaphores.
    // Don't forget to initialize them first!
    // YOU DO NOT NEED TO IMPLEMENT THIS FOR KERNEL SPACE.
    int value;
    sem_getvalue(&mutex, &value);
    printf("sem_t mutex = %d\n", value);
    sem_getvalue(&fill_count, &value);
    printf("sem_t fill_count = %d\n", value);
    sem_getvalue(&empty_count, &value);
    printf("sem_t empty_count = %d\n", value);
}
//producer thread fucntion
/*
void *producer(void *arg){
	char *data = (char*)malloc(sizeof(char)*1024);
	int p =0;
	for (int i=0; i<20; i++){
		*data = '0'+p;
		//printf("%c\n",*data);
		enqueue_buffer_421(data);
		printf("insertion number %d\n",i);
		print_semaphores();
		p++;
		if (p>9) p = 0;
	}
	return NULL;
}
//consumer thread function
void *consumer(void *arg){
	node_421_t *node = buffer.read;
	for (int i =0; i<20;i++){
		printf("[%d]->%s\n",i, node->data);
		node = node->next; 
	}
	return NULL;
}
*/
int main(){
	init_buffer_421();
	char *data = (char*)malloc(sizeof(char)*1024);
	int p =0;
	for (int i=0; i<20; i++){
		*data = '0'+p;
		//printf("%c\n",*data);
		enqueue_buffer_421(data);
		printf("insertion number %d\n",i);
		print_semaphores();
		p++;
		if (p>9) p = 0;
	}
	for (int i =0; i<20;i++){
		dequeue_buffer_421(data);
		printf("[%d]->%s\n",i, data);//i don't know why this is working
	}
	enqueue_buffer_421(data);	
}


