#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void) {
    int p[2];
    if (pipe(p) == -1) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {              
        write(STDOUT_FILENO, "hello\n", 6);

        close(p[0]);             
        write(p[1], "x", 1);      
        close(p[1]);
        _exit(0);
    } else {                      
        close(p[1]);                
        char dummy;
        read(p[0], &dummy, 1);      
        close(p[0]);

        write(STDOUT_FILENO, "goodbye\n", 8);
    }
    return 0;
}