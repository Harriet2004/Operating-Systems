To run the code on the teach servers, we used the 'scp' command as we're users of mac. 
To copy files, scp <file_name> sXXXXXXX@jupiter.csit.rmit.edu.au:<destination_directory_name> and to copy directories scp -r <source_directory_name> sXXXXXXX@jupiter.csit.rmit.edu.au:<destination_directory_name>

After copying the files to the teaching servers, they can be run by going to the right directory by using the 'cd' command.
To run the task 1 which is multithreaded multiple file copying, the following command has to be executed: ./mmcopier <number_of_files> source_dir destination_dir
To run the task 2 which is multithreaded single file copying, the following command has to be executed: ./mscopier <n> input_file output_file
An input_file is already present in the code which is known as gen.txt, output_file can be any .txt file and it will be generated.
