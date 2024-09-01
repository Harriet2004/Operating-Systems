To run the code on the teach servers, we used the 'scp' command as we're users of mac.
To copy files, scp <file_name> sXXXXXXX@jupiter.csit.rmit.edu.au:<destination_directory_name> and to copy directories scp -r <source_directory_name> sXXXXXXX@jupiter.csit.rmit.edu.au:<destination_directory_name>

After copying the files to the teaching servers, they can be run by going to the right directory by using the 'cd' command.
To run the task 1 which is multithreaded multiple file copying, the following command has to be executed: ./mmcopier <number_of_files> source_dir destination_dir
To run the task 2 which is multithreaded single file copying, the following command has to be executed: ./mscopier <n> input_file output_file
An input_file is already present in the code which is known as gen.txt, output_file can be any .txt file and it will be generated.
README.md

Project Title
Multithreading & Synchronisation

Introduction
This project involves developing a multithreaded program in C to perform file copying tasks using synchronization mechanisms such as mutexes and condition variables. The project consists of two tasks:
Task 1: Multithreaded multiple file copying
Task 2: Multithreaded single file copying, emulating a producer-consumer problem.

Compilation Instructions
To compile both tasks, run the following commands:
Firstly using the Makefile to make the files required for the tasks:
make all # Compiles both mmcopier and mscopier
make clean # Cleans up the build files

Execution Instructions
Task 1: Multithreaded Multiple File Copying
Run the following command:
./mmcopier <number_of_files> <source_dir> <destination_dir>
number_of_files: Number of files to copy (must be between 1 and 10).
source_dir: The directory containing the source files (e.g., source_dir/source1.txt).
destination_dir: The target directory where the files will be copied. If no target directory present, program will create one.

Task 2: Multithreaded Single File Copying
To generate a random <source_file> of words for this task, just change directory to the directory 'words' and execute the following commands:
chmod +x generate_text.sh
./generate_text.sh <number_of_words> > <source_file>
To generate just random words use these commands after being in the directory 'words':
chmod +x generate_text.sh
./generate_text.sh <number_of_words>

To execute the task run the following command:
./mscopier <n> <source_file> <destination_file>
n: Number of threads to use (must be between 2 and 10).
source_file: The file to be copied.
destination_file: The target file. If no target file present, program will create one.

Lock and Synchronization Details
Task 1: mmcopier.c
Locking Mechanism:
No explicit locking is required in this task as each thread operates independently on its own file.

Task 2: mscopier.c
Locking Mechanism:
pthread_mutex_t is used to protect access to the shared queue (queue.mutex).
pthread_cond_t is used for condition variables:
queue.not_empty: Signaled when the queue is not empty.
queue.not_full: Signaled when the queue is not full.

Critical Sections:
Locks are implemented around critical sections in reader_thread and writer_thread to prevent race conditions.

Memory Leak Checks
To check for memory leaks, we used valgrind. Run the following command on the university server:
valgrind --leak-check=full ./mmcopier <number_of_files> <source_dir> <destination_dir>
valgrind --leak-check=full ./mscopier <n> <source_file> <destination_file>
Results indicate that there are no memory leaks, as all heap blocks were freed, and no errors were detected.

File Transfer to University Server
To transfer files to the university server, use the following scp commands:
To copy files:
scp <file*name> <server_name>:<destination_directory_name>
To copy directories:
scp -r <source_directory_name> <server_name>:<destination_directory_name>

Contact Information
Group Members:
Member 1: Harriet Matthew, s3933558
Member 2: Lakshay Arora, s3988616