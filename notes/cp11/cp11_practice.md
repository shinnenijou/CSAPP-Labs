# Chapter 11 网络编程

## 练习题11.1
| 十六进制地址 | 点分十进制地址  |
| ------------ | --------------- |
| 0x0          | 0.0.0.0         |
| 0xffffffff   | 255.255.255.255 |
| 0x7f000001   | 127.0.0.1       |
| 0xcdbca079   | 205.188.160.121 |
| 0x400c950d   | 64.12.149.13    |
| 0xcdbc9217   | 205.188.146.23  |

## 练习题11.2
完整程序源文件如下
```
#include<arpa/inet.h>
#include<unistd.h>
#include<string>
#include<iostream>

long Stoul(const std::string &__str, size_t *__idx = (size_t *)0, int __base = 10)
{
    try{
        return std::stoul(__str, __idx, __base);
    }
    catch (std::invalid_argument const &ex){
        std::cerr << "invalid argument" <<  std::endl;
        exit(-1);
    }
    catch (std::out_of_range const &ex){
        std::cerr << "out of interger range" <<  std::endl;
        exit(-1);
    }
}

const char* Inet_ntop(int __af, const void *__src, char *__dst, socklen_t __size){
    const char *rc = NULL;
    if(rc = inet_ntop(__af, __src, __dst, __size)){
        return rc;
    }
    else{
        std::cerr << "invalid argument" << std::endl;
        exit(-1);
    }
}

int main(int argc, char **argv){
    if (argc < 2){
        std::cerr << "argument not found" << std::endl;
        exit(-1);
    }
    unsigned uint = Stoul(argv[1], nullptr, 16);
    char ip[16];
    std::cout << std::string(Inet_ntop(AF_INET, &uint, ip, 16)) << std::endl;
    return 0;
}
```

## 练习题11.3
完整程序源文件如下
```
#include<arpa/inet.h>
#include<unistd.h>
#include<string>
#include<iostream>
#include<cstdio>

int Inet_pton(int __af, const char *__src, void *__dst){
    int rc;
    if((rc = inet_pton(__af, __src, __dst)) == 1){
        return rc;
    }
    else if (rc == 0){
        std::cerr << "invalid ip address" << std::endl;
        exit(-1);
    }
    else{
        std::cerr << "invalid argument" << std::endl;
        exit(-1);
    }
}

int main(int argc, char **argv){
    if (argc != 2){
        std::cerr << "arguments number error" << std::endl;
        exit(-1);
    }
    const char *ip = argv[1];
    unsigned uint;
    Inet_pton(AF_INET, ip, &uint);
    uint = ntohl(uint);
    int buf_size = (sizeof(unsigned) << 1) + 1;
    char buf[buf_size];
    snprintf(buf, buf_size, "%x", uint);
    std::cout << "0x" << buf << std::endl;
    return 0;
}
```

## 练习题11.4
完整的源代码如下（好像也没复杂多少？只是addrinfo的结构确实绕不明白）
```
#include<arpa/inet.h>
#include<netdb.h>
#include<iostream>

#define MAXLINE 100

int main(int argc, char **argv)
{
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int rc, flags;

    if (argc != 2){
        std::cerr << "usage: " << argv[0] << " <domain name>" << std::endl;
        exit(0);
    }
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rc = getaddrinfo(argv[1], nullptr, &hints, &listp)) != 0){
        std::cerr << "getaddrinfo error: " << gai_strerror(rc) << std::endl;
        exit(-1);
    }

    for (p = listp; p; p = p->ai_next){
        struct sockaddr_in *addr = (struct sockaddr_in *) p->ai_addr;
        unsigned addr_u = addr->sin_addr.s_addr; // network byte order
        char buf[16];
        std::cout << inet_ntop(p->ai_family, &addr_u, buf, 16) << std::endl; // network byte order
    }

    freeaddrinfo(listp);
    exit(0);
}
```

## 练习题11.5
因为图11-27中的CGI程序的标准输入与标准输被父进程重定向到了socket文件，输入输出的文件位置不会相互影响，也不需要CGI程序主动关闭文件来释放资源（内核会在程序结束时全部关闭），因此可以直接使用标准I/O。

## 练习题11.6

## 练习题11.7
代码diff如下。实际测试发现体积较小的视频文件完全可以正常传输，但体积较大的视频文件会因为客户端提前关闭连接而引起SIGPIPE退出
```
@@ -172,6 +172,8 @@ void get_filetype(char *filename, char *filetype)
         strcpy(filetype, "image/png");
     else if (strstr(filename, ".jpg"))
         strcpy(filetype, "image/jpeg");
+    else if (strstr(filename, ".mp4"))
+        strcpy(filetype, "video/mp4");
     else
         strcpy(filetype, "text/plain");
 }
```
