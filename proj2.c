#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

void function(DIR *pDir,struct dirent *pDirent)
{
    
    // Process each entry.

    while ((pDirent = readdir(pDir)) != NULL) {
        printf("--------------------\n");
        printf ("d_ino:[%ld]\n", pDirent->d_ino);
        printf ("d_off:[%ld]\n", pDirent->d_off);
        printf ("d_reclen:[%d]\n", pDirent->d_reclen);
        printf ("d_type:[%d]\n", pDirent->d_type);
        printf ("d_name:[%s]\n", pDirent->d_name);
        printf("--------------------\n");
    }
    //return 0;
}

int main(int argc, char** argv)
{
    struct dirent *pDirent;
    DIR *pDir;

    // Ensure correct argument count.

    if (argc != 2) {
        printf ("Usage: testprog <dirname>\n");
        return 1;
    }

    pDir = opendir (argv[1]);

     // Ensure we can open directory.

    if (pDir == NULL) {
        printf ("Cannot open directory '%s'\n", argv[1]);
        return 1;
    }

    function(pDir,pDirent);
    // Close directory and exit.

    closedir (pDir);
    return 0;
    
        return 0;
}