#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(void) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(1);
    }
    if (pid1 == 0) {
        close(fd[0]); 
        dup2(fd[1], STDOUT_FILENO); 
        close(fd[1]);

        printf("child1 process wrote in....\n");
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(1);
    }
    if (pid2 == 0) {
        close(fd[1]);               
        dup2(fd[0], STDIN_FILENO);   

        char test[50];
        size_t len1 = strlen("child1 process ");
        size_t len2 = strlen("wrote in....\n");
        read(STDIN_FILENO, test, len1);
        read(fd[0], test + len1, len2);
        printf("%s", test);
    }

    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
