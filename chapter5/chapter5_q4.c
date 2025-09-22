#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>


int main() {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {  
        char *argv[] = { "ls", "-l", NULL };
        char *envp[] = {"MYFLAG=1", NULL };
        execl("/bin/ls", "ls", "-l", NULL);
        execle("/bin/ls", "ls", "-l", NULL, envp);
        execlp("ls", "ls", "-l", NULL);
        execv("/bin/ls", argv);
        execvp("ls", argv);
        execvpe("ls", argv, envp);
    } 
    else 
    {        
    }
    return 0;
}
