#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

void unix_error(char *msg){
    fprintf(stderr, "%s: %s/n", msg, strerror(errno));
    exit(0);
}

pid_t Fork(){
    pid_t pid;
    if ((pid = fork()) < 0){
        unix_error("Fork error");
    }

    return pid;
}

unsigned int snooze(unsigned int secs){
    unsigned int left_secs;
    left_secs = sleep(secs);
    printf("Slept for %d of %d secs.\n", secs - left_secs, secs);
    return left_secs;
}

void sigint_handler(int sig){
    return;
}

int main(int argc, char ** argv, char **envp){
    if(argv[1] == NULL){
        printf("arg error!\n");
        return 0;
    }

    int secs;
    if((secs = atoi(argv[1])) == 0){
        printf("arg error!\n");
    }

    signal(SIGINT, sigint_handler);

    snooze(secs);
    return 0;
}