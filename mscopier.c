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

typedef struct {
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

SharedQueue queue = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .current_sequence = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

pthread_mutex_t eof_mutex = PTHREAD_MUTEX_INITIALIZER;
int eof_status = 0;
int next_line_to_write = 0;

void *reader_thread(void *arg) {
    FILE *source_file = (FILE *)arg;
    char *line_pointer = NULL;
    size_t length = 0;
    bool loop_run = true;

    while (loop_run) {
        pthread_mutex_lock(&queue.mutex);

        while (queue.count == QUEUE_SIZE) {
            pthread_cond_wait(&queue.not_full, &queue.mutex);
        }
    
        ssize_t read_len = getline(&line_pointer, &length, source_file);
        if (read_len != -1) {
            queue.lines[queue.tail] = line_pointer;  
            queue.sequence[queue.tail] = queue.current_sequence++;
            queue.tail = (queue.tail + 1) % QUEUE_SIZE;
            queue.count++;
            line_pointer = NULL;  
            length = 0;      
        }
        else {
            eof_status = 1;
            pthread_cond_broadcast(&queue.not_empty); 
            pthread_mutex_unlock(&queue.mutex);
            loop_run = false;
        }

        pthread_cond_signal(&queue.not_empty); 
        pthread_mutex_unlock(&queue.mutex);
    }
    free(line_pointer);
    return NULL;
}

void *writer_thread(void *arg) {
    FILE *destination_file = (FILE *)arg;
    bool loop_run = true;

    while (loop_run) {
        pthread_mutex_lock(&queue.mutex);

        while (queue.count == 0) {
            if (eof_status && queue.count == 0) {  // If EOF is reached and queue is empty, exit
                pthread_mutex_unlock(&queue.mutex);
                loop_run = false;  // Exit the loop
                break;
            }
            pthread_cond_wait(&queue.not_empty, &queue.mutex);
        }

        if (!loop_run) {  // If exiting, skip further execution
            break;
        }

        pthread_mutex_lock(&eof_mutex); // Lock to protect `next_line_to_write`
        if (queue.sequence[queue.head] == next_line_to_write) {
            char *line = queue.lines[queue.head];
            queue.head = (queue.head + 1) % QUEUE_SIZE;
            queue.count--;

            // Increment the next line to write
            next_line_to_write++;

            pthread_cond_signal(&queue.not_full);  // Notify readers that space is available
            pthread_mutex_unlock(&queue.mutex);

            // Write the line to the destination file
            fprintf(destination_file, "%s", line);

            free(line);
        } else {
            pthread_mutex_unlock(&queue.mutex);  // Always unlock the queue
        }
        pthread_mutex_unlock(&eof_mutex);  // Unlock sequence protection
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

    for (int i = 0; i < n; ++i) {
        ret = pthread_create(&readers[i], NULL, reader_thread, source_file);
        if (ret) print_error("Error: Creation of thread error");
        ret = pthread_create(&writers[i], NULL, writer_thread, destination_file);
        if (ret) print_error("Error: Creation of thread error");
    }

    for (int i = 0; i < n; ++i) {
        ret = pthread_join(readers[i], NULL);
        if (ret) print_error("Error: Creation of thread error");
        ret = pthread_join(writers[i], NULL);
        if (ret) print_error("Error: Creation of thread error");
    }

    fclose(source_file);
    fclose(destination_file);

    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);
    pthread_mutex_destroy(&eof_mutex);

    return EXIT_SUCCESS;
}
