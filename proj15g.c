#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

void check_corruption(const char *file_path, const char *dirname) {
    struct stat st;
    if (stat(file_path, &st) == 0) {
        // Check if permissions are missing
        if ((st.st_mode & S_IRUSR) == 0 || (st.st_mode & S_IWUSR) == 0) {
            // Attempt to grant read and write permissions
            if (chmod(file_path, S_IRUSR | S_IWUSR) == -1) {
                perror("Error granting permissions to the file");
                return; // Exit the function without further action
            }
        
            // Create the contaminated directory if it doesn't exist
            char contaminated_dir[1024];
            snprintf(contaminated_dir, sizeof(contaminated_dir), "%s/contaminated", dirname);
            mkdir(contaminated_dir, S_IRWXU);

            // Create and write to the shell script
            char script_path[1024];
            snprintf(script_path, sizeof(script_path), "%s/corruption_check.sh", dirname);
        
            FILE *script_file = fopen(script_path, "w");
            if (script_file == NULL) {
                perror("Error creating shell script");
                return; // Exit the function without further action
            }
        
            fprintf(script_file, "#!/bin/bash\n");
            fprintf(script_file, "\n");
            fprintf(script_file, "# File path and directory name\n");
            fprintf(script_file, "file=\"%s\"\n", file_path);
            fprintf(script_file, "dirname=\"%s\"\n", dirname);
            fprintf(script_file, "\n");
            fprintf(script_file, "# Debugging message\n");
            fprintf(script_file, "echo \"Script started for $file\"\n");
            fprintf(script_file, "\n");
            fprintf(script_file, "# Define thresholds\n");
            fprintf(script_file, "MIN_LINES=3\n");
            fprintf(script_file, "MAX_WORDS=1000\n");
            fprintf(script_file, "MAX_CHARACTERS=2000\n");
            fprintf(script_file, "\n");
            fprintf(script_file, "# Function to count lines, words, characters, non-ASCII, and specific keywords in a file\n");
            fprintf(script_file, "count_stats() {\n");
            fprintf(script_file, "    if [[ $(tail -c 1 \"$file\") != $'\\n' ]]; then\n");
            fprintf(script_file, "        echo >> \"$file\"\n");
            fprintf(script_file, "    fi\n");
            fprintf(script_file, "    local file=\"$1\"\n");
            fprintf(script_file, "    local lines=$(wc -l < \"$file\")\n");
            fprintf(script_file, "    local words=$(wc -w < \"$file\")\n");
            fprintf(script_file, "    local characters=$(wc -c < \"$file\")\n");
            fprintf(script_file, "    local non_ascii=$(grep -P \"[^\\x00-\\x7F]\" \"$file\" | wc -l)\n");
            fprintf(script_file, "    local keywords=(\"corrupted\" \"dangerous\" \"risk\" \"attack\" \"malware\" \"malicious\")\n");
            fprintf(script_file, "    local keyword_count=0\n");
            fprintf(script_file, "    for keyword in \"${keywords[@]}\"; do\n");
            fprintf(script_file, "        keyword_count=$((keyword_count + $(grep -oi \"$keyword\" \"$file\" | wc -l)))\n");
            fprintf(script_file, "    done\n");
            fprintf(script_file, "    echo \"$lines $words $characters $non_ascii $keyword_count\"\n");
            fprintf(script_file, "}\n");
            fprintf(script_file, "# Function to move file to contaminated folder\n");
            fprintf(script_file, "move_to_contaminated() {\n");
            fprintf(script_file, "    local file=\"$1\"\n");
            fprintf(script_file, "    local dirname=\"$2\"\n");
            fprintf(script_file, "    mkdir -p \"$dirname/contaminated\"\n");
            fprintf(script_file, "    mv \"$file\" \"$dirname/contaminated/\"\n");
            fprintf(script_file, "}\n");
            fprintf(script_file, "\n");
            fprintf(script_file, "# Main script execution\n");
            fprintf(script_file, "# Count lines, words, characters, non-ASCII, and specific keywords\n");
            fprintf(script_file, "stats=$(count_stats \"$file\")\n");
            fprintf(script_file, "read -r lines words characters non_ascii keyword_count <<< \"$stats\"\n");
            fprintf(script_file, "\n");
            fprintf(script_file, "# Debugging message\n");
            fprintf(script_file, "echo \"File stats: $lines lines, $words words, $characters characters, $non_ascii non-ASCII characters, $keyword_count keywords found\"\n");
            fprintf(script_file, "\n");
            fprintf(script_file, "# Check conditions and move file if necessary\n");
            fprintf(script_file, "if (( lines < MIN_LINES )) && (( words > MAX_WORDS )) && (( characters > MAX_CHARACTERS )) || (( non_ascii > 0 )) || (( keyword_count > 0 )); then\n");
            fprintf(script_file, "    move_to_contaminated \"$file\" \"$dirname\"\n");
            fprintf(script_file, "    echo \"File moved to contaminated directory\"\n");
            fprintf(script_file, "fi\n");
            
            fclose(script_file);

            // Make the script executable
            chmod(script_path, S_IRWXU);

            // Execute the shell script
            if (system(script_path) == -1) {
                perror("Error executing shell script");
            }
        }
    } else {
        perror("Error checking file permissions");
    }
}


