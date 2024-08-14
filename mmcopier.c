#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

// Structure to hold the source and destination file names
typedef struct {
    char src_file[256];
    char dst_file[256];
} ThreadData;

// Function to copy a file, this will run in each thread
void *copy_file(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int src_fd, dst_fd;
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];

    // Open the source file
    src_fd = open(data->src_file, O_RDONLY);
    if (src_fd < 0) {
        perror("Error opening source file");
        pthread_exit(NULL);
    }

    // Open/create the destination file
    dst_fd = open(data->dst_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("Error opening destination file");
        close(src_fd);
        pthread_exit(NULL);
    }

    // Read from source and write to destination
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written < 0) {
            perror("Error writing to destination file");
            close(src_fd);
            close(dst_fd);
            pthread_exit(NULL);
        }
    }

    // Close the files
    close(src_fd);
    close(dst_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_files> <source_dir> <destination_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_files = atoi(argv[1]);  // Convert the first argument to an integer
    char *source_dir = argv[2];     // Source directory path
    char *destination_dir = argv[3]; // Destination directory path

    pthread_t threads[num_files];  // Array to hold thread IDs
    ThreadData thread_data[num_files];  // Array to hold file paths for each thread

    // Create threads to copy files
    for (int i = 0; i < num_files; i++) {
        snprintf(thread_data[i].src_file, sizeof(thread_data[i].src_file), "%s/source%d.txt", source_dir, i + 1);
        snprintf(thread_data[i].dst_file, sizeof(thread_data[i].dst_file), "%s/destination%d.txt", destination_dir, i + 1);

        if (pthread_create(&threads[i], NULL, copy_file, &thread_data[i]) != 0) {
            perror("Error creating thread");
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_files; i++) {
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}

