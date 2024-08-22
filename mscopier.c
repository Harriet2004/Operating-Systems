#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

    int print_error(char *msg) {
        fprintf(stderr, "%s\n", msg);
        exit(2);
    }
    
// Shared data structures
char buffer[BUFFER_SIZE];
int buffer_filled = 0;  // 0 means empty, >0 means filled
int eof_reached = 0;

// Mutex and condition variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;

// File pointers
FILE *source_file, *destination_file;

void *reader_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        
        // Wait until the buffer is empty
        while (buffer_filled > 0) {
            pthread_cond_wait(&buffer_not_full, &mutex);
        }

        // Read data from the source file into the buffer
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, source_file);

        if (bytes_read == 0) {  // End of file reached
            eof_reached = 1;
            pthread_cond_signal(&buffer_not_empty);  // Notify any waiting writer
            pthread_mutex_unlock(&mutex);
            break;
        }

        buffer_filled = bytes_read;
        pthread_cond_signal(&buffer_not_empty);  // Signal that buffer has data
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void *writer_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait until the buffer has data
        while (buffer_filled == 0 && !eof_reached) {
            pthread_cond_wait(&buffer_not_empty, &mutex);
        }

        // If EOF and buffer is empty, exit the loop
        if (eof_reached && buffer_filled == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Write data from the buffer to the destination file
        fwrite(buffer, 1, buffer_filled, destination_file);

        buffer_filled = 0;
        pthread_cond_signal(&buffer_not_full);  // Signal that buffer is empty
        pthread_mutex_unlock(&mutex);
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
    if (n < 2 || n > 10) {
        fprintf(stderr, "Error: n must be between 2 and 10\n");
        return EXIT_FAILURE;
    }

    // Open source and destination files
    source_file = fopen(argv[2], "rb");
    if (source_file == NULL) {
        perror("Error opening source file");
        return EXIT_FAILURE;
    }

    destination_file = fopen(argv[3], "wb");
    if (destination_file == NULL) {
        perror("Error opening destination file");
        fclose(source_file);
        return EXIT_FAILURE;
    }

    pthread_t readers[n], writers[n];

    // Create reader and writer threads
    for (int i = 0; i < n; ++i) {
        ret = pthread_create(&readers[i], NULL, reader_thread, NULL);
        ret = pthread_create(&writers[i], NULL, writer_thread, NULL);
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
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&buffer_not_empty);
    pthread_cond_destroy(&buffer_not_full);

    return EXIT_SUCCESS;
}
