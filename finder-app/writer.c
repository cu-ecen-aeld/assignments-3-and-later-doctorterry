#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

int main(int argc, char *argv[]){
    
    int fd;

    if(argc != 3){
        printf("WRITER: Invalid number of arguments (exit(1))\n");
        syslog(LOG_ERR, "ERROR: Invalid number of parameters. Failed.");
        exit(1);
    }

    fd = open (argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd == -1){
        printf("WRITER: Failed to open file (exit(1)).\n");
        syslog(LOG_ERR, "ERROR: Could not open file. Failed.");
        exit(1) ;
    }

    write (fd, argv[2], strlen(argv[2]));

    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

    close(fd);
    
    printf("WRITER: Success (exit(0))\n");

    exit(0);

}