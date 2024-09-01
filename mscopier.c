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

// Error handling function: prints out error message
int print_error(char *msg)
{
    fprintf(stderr, "%s %s\n", msg, strerror(errno));
    exit(2);
}

// Structure to hold the shared queue
typedef struct
{
    char *lines[QUEUE_SIZE];  // Array to hold lines
    int sequence[QUEUE_SIZE]; // Sequence numbers to ensure correct order
    int head;                 // Index for the front of the queue
    int tail;                 // Index for the end of the queue
    int count;                // Number of items in the queue
    int current_sequence;     // The current sequence number for the next line added to the queue
    pthread_mutex_t mutex;    // Mutex to protect access to the queue
    pthread_cond_t not_empty; // Condition variable to signal that the queue is not empty
    pthread_cond_t not_full;  // Condition variable to signal that the queue is not full
} SharedQueue;

// Shared queue initialized with default values
SharedQueue queue = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .current_sequence = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

pthread_mutex_t eof_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect the end-of-file status
int eof_status = 0;                                    // Status to indicate if the end-of-file is reached
int next_line_to_write = 0;                            // Next line number to write to the destination file

/*
 * This function is executed by the reader threads. It reads lines from the source file and adds them to the shared queue.
 
 *  args: A pointer to the source file
 *  returns: NULL
 */
void *reader_thread(void *arg) {
    FILE *source_file = (FILE *)arg;
    char *line_pointer = NULL; // Pointer to store the line read from the file
    size_t length = 0;         // Variable to store the length of the line
    bool loop_run = true;      // Control variable to manage the loop execution

    while (loop_run) {
        pthread_mutex_lock(&queue.mutex); // Locks the queue for thread-safe access

        // Wait if the queue is full
        while (queue.count == QUEUE_SIZE) {
            pthread_cond_wait(&queue.not_full, &queue.mutex);
        }

        // Reads a line from the source file
        ssize_t read_len = getline(&line_pointer, &length, source_file);
        // If read line is successful
        if (read_len != -1) {
            queue.lines[queue.tail] = line_pointer;        // Store the pointer to the read line in the queue at the current tail position
            queue.sequence[queue.tail] = queue.current_sequence++; // Store the current sequence number and increment it for the next line
            queue.tail = (queue.tail + 1) % QUEUE_SIZE;    // Move the tail index forward, wrapping around if it reaches the end of the queue
            queue.count++;                                 // Increment queue count
            line_pointer = NULL;                           // Reset line_pointer to NULL to allow getline to allocate a new buffer
            length = 0;                                    // Reset the length variable for the next getline call
        }
        else {
            eof_status = 1;                           // Sets EOF status to indicate the end of the file
            pthread_cond_broadcast(&queue.not_empty); // Notify all waiting writer threads
            pthread_mutex_unlock(&queue.mutex);       // Unlock the queue mutex
            loop_run = false;                         // Exit the loop
        }

        pthread_cond_signal(&queue.not_empty); // Signal that the queue is not empty
        pthread_mutex_unlock(&queue.mutex);    // Unlock the queue
    }
    free(line_pointer); // Free the memory allocated for the line
    return NULL;
}

/*
 * This function is executed by the writer threads. It retrieves lines from the shared queue and writes them to the destination file.
 
 *  args: A pointer to the destination file
 *  returns: NULL
 */
void *writer_thread(void *arg) {
    FILE *destination_file = (FILE *)arg;
    bool loop_run = true; // Control variable to manage the loop execution

    while (loop_run) {
        pthread_mutex_lock(&queue.mutex); // Locks the queue for thread-safe access

        // Wait if the queue is empty
        while (queue.count == 0) {   
            // If EOF is reached and queue is empty, unlock the queue mutex and exit the loop
            if (eof_status && queue.count == 0) {                                       
                pthread_mutex_unlock(&queue.mutex); 
                loop_run = false;                   
                break;
            }
            pthread_cond_wait(&queue.not_empty, &queue.mutex);
        }

        // If exiting, skip further execution
        if (!loop_run){ 
            break;
        }

        pthread_mutex_lock(&eof_mutex); // Lock to protect `next_line_to_write`
        if (queue.sequence[queue.head] == next_line_to_write) {
            char *line = queue.lines[queue.head];
            queue.head = (queue.head + 1) % QUEUE_SIZE;
            queue.count--;

            // Increments the next line to write
            next_line_to_write++;

            pthread_cond_signal(&queue.not_full); // Notify readers that space is available
            pthread_mutex_unlock(&queue.mutex);

            // Writes the line to the destination file
            fprintf(destination_file, "%s", line);

            free(line); // Free the memory allocated for the line
        }
        else {
            pthread_mutex_unlock(&queue.mutex); // Always unlock the queue
        }
        pthread_mutex_unlock(&eof_mutex); // Unlocks sequence protection
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    if (argc != 4) {
        fprintf(stderr, "Error: Must provide only three arguments: %s <n> <source_file> <destination_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n < 2 || n > 10) {
        print_error("Error: <n> must be between 2 and 10\n");
    }

    FILE *source_file = fopen(argv[2], "r");
    if (source_file == NULL) {
        fprintf(stderr, "Error: Cannot open source file %s: %s\n", argv[2], strerror(errno));
        return EXIT_FAILURE;
    }

    FILE *destination_file = fopen(argv[3], "wb");
    if (destination_file == NULL) {
        fprintf(stderr, "Error: Cannot open destination file %s: %s\n", argv[3], strerror(errno));
        fclose(source_file);
        return EXIT_FAILURE;
    }

    pthread_t readers[n];
    pthread_t writers[n];

    // Creates reader and writer threads
    for (int i = 0; i < n; ++i) {
        ret = pthread_create(&readers[i], NULL, reader_thread, source_file);
        if (ret)
            print_error("Error: Creation of thread error");
        ret = pthread_create(&writers[i], NULL, writer_thread, destination_file);
        if (ret)
            print_error("Error: Creation of thread error");
    }

    // Waits for all threads to finish
    for (int i = 0; i < n; ++i) {
        ret = pthread_join(readers[i], NULL);
        if (ret)
            print_error("Error: Creation of thread error");
        ret = pthread_join(writers[i], NULL);
        if (ret)
            print_error("Error: Creation of thread error");
    }

    fclose(source_file);
    fclose(destination_file);

    // Clean up mutexes and condition variables
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);
    pthread_mutex_destroy(&eof_mutex);

    return EXIT_SUCCESS;
}
