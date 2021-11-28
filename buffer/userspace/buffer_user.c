#include <stdio.h>
#include "buffer.h"
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>


static ring_buffer_421_t buffer;
static sem_t mutex;
static sem_t fill_count;
static sem_t empty_count;
int curr_value = 0;

long init_buffer_421(void) {
    // Note: You will need to initialize semaphores in this function.
    // Ensure we're not initializing a buffer that already exists.
    if (buffer.read || buffer.write) 
    {
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
    for (i = 1; i < SIZE_OF_BUFFER; i++) 
    {
        current->next = (node_421_t *) malloc(sizeof(node_421_t));
        current = current->next;
    }
    
    // Complete the chain.
    current->next = node;
    buffer.read = node;
    buffer.write = node;
    buffer.length = 0;

    // Initialize your semaphores here.
    sem_init(&mutex, 0, 1);
    sem_init(&fill_count, 0, 0);//we should not dequeue if there in nothing filled
    sem_init(&empty_count, 0, 20);//start at 19 to make sure we have a negative value after 20 consecutive insertions

    return 0;
}

long enqueue_buffer_421(char * data) //Writes data to the current array slot -- USED BY PRODUCER
{
    // NOTE: You have to modify this function to use semaphores.
    if (!buffer.write) 
    {
        printf("write_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }

    //Item is already "produced" by loop, tell consumer to wait until producer is finished & decrement empty count
    sem_wait(&empty_count);
    sem_wait(&mutex);

    //Take produced item and put into array indices in node
    printf("Enqueue:	%c\n", data[0]); 
    print_semaphores(); 
    printf("__________________________\n");
    memcpy(buffer.write->data, data, DATA_LENGTH);
    buffer.write = buffer.write->next;
    buffer.length++;
    
    //Signal consumer that producer is finished, increment fill count and buffer length
    sem_post(&mutex);
    sem_post(&fill_count);
 

    return 0;
}

long dequeue_buffer_421(char * data) //Takes the data stored in an array slot and "removes" it (prints it)
{
    // NOTE: Implement this function.
    if (!buffer.write) 
    {
        printf("write_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }
    
    //Signal that mutex is locked, decrement fill count
    sem_wait(&fill_count);
    sem_wait(&mutex);

    //Remove item from buffer, store value in temp variable
    printf("Dequeue:	%c\n", data[0]); 
    print_semaphores();
    printf("__________________________\n");
    
    memcpy(data, buffer.read->data, DATA_LENGTH);
    buffer.read = buffer.read->next;
	buffer.length--;
    
    //Signal that consumer is finished,
    sem_post(&mutex);
    sem_post(&empty_count);

    return 0;
}

long delete_buffer_421(void) {
    // Tip: Don't call this while any process is waiting to enqueue or dequeue.
    if (!buffer.read) 
    {
        printf("delete_buffer_421(): The buffer does not exist. Aborting.\n");
        return -1;
    }
    
    // Get rid of all existing nodes.
    node_421_t *temp;
    node_421_t *current;
    current = buffer.read->next;
    
    while (current != buffer.read)
    {
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
void *producer(void *arg)
{
	//init_buffer_421(); //Producer initalizes buffer (as per rubric)
	
	arg = arg;//so that we don;t get the warning that we never use them
	char *data;
	data = (char*)malloc(sizeof(char)*DATA_LENGTH);
	int p;
	p = 0;
	
	struct timeval t1;
	gettimeofday(&t1, NULL); //Get current time of day
	srand(t1.tv_usec * t1.tv_sec); //Multiply current microseconds by seconds, use this value to seed 
	
	suseconds_t sleepTime; //Will hold sleep time
	int base_time; 		  //Base time, number between 0 and 10
	
	for (int i = 0; i < 100000; i++)
	{
		int c;
		c = '0'+p;
		memset(data, c, DATA_LENGTH);
		data[1023] = '\0';//set the last index to null for printf later
		
		enqueue_buffer_421(data);
		
		printf("Produced:	%c\n", data[0]); 
		printf("__________________________\n"); 
		
		p++;
		if (p>9) p = 0;
		
		base_time = (rand() % 11); //Base time, number between 0 and 10
		sleepTime = (suseconds_t) base_time * 1000; 
		usleep(sleepTime);
	}
	
	free(data);
	return NULL;
}
//consumer thread function
void *consumer(void *arg)
{
	arg = arg;
	
	char *data;
	data = (char*)malloc(sizeof(char)*DATA_LENGTH);
	
	struct timeval t1;
	gettimeofday(&t1, NULL); //Get current time of day
	srand(t1.tv_usec * t1.tv_sec); //Multiply current microseconds by seconds, use this value to seed 
	
	suseconds_t sleepTime; //Will hold sleep time
	int base_time; 		  //Base time, number between 0 and 10
	
	for (int i = 0; i < 100000;i++)
	{
		dequeue_buffer_421(data);
		printf("Consumed:	%c\n", data[0]);
		printf("__________________________\n");
		
		base_time = (rand() % 11); //Base time, number between 0 and 10
		sleepTime = (suseconds_t) base_time * 1000; 
		usleep(sleepTime);
	}
	
	free(data);
	return NULL;
}

int main()
{

	pthread_t producerID, consumerID;
	void *thread_result;
	init_buffer_421();
	
	pthread_create(&producerID, NULL, producer, NULL);
	pthread_create(&consumerID, NULL, consumer, NULL);
	
	pthread_join(producerID, &thread_result);
	pthread_join(consumerID, &thread_result);
	
	delete_buffer_421();
	
	return 0;
}

