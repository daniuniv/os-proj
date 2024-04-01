#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

void function(const char *dirname) {
    struct dirent *pDirent;
    DIR *pDir;

    // Open directory.
    pDir = opendir(dirname);
    if (pDir == NULL) {
        printf("Cannot open directory '%s'\n", dirname);
        return;
    }

    // Process each entry.
    while ((pDirent = readdir(pDir)) != NULL) {
        // Print entry info.
        printf("--------------------\n");
        printf("d_ino:[%ld]\n", pDirent->d_ino);
        printf("d_off:[%ld]\n", pDirent->d_off);
        printf("d_reclen:[%d]\n", pDirent->d_reclen);
        printf("d_type:[%d]\n", pDirent->d_type);
        printf("d_name:[%s]\n", pDirent->d_name);
        printf("--------------------\n");

        // If entry is a directory, recursively call function.
        if (pDirent->d_type == DT_DIR) {
            // Skip '.' and '..' to avoid infinite loop.
            if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
                continue;

            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirname, pDirent->d_name);
            function(path);
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

    // Call function with specified directory.
    function(argv[1]);

    return 0;
}
