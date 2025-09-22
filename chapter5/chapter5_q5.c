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

        pid_t ret = wait(NULL);
        printf("child process. after child wait, ret = %d\n", ret);
    } else {
        printf("parent process. parent pid:%d, child=%d\n", getpid(), pid);
        pid_t ret = wait(NULL);
        printf("parent process. after parent wait, ret = %d\n", ret);
    }

    return 0;
}
