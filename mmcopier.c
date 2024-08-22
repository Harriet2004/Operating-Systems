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

    int print_error(char *msg) {
        fprintf(stderr, "%s %s\n", msg, strerror(errno));
        exit(2);
    }

typedef struct 
{
    char source_file[FILE_LENGTH];
    char destination_file[FILE_LENGTH];
} FileData;

void *fileCopy(void *arg) {
    FileData *file_data = (FileData *)arg;
    FILE *source_file;
    FILE *dest_file;
    size_t num_bytes_read;
    size_t num_bytes_written;
    char file_buffer[BUFFER];

    source_file = fopen(file_data->source_file, "rb");
    if (source_file == NULL) {
        fprintf(stderr, "Error while opening the source file %s: %s\n", file_data->source_file, strerror(errno));
        pthread_exit(NULL);
    }

    dest_file = fopen(file_data->destination_file, "wb");
    if (dest_file == NULL) {
        fprintf(stderr, "Error while opening the destination file %s: %s\n", file_data->destination_file, strerror(errno));
        fclose(source_file);
        pthread_exit(NULL);
    }

    while ((num_bytes_read = fread(file_buffer, 1, BUFFER, source_file)) > 0) {
        num_bytes_written = fwrite(file_buffer, 1, num_bytes_read, dest_file);
        if (num_bytes_written < num_bytes_read) {
            fprintf(stderr, "Error while writing to the destination file %s: %s\n", file_data->destination_file, strerror(errno));
            fclose(source_file);
            fclose(dest_file);
            pthread_exit(NULL);
        }
    }

    if (ferror(source_file)) {
        fprintf(stderr, "Error while reading the source file %s\n", file_data->source_file);
    }

    // Close files
    fclose(source_file);
    fclose(dest_file);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int ret;
    const char *destination = "./destination_dir";
    struct stat st = {0};

    if (stat(destination, &st) == -1)
    {
        mkdir(destination, 0755);
    }

    if (argc != 4)
    {
        fprintf(stderr, "Error: Must provide only three arguments: %s <num_files> <source_dir> <destination_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_files = atoi(argv[1]);

    if (num_files < 0 || num_files > 10)
    {   
        print_error("Error: <num_files> must be between 0 and 10");
        return EXIT_FAILURE;
    }

    char *source_dir = argv[2];
    char *destination_dir = argv[3];

    pthread_t thread[num_files];
    FileData thread_data[num_files];

    for (int i = 0; i < num_files; i++)
    {   
        snprintf(thread_data[i].source_file, sizeof(thread_data[i].source_file), "%s/source%d.txt", source_dir, i + 1);
        thread_data[i].source_file[sizeof(thread_data[i].source_file) - 1] = '\0';

        snprintf(thread_data[i].destination_file, sizeof(thread_data[i].destination_file), "%s/source%d.txt", destination_dir, i + 1);
        thread_data[i].destination_file[sizeof(thread_data[i].destination_file) - 1] = '\0';

        ret = pthread_create(&thread[i], NULL, fileCopy, &thread_data[i]);
        if (ret) print_error("Error: Creation of thread error");
    }

    for (int i = 0; i < num_files; i++)
    {
        ret = pthread_join(thread[i], NULL);
        if (ret) print_error("Error: pthread join failed");
    }

    return EXIT_SUCCESS;
}
