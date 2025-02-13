#include<unistd.h>
#include<stdio.h>

int main(int argc, char **argv, char **envp){
    printf("Command-line arguments:\n");
    int i = 0;
    while(*(argv + i))
        printf("    argv[%d]: %s\n", i, *(argv + i++));
    i = 0;
    printf("Enviroment variables:\n");
    while(*(envp + i))
        printf("    envp[%d]: %s\n", i, *(envp + i++));
    return 0;
}