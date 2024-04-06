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
        // Print entry name.
        char entry_info[500];
        sprintf(entry_info, "--------------------\nd_ino:[%ld]\nd_off:[%ld]\nd_reclen:[%d]\nd_type:[%d]\nd_name:[%s]\n--------------------\n",
                pDirent->d_ino, pDirent->d_off, pDirent->d_reclen, pDirent->d_type, pDirent->d_name);
        write(output_fd, entry_info, strlen(entry_info));
        
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

int main(int argc, char **argv) {
    // Ensure correct argument count.
    if (argc != 2) {
        printf("Usage: testprog <dirname>\n");
        return 1;
    }

    // Open file for writing (current snapshot)
    int current_snapshot_fd = open("snapshot", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (current_snapshot_fd == -1) {
        printf("Error opening current snapshot.\n");
        return 1;
    }

    // Call function with specified directory to create current snapshot.
    function(argv[1], current_snapshot_fd);

    // Close current snapshot file
    close(current_snapshot_fd);

    // Open previous snapshot file
    int prev_snapshot_fd = open("prev_snapshot", O_RDONLY);
    if (prev_snapshot_fd == -1) {
        printf("No previous snapshot found. Creating an empty previous snapshot.\n");
        // Create an empty previous snapshot file
        prev_snapshot_fd = open("prev_snapshot", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (prev_snapshot_fd == -1) {
            printf("Error creating previous snapshot.\n");
            return 1;
        }
        close(prev_snapshot_fd);
        return 0;  // Exit since there's no previous snapshot to compare
    } else {
        // Close previous snapshot file
        close(prev_snapshot_fd);
    }

    // Compare the two snapshots
    int current_fp = open("snapshot", O_RDONLY);
    int prev_fp = open("prev_snapshot", O_RDONLY);
    if (current_fp == -1 || prev_fp == -1) {
        printf("Error opening snapshot files for comparison.\n");
        return 1;
    }

    char current_buffer[BUFFER_SIZE];
    char prev_buffer[BUFFER_SIZE];

    // Read contents of current snapshot
    ssize_t current_size = read(current_fp, current_buffer, BUFFER_SIZE);
    // Read contents of previous snapshot
    ssize_t prev_size = read(prev_fp, prev_buffer, BUFFER_SIZE);

    if (current_size != prev_size || memcmp(current_buffer, prev_buffer, current_size) != 0) {
        // Differences found, print a message to indicate changes
        printf("Changes detected since the previous run:\n");
        write(STDOUT_FILENO, current_buffer, current_size);
    } else {
        printf("No changes detected since the previous run.\n");
    }

    // Close files
    close(current_fp);
    close(prev_fp);

    // Copy current snapshot to previous snapshot
    if (rename("snapshot", "prev_snapshot") != 0) {
        printf("Error copying current snapshot to previous snapshot.\n");
        return 1;
    }

    return 0;
}


