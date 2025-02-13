#include<unistd.h>
#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>

unsigned int snooze(unsigned int secs){
    unsigned int remain;
    remain = sleep(secs);
    printf("Slept for %d of %d secs.\n", secs - remain, secs);
    return remain;
}

void sigint_handler(int sig){  /* SIGINT HANDLER */
    return;
}

int main(int argc, char **argv){
    if (signal(SIGINT, sigint_handler) == SIG_ERR){
        fprintf(stderr, "signal error: %s\n", strerror(errno));
        exit(0);
    }
    if (argc == 2){
        unsigned int secs = *(argv[1]) - '0';
        snooze(secs);
    }
    return 0;
}