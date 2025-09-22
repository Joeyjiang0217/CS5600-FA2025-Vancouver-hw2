#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        printf("child process. child pid:%d, parent=%d\n", getpid(), getppid());

    } else {
        printf("parent process. parent pid:%d, child=%d\n", getpid(), pid);
        pid_t ret = waitpid(pid, NULL, 0);
        printf("parent process. after parent waitpid(), ret = %d\n", ret);
    }

    return 0;
}
