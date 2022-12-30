#include <signal.h> // sigemptyset, sigaddset, sigprocmask
#include <sys/signalfd.h> // signalfd
#include <sys/wait.h> // signalfd
#include <unistd.h> // alarm, read
#include <stdio.h> // printf
#include <poll.h> // poll
#include <string.h> // strsignal
#include <fcntl.h> // open
#include <stdlib.h>
#include <sys/stat.h>

volatile sig_atomic_t stop = 0;

pid_t pid;
int status = 0;

void sigterm_handler(int sig){
    printf("SIGTERM handler\n");
    unlink("request");
    stop = 1;
}

int main()
{
    unlink("request");
    if ((mkfifo("request", 0666)) == -1) {

        fprintf(stderr, "Cannot create fifo\n");

        exit(0);

    }
    pid = fork();
    if(pid == 0){
        signal(SIGUSR1, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, sigterm_handler);

        while(!stop) {
            kill(0, SIGUSR1);
            sleep(3);
        }
        exit(0);
    }
    signal(SIGTERM, sigterm_handler);
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig, SIGUSR1);
    sigaddset(&sig, SIGINT);
    sigaddset(&sig, SIGTERM);
    sigprocmask(SIG_BLOCK, &sig, NULL);
    int sigfd = signalfd(-1, &sig, 0);
    int pipefd = open("request", O_RDONLY);

    struct pollfd pfd[] =
            {
                    {.fd = sigfd, .events = POLLIN },
                    {.fd = pipefd, .events = POLLIN }
            };


    while (!stop)
    {
        poll(pfd, 2, -1);
        if ((pfd[0].revents & POLLIN) != 0)
        {
            struct signalfd_siginfo siginfo = {};
            read(sigfd, &siginfo, sizeof(siginfo));
            if(siginfo.ssi_signo == SIGINT) {
                printf("\nCatch SIGINT with\n");
                stop=1;
            }
            else {
                printf("Catch with pid = %d - '%s'\n", pid, strsignal(siginfo.ssi_signo));
            }
        }
        if ((pfd[1].revents & POLLIN) != 0)
        {
            char buf[16];
            int n = read(pipefd, buf, sizeof(buf));
            if (n != 0)
            {
                printf("Text: %.*s", n, buf);
            }
        }
    }

    // close(pipefd);
    unlink("request");
    kill(0, SIGTERM);
    wait(&status);

    printf("Child  %d exit code: %d\n", getpid(), status);
    exit(0);
}
