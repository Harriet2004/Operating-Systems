#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h> 
#include <errno.h>

#define BUFFER_SIZE 1024
#define LINE_BUFFER_SIZE 1024
#define QUEUE_SIZE 20

    int print_error(char *msg) {
        fprintf(stderr, "%s %s\n", msg, strerror(errno));
        exit(2);
    }

typedef struct
{
    char *lines[QUEUE_SIZE];  // Array to hold lines
    int head;                 // Index for the front of the queue
    int tail;                 // Index for the end of the queue
    int count;                // Number of items in the queue
    pthread_mutex_t mutex;    // Mutex to protect access to the queue
    pthread_cond_t not_empty; // Condition variable to signal that the queue is not empty
    pthread_cond_t not_full;  // Condition variable to signal that the queue is not full
} SharedQueue;

// Shared data structures
SharedQueue queue;

// Mutex and condition variables for end-of-file flag
pthread_mutex_t eof_mutex = PTHREAD_MUTEX_INITIALIZER;
int eof_status = 0;

void *reader_thread(void *arg)
{
    FILE *source_file = (FILE *)arg;
    char *line_pointer = NULL;
    size_t length = 0;
    bool loop_run = true;

    while (loop_run)
    {
        pthread_mutex_lock(&queue.mutex);

        // Wait until the queue is not full
        while (queue.count == QUEUE_SIZE)
        {
            pthread_cond_wait(&queue.not_full, &queue.mutex);
        }

        // Read a line from the source file
        ssize_t read_len = getline(&line_pointer, &length, source_file);
        if (read_len != -1)
        {
            // Add the line to the queue
            queue.lines[queue.tail] = line_pointer;  // Directly store the line in the queue
            queue.tail = (queue.tail + 1) % QUEUE_SIZE;
            queue.count++;
            line_pointer = NULL;  // Reset the line pointer for the next getline() call
            length = 0;      // Reset the buffer length
        }
        else
        {
            // Signal EOF and exit the loop
            pthread_mutex_lock(&eof_mutex);
            eof_status = 1;
            pthread_mutex_unlock(&eof_mutex);
            pthread_cond_broadcast(&queue.not_empty); // Notify all writers
            pthread_mutex_unlock(&queue.mutex);
            loop_run = false;
        }

        pthread_cond_signal(&queue.not_empty); // Signal that the queue is not empty
        pthread_mutex_unlock(&queue.mutex);
    }
    free(line_pointer);
    return NULL;
}

void *writer_thread(void *arg)
{
    FILE *destination_file = (FILE *)arg;
    bool loop_run = true; 

    while (loop_run)
    {
        pthread_mutex_lock(&queue.mutex);  

        while (queue.count == 0)
        {
            pthread_mutex_lock(&eof_mutex);  
            if (eof_status) 
            {
                pthread_mutex_unlock(&eof_mutex);  
                loop_run = false; 
                break;  
            }
            pthread_mutex_unlock(&eof_mutex); 
            pthread_cond_wait(&queue.not_empty, &queue.mutex); 
        }

        if (!loop_run) 
        {
            pthread_mutex_unlock(&queue.mutex); 
            break; 
        }

        char *line = queue.lines[queue.head];
        queue.head = (queue.head + 1) % QUEUE_SIZE;  
        queue.count--;  

        pthread_cond_signal(&queue.not_full);  
        pthread_mutex_unlock(&queue.mutex);  

        fprintf(destination_file, "%s", line);
        free(line);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int ret;
    
    if (argc != 4) {
        fprintf(stderr, "Error: Must provide only three arguments: %s <n> <source_file> <destination_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n < 2 || n > 10)
    {
        print_error("Error: <n> must be between 2 and 10\n");
    }

    // Open source and destination files
    FILE *source_file = fopen(argv[2], "r");
    if (source_file == NULL)
    {
        fprintf(stderr, "Error: Cannot open source file %s: %s\n", argv[2], strerror(errno));
        return EXIT_FAILURE;
    }

    FILE *destination_file = fopen(argv[3], "wb");
    if (destination_file == NULL)
    {   
        fprintf(stderr, "Error: Cannot open destination file %s: %s\n", argv[3], strerror(errno));
        fclose(source_file);
        return EXIT_FAILURE;
    }

    // Initialize the shared queue
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;
    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.not_empty, NULL);
    pthread_cond_init(&queue.not_full, NULL);

    pthread_t readers[n];
    pthread_t writers[n];

    // Create reader and writer threads
    for (int i = 0; i < n; ++i)
    {
        ret = pthread_create(&readers[i], NULL, reader_thread, source_file);
        if (ret) print_error("Error: Creation of thread error");
        ret = pthread_create(&writers[i], NULL, writer_thread, destination_file);
        if (ret) print_error("Error: Creation of thread error");
    }

    // Wait for all threads to finish
    for (int i = 0; i < n; ++i) {
        ret = pthread_join(readers[i], NULL);
        if (ret) print_error("Error: Creation of thread error");
        ret = pthread_join(writers[i], NULL);
        if (ret) print_error("Error: Creation of thread error");
    }

    // Close files
    fclose(source_file);
    fclose(destination_file);

    // Cleanup
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);

    return EXIT_SUCCESS;
}