void print_file_info(struct dirent *pDirent, const char *path, int output_fd, int depth) {
    char entry_info[500];
    struct stat st;
    if (stat(path, &st) == 0) {
        // Add indentation based on depth
        for (int i = 0; i < depth; i++) {
            write(output_fd, "  ", 2);
        }

        // Print file information
        sprintf(entry_info, "- [%s] (Size: %ld bytes, d_off:[%ld], d_reclen:[%d], d_type:[%d])\n", pDirent->d_name, st.st_size, pDirent->d_off, pDirent->d_reclen, pDirent->d_type);
        write(output_fd, entry_info, strlen(entry_info));
    } 
}

void function(const char *dirname, int output_fd, int depth) {
    struct dirent *pDirent;
    DIR *pDir;

    // Open directory.
    pDir = opendir(dirname);
    if (pDir == NULL) {
        char err_msg[100];
        sprintf(err_msg, "Cannot open directory '%s'\n", dirname);
        write(output_fd, err_msg, strlen(err_msg));
        return;
    }

    // Process each entry.
    while ((pDirent = readdir(pDir)) != NULL) {
        // Construct full path
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirname, pDirent->d_name);

        // Add indentation based on depth
        for (int i = 0; i < depth; i++) {
            write(output_fd, "  ", 2);
        }

        // If entry is a directory, recursively call function.
        if (pDirent->d_type == DT_DIR) {
            // Skip '.' and '..' to avoid infinite loop.
            if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0|| strcmp(pDirent->d_name, "contaminated") == 0)
                continue;

            write(output_fd, "- ", 2);
            write(output_fd, pDirent->d_name, strlen(pDirent->d_name));
            write(output_fd, "\n", 1);

            pid_t child_pid = fork();
            if (child_pid == 0) { // Child process
                function(path, output_fd, depth + 1);
                exit(0);
            } else if (child_pid > 0) { // Parent process
                int status;
                wait(&status);
            } else {
                perror("Error creating child process");
                exit(1);
            }
        } 
        // If entry is a regular file, check for corruption.
        else if (pDirent->d_type == DT_REG && strcmp(pDirent->d_name, "corruption_check.sh") != 0) {
            check_corruption(path, dirname);
            
            // Print file information with indentation.
            print_file_info(pDirent, path, output_fd, depth);
        }
    }

    // Close directory.
    closedir(pDir);
}



int compare_files(const char *file1, const char *file2) {
    int fd1 = open(file1, O_RDONLY);
    int fd2 = open(file2, O_RDONLY);

    if (fd1 == -1 || fd2 == -1) {
        perror("Error opening snapshot files for comparison");
        return -1;
    }

    char buf1[BUFFER_SIZE];
    char buf2[BUFFER_SIZE];
    ssize_t bytes_read1, bytes_read2;

    do {
        bytes_read1 = read(fd1, buf1, BUFFER_SIZE);
        bytes_read2 = read(fd2, buf2, BUFFER_SIZE);

        if (bytes_read1 != bytes_read2 || memcmp(buf1, buf2, bytes_read1) != 0) {
            // Files are different
            close(fd1);
            close(fd2);
            return 1;
        }
    } while (bytes_read1 > 0 && bytes_read2 > 0);

    close(fd1);
    close(fd2);

    // Files are identical
    return 0;
}

int main(int argc, char **argv) {
    // Ensure correct argument count.
    if (argc < 2) {
        printf("Usage: testprog <dirname1> <dirname2> ...\n");
        return 1;
    }
    if (argc > 11) {
        printf("Too many command line arguments(directories), it shouldn't exceed 10 ... \n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char *dirname = argv[i];

        // Open previous snapshot file
        char prev_snapshot_path[1024];
        snprintf(prev_snapshot_path, sizeof(prev_snapshot_path), "%s/prev_snapshot", dirname);
        int prev_snapshot_fd = open(prev_snapshot_path, O_RDONLY);
        if (prev_snapshot_fd == -1) {
            printf("No previous snapshot found for %s. Creating an empty previous snapshot.\n", dirname);
            // Create an empty previous snapshot file
            prev_snapshot_fd = open(prev_snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (prev_snapshot_fd == -1) {
                printf("Error creating previous snapshot for %s.\n", dirname);
                return 1;
            }
            close(prev_snapshot_fd);
        } else {
            // Close previous snapshot file
            close(prev_snapshot_fd);
        }

        // Open file for writing (current snapshot)
        char current_snapshot_path[1024];
        snprintf(current_snapshot_path, sizeof(current_snapshot_path), "%s/snapshot", dirname);
        int current_snapshot_fd = open(current_snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (current_snapshot_fd == -1) {
            printf("Error opening current snapshot for %s.\n", dirname);
            return 1;
        }

        // Call function with specified directory to create current snapshot.
        function(dirname, current_snapshot_fd, 1);

        // Close current snapshot file
        close(current_snapshot_fd);

        // If it's the first run, there's no need to compare snapshots
        if (prev_snapshot_fd == -1) {
            continue;  
        }

        // Compare the two snapshots
        int comparison_result = compare_files(current_snapshot_path, prev_snapshot_path);
        if (comparison_result == -1) {
            return 1; // Error occurred during comparison
        } else if (comparison_result == 1) {
            printf("Changes detected since the previous run for %s\n", dirname);
        } else {
            printf("No changes detected since the previous run for %s.\n", dirname);
        }

        // Copy current snapshot to previous snapshot
        if (rename(current_snapshot_path, prev_snapshot_path) != 0) {
            printf("Error copying current snapshot to previous snapshot for %s.\n", dirname);
            return 1;
        }
    }

    return 0;
}
