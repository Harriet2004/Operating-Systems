#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define QUEUE_SIZE 20
#define LINE_BUFFER_SIZE 1024

int print_error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(2);
}

// Structure to hold the shared queue
typedef struct
{
    char *lines[QUEUE_SIZE];  // Array to hold lines
    int head;                 // Index for the front of the queue
    int tail;                 // Index for the end of the queue
    int count;                // Number of items in the queue
    pthread_mutex_t mutex;    // Mutex to protect access to the queue
    pthread_cond_t not_empty; // Condition variable to signal that the queue is not empty
    pthread_cond_t not_full;  // Condition variable to signal that the queue is not full
    int done;                 // Flag to signal EOF
} SharedQueue;

// Producer function to read lines from the source file and add them to the queue
void *producer(void *arg)
{
    // Extract the source file name and shared queue from the arguments
    char **args = (char **)arg;
    char *source_file = args[0];
    SharedQueue *queue = (SharedQueue *)args[1];

    // Open the source file for reading
    FILE *source = fopen(source_file, "r");
    if (!source)
    {
        perror("Error opening source file");
        free(arg);
        pthread_exit(NULL);
    }

    char *line = NULL; // Pointer to hold each line read from the file
    size_t len = 0;    // Variable to hold the length of the line buffer
    ssize_t read;      // Variable to hold the number of characters read

    // Read lines from the source file and add them to the shared queue
    while ((read = getline(&line, &len, source)) != -1)
    {
        pthread_mutex_lock(&queue->mutex); // Lock the queue for thread-safe access

        // Wait if the queue is full
        while (queue->count == QUEUE_SIZE)
        {
            pthread_cond_wait(&queue->not_full, &queue->mutex);
        }

        // Add the line to the queue
        queue->lines[queue->tail] = strdup(line);
        queue->tail = (queue->tail + 1) % QUEUE_SIZE;
        queue->count++;

        pthread_cond_signal(&queue->not_empty); // Signal that the queue is not empty
        pthread_mutex_unlock(&queue->mutex);    // Unlock the queue
    }

    free(line);     // Free the memory allocated for the line buffer
    fclose(source); // Close the source file

    // Signal end of file to consumers
    pthread_mutex_lock(&queue->mutex);
    queue->done = 1;
    pthread_cond_broadcast(&queue->not_empty); // Signal all consumers that EOF is reached
    pthread_mutex_unlock(&queue->mutex);

    pthread_exit(NULL);
}

// Consumer function to retrieve lines from the queue and write them to the destination file
void *consumer(void *arg)
{
    // Extract the destination file name and shared queue from the arguments
    char **args = (char **)arg;
    char *destination_file = args[0];
    SharedQueue *queue = (SharedQueue *)args[1];

    // Open the destination file for writing
    FILE *dest = fopen(destination_file, "w");
    if (!dest)
    {
        perror("Error opening destination file");
        free(arg);
        pthread_exit(NULL);
    }

    char *line; // Pointer to hold each line retrieved from the queue

    // Continuously retrieve lines from the queue and write them to the destination file
    while (1)
    {
        pthread_mutex_lock(&queue->mutex); // Locks the queue for thread-safe access

        // Wait if the queue is empty and no EOF signal is set
        while (queue->count == 0 && !queue->done)
        {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }

        // Check if EOF is reached and the queue is empty
        if (queue->count == 0 && queue->done)
        {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }

        // Retrieve the line from the queue
        line = queue->lines[queue->head];
        queue->head = (queue->head + 1) % QUEUE_SIZE;
        queue->count--;

        pthread_cond_signal(&queue->not_full); // Signal that the queue is not full
        pthread_mutex_unlock(&queue->mutex);   // Unlocks the queue

        fputs(line, dest); // Writes the line to the destination file
        free(line);        // Free the memory allocated for the line
    }

    fclose(dest); // Closes the destination file
    free(arg);    // Free the dynamically allocated argument memory
    pthread_exit(NULL);
}

// Main function
int main(int argc, char *argv[])
{
    // Checks for correct number of command-line arguments
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <num_threads> <source_file> <destination_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);  // Number of producer and consumer threads
    char *source_file = argv[2];      // Source file name
    char *destination_file = argv[3]; // Destination file name

    pthread_t producers[num_threads]; // Array to hold producer thread IDs
    pthread_t consumers[num_threads]; // Array to hold consumer thread IDs

    // Initialize the shared queue
    SharedQueue queue = {
        .head = 0,
        .tail = 0,
        .count = 0,
        .done = 0};
    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.not_empty, NULL);
    pthread_cond_init(&queue.not_full, NULL);

    // Prepares arguments for the producer and consumer threads
    char *producer_args[] = {source_file, (char *)&queue};
    char *consumer_args[] = {destination_file, (char *)&queue};

    // Creates producer threads
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&producers[i], NULL, producer, (void *)&producer_args);
    }

    // Creates consumer threads
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&consumers[i], NULL, consumer, (void *)&consumer_args);
    }

    // Wait for all producer threads to finish
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(producers[i], NULL);
    }

    // Wait for all consumer threads to finish
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(consumers[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);

    return EXIT_SUCCESS;
}
