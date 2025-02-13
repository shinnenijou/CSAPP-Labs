# Chapter 8 异常控制流
 
## 练习题8.1
| 进程对 | 并发的？ |
| ------ | -------- |
| AB     | yes      |
| AC     | no       |
| BC     | yes      |

## 练习题8.2
1. 子进程输出为`p1: x=2\np2: x=2\n`
2. 父进程输出为`p2: x=0\n`

## 练习题8.3
`abcc, acbc, bacc`

## 练习题8.4
一种可能的输出为
```
Hello
0
1
Bye
2
Bye
```
共6行输出

## 练习题8.5
```
#include <unistd.h>

unsigned int snooze(unsigned int secs){
    unsigned int left_secs;
    left_secs = sleep(secs);
    printf("Slept for %d of %d secs.\n", secs - left_secs, secs);
    return left_secs;
}
```

## 练习题8.6
```
#include <stdio.h>

int main(int argc, char **argv, char **envp){
    printf("Command Line arguments:\n");
    for (int i = 0; argv[i] != NULL; ++i){  // argv always end with a NULL
        printf("    argc[%d]: %s\n", i, argv[i]);
    }

    printf("Environment variables:\n");
    for (int i = 0; envp[i] != NULL; ++i){  // envp always end with a NULL
        printf("    envp[%d]: %s\n", i, envp[i]);
    }
}
```

## 练习题8.7
```
#include <unistd.h>

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

    //signal(SIGINT, sigint_handler);
    if(signal(SIGINT, sigint_handler) == SIG_ERR){  // error may occur in signal function
        unix_error("signal error\n");
    }

    snooze(secs);
    return 0;
}
```

## 练习题8.8
`213`