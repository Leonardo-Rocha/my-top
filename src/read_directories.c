#include <stdio.h>
#include <dirent.h>

int main() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("./src")) != NULL) 
    {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            printf ("%s\n", ent->d_name);
        }
        closedir (dir);
    } 
    else 
    {
        /* could not open directory */
        perror ("");
        return -1;
    }

    return 0;
}