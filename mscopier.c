#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

// Shared data structures
SharedQueue queue;

// Mutex and condition variables for end-of-file flag
pthread_mutex_t eof_mutex = PTHREAD_MUTEX_INITIALIZER;
int eof_reached = 0;

void *reader_thread(void *arg)
{
    FILE *source_file = (FILE *)arg;
    char line[LINE_BUFFER_SIZE];

    while (1)
    {
        pthread_mutex_lock(&queue.mutex);

        // Wait until the queue is not full
        while (queue.count == QUEUE_SIZE)
        {
            pthread_cond_wait(&queue.not_full, &queue.mutex);
        }

        // Read a line from the source file
        if (fgets(line, LINE_BUFFER_SIZE, source_file) != NULL)
        {
            // Add the line to the queue
            queue.lines[queue.tail] = strdup(line);
            queue.tail = (queue.tail + 1) % QUEUE_SIZE;
            queue.count++;
        }
        else
        {
            // Signal EOF and exit the loop
            pthread_mutex_lock(&eof_mutex);
            eof_reached = 1;
            pthread_mutex_unlock(&eof_mutex);
            pthread_cond_broadcast(&queue.not_empty); // Notify all writers
            pthread_mutex_unlock(&queue.mutex);
            break;
        }

        pthread_cond_signal(&queue.not_empty); // Signal that the queue is not empty
        pthread_mutex_unlock(&queue.mutex);
    }

    return NULL;
}

void *writer_thread(void *arg)
{
    FILE *destination_file = (FILE *)arg;

    while (1)
    {
        pthread_mutex_lock(&queue.mutex);

        // Wait until the queue is not empty
        while (queue.count == 0)
        {
            pthread_mutex_lock(&eof_mutex);
            if (eof_reached)
            {
                pthread_mutex_unlock(&eof_mutex);
                pthread_mutex_unlock(&queue.mutex);
                return NULL; // Exit the thread if EOF and the queue is empty
            }
            pthread_mutex_unlock(&eof_mutex);
            pthread_cond_wait(&queue.not_empty, &queue.mutex);
        }

        // Retrieve the line from the queue
        char *line = queue.lines[queue.head];
        queue.head = (queue.head + 1) % QUEUE_SIZE;
        queue.count--;

        pthread_cond_signal(&queue.not_full); // Signal that the queue is not full
        pthread_mutex_unlock(&queue.mutex);

        // Write the line to the destination file
        fputs(line, destination_file);
        free(line); // Free the memory allocated for the line
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int ret;
    
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <n> <source_file> <destination_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n < 2 || n > 10)
    {
        fprintf(stderr, "Error: n must be between 2 and 10\n");
        return EXIT_FAILURE;
    }

    // Open source and destination files
    FILE *source_file = fopen(argv[2], "r");
    if (source_file == NULL)
    {
        perror("Error opening source file");
        return EXIT_FAILURE;
    }

    FILE *destination_file = fopen(argv[3], "w");
    if (destination_file == NULL)
    {
        perror("Error opening destination file");
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

    pthread_t readers[n], writers[n];

    // Create reader and writer threads
    for (int i = 0; i < n; ++i)
    {
        ret = pthread_create(&readers[i], NULL, reader_thread, source_file);
        ret = pthread_create(&writers[i], NULL, writer_thread, destination_file);
        if (ret) print_error("Error: Creation of thread error");
    }

    // Wait for all threads to finish
    for (int i = 0; i < n; ++i) {
        ret = pthread_join(readers[i], NULL);
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
