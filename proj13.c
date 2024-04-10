#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void function(const char *dirname, int output_fd) {
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
        // Print entry name and size.
        char entry_info[500];
        char entry_path[1024];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", dirname, pDirent->d_name);
        struct stat st;
        if (stat(entry_path, &st) == 0) {
            sprintf(entry_info, "--------------------\n"
                                "d_off:[%ld]\n"
                                "d_reclen:[%d]\n"
                                "d_type:[%d]\n"
                                "d_name:[%s]\n"
                                "Size:[%ld bytes]\n"
                                "--------------------\n",
                    pDirent->d_off, pDirent->d_reclen, pDirent->d_type, pDirent->d_name, st.st_size);
            write(output_fd, entry_info, strlen(entry_info));
        } else {
            perror("Error getting file information");
        }
        
        // If entry is a directory, recursively call function.
        if (pDirent->d_type == DT_DIR) {
            // Skip '.' and '..' to avoid infinite loop.
            if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
                continue;

            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirname, pDirent->d_name);
            function(path, output_fd);
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
        function(dirname, current_snapshot_fd);

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
