#include "../common/rio.h"
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv){
    if (argc != 3){
        return -1;
    }

    char *filename = argv[1];

    int n = atoi(argv[2]);
    if(n < 0){
        return -1;
    }

    rio_t rio;
    int fd = Open(filename, O_RDONLY);
    char *buf = (char *)malloc(sizeof(char) * (n + 1));
    ssize_t rc;

    rio_readinitb(&rio, fd);
    if ((rc = rio_readnb(&rio, buf, n)) >= 0){
        printf("%ld\n", rc);
        buf[rc] = '\0';
        printf("%s", buf);
    }

    return 0;
}