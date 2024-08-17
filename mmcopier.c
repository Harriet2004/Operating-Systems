#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FILE_LENGTH 256
#define BUFFER 1024

// Structure to hold the source and destination file names
struct FileData
{
    char source_file[FILE_LENGTH];
    char destination_file[FILE_LENGTH];
};

void *fileCopy(void *arg)
{
    struct FileData *file_data = (struct FileData *)arg;
    int source_desc;
    int dest_desc;
    ssize_t num_bytes_read;
    ssize_t num_bytes_written;

    char file_buffer[BUFFER];

    source_desc = open(file_data->source_file, O_RDONLY);
    if (source_desc < 0)
    {
        fprintf(stderr, "Error while opening the source file %s: %s\n", file_data->source_file, strerror(errno));
        pthread_exit(NULL);
    }

    dest_desc = open(file_data->destination_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_desc < 0)
    {
        fprintf(stderr, "Error while opening the destination file %s: %s\n", file_data->destination_file, strerror(errno));
        close(source_desc);
        pthread_exit(NULL);
    }

    while ((num_bytes_read = read(source_desc, file_buffer, BUFFER)) > 0)
    {
        num_bytes_written = write(dest_desc, file_buffer, num_bytes_read);
        if (num_bytes_written < 0)
        {
            fprintf(stderr, "Error while writing to the destination file %s: %s\n", file_data->destination_file, strerror(errno));
            close(source_desc);
            close(dest_desc);
            pthread_exit(NULL);
        }
    }
    close(source_desc);
    close(dest_desc);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    const char *destination = "./destination_dir";
    struct stat st = {0};

    if (stat(destination, &st) == -1)
    {
        mkdir(destination, 0755);
    }

    if (argc != 4)
    {
        fprintf(stderr, "Error: Must provide only three arguments: <num_files> <source_dir> <destination_dir>\n");
        return EXIT_FAILURE;
    }

    int num_files = atoi(argv[1]);

    if (num_files < 0 || num_files > 10)
    {
        fprintf(stderr, "Error: <num_files> should be between 0 and 10.\n");
        return EXIT_FAILURE;
    }

    char *source_dir = argv[2];
    char *destination_dir = argv[3];

    pthread_t thread[num_files];
    struct FileData thread_data[num_files];

    char source_path[FILE_LENGTH];
    char destination_path[FILE_LENGTH];

    // Create threads to copy files
    for (int i = 0; i < num_files; i++)
    {
        sprintf(source_path, "%s/source%d.txt", source_dir, i + 1);
        strncpy(thread_data[i].source_file, source_path, sizeof(thread_data[i].source_file) - 1);
        thread_data[i].source_file[sizeof(thread_data[i].source_file) - 1] = '\0';

        sprintf(destination_path, "%s/source%d.txt", destination_dir, i + 1);
        strncpy(thread_data[i].destination_file, destination_path, sizeof(thread_data[i].destination_file) - 1);
        thread_data[i].destination_file[sizeof(thread_data[i].destination_file) - 1] = '\0';

        int thread_creation = pthread_create(&thread[i], NULL, fileCopy, &thread_data[i]);
        if (thread_creation != 0)
        {
            fprintf(stderr, "Error: Creation of thread error %s\n", strerror(thread_creation));
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_files; i++)
    {
        pthread_join(thread[i], NULL);
    }

    return EXIT_SUCCESS;
}
