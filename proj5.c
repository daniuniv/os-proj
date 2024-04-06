#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

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
        // Print entry info.
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

    // Open file for writing
    int output_fd = open("snapshot", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (output_fd == -1) {
        printf("Error opening file.\n");
        return 1;
    }

    // Call function with specified directory.
    function(argv[1], output_fd);

    // Close file
    close(output_fd);

    return 0;
}


