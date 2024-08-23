#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FILE_LENGTH 256
#define BUFFER 1024

// Error handling function: prints out error message
int print_error(char *msg) {
    fprintf(stderr, "%s %s\n", msg, strerror(errno));
    exit(2);
}

// Structure to store the source and destination file path which is being used for each thread
typedef struct {
    char source_file[FILE_LENGTH];
    char destination_file[FILE_LENGTH];
} FileData;

/* This function is used to read data from the source file and then create a destination file which does not exist, and write 
* the data into that particular file. It checks for errors such as the existence of files and if any error occurs during the 
* process of reading and writing. After the process is done, it exits from the function either because the file copy process is
* done or if it encounters any such error

*   args: A pointer to the FileData structure which contains the source and the destination file paths
*   returns: This function just exits the thread using 'pthread_exit'.
*/

void *fileCopy(void *arg) {
    FileData *file_data = (FileData *)arg;
    FILE *src_file;
    FILE *dest_file;
    size_t num_bytes_read;
    size_t num_bytes_written;
    char file_buffer[BUFFER];

    // Open the source file
    src_file = fopen(file_data->source_file, "rb");
    // Checks error while opening file
    if (src_file == NULL) {
        fprintf(stderr, "Error while opening the source file %s: %s\n", file_data->source_file, strerror(errno));
        pthread_exit(NULL);
    }

    // Open the destination file
    dest_file = fopen(file_data->destination_file, "wb");
    // Checks error while opening file
    if (dest_file == NULL) {
        fprintf(stderr, "Error while opening the destination file %s: %s\n", file_data->destination_file, strerror(errno));
        fclose(src_file);
        pthread_exit(NULL);
    }

    // Copies data from source file to destination file in the size of BUFFER
    while ((num_bytes_read = fread(file_buffer, 1, BUFFER, src_file)) > 0) {
        num_bytes_written = fwrite(file_buffer, 1, num_bytes_read, dest_file);
        // Checks if there is any error in the process of copying data
        if (num_bytes_written < num_bytes_read) {
            fprintf(stderr, "Error while writing to the destination file %s: %s\n", file_data->destination_file, strerror(errno));
            fclose(src_file);
            fclose(dest_file);
            pthread_exit(NULL);
        }
    }

    // Checks for error while reading data from source file
    if (ferror(src_file)) {
        fprintf(stderr, "Error while reading the source file %s\n", file_data->source_file);
    }

    // Close files and exits thread
    fclose(src_file);
    fclose(dest_file);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int ret;
    // Checking if the destination directory exists, if not it creates one
    const char *destination = "./destination_dir";
    struct stat st = {0};
    if (stat(destination, &st) == -1) 
    {
        mkdir(argv[3], 0755); // Create the directory with read/write/execute permissions
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

    // Arrays for thread data and identifiers
    pthread_t thread[num_files];
    FileData thread_data[num_files];

    for (int i = 0; i < num_files; i++)
    {   
        // Code for the source file path
        snprintf(thread_data[i].source_file, sizeof(thread_data[i].source_file), "%s/source%d.txt", source_dir, i + 1);
        thread_data[i].source_file[sizeof(thread_data[i].source_file) - 1] = '\0';

        // Code for the destination file path
        snprintf(thread_data[i].destination_file, sizeof(thread_data[i].destination_file), "%s/source%d.txt", destination_dir, i + 1);
        thread_data[i].destination_file[sizeof(thread_data[i].destination_file) - 1] = '\0';
        
        // Creates a new thread which is used to copy the file
        ret = pthread_create(&thread[i], NULL, fileCopy, &thread_data[i]);
        if (ret) print_error("Error: Creation of thread error");
    }

    for (int i = 0; i < num_files; i++)
    {   
        // Waiting for all the threads to finis
        ret = pthread_join(thread[i], NULL);
        if (ret) print_error("Error: pthread join failed");
    }

    return EXIT_SUCCESS;
}
