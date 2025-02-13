- [练习题3.1](#练习题31)
- [练习题3.2](#练习题32)
- [练习题3.3](#练习题33)
- [练习题3.4](#练习题34)
- [练习题3.5](#练习题35)
- [练习题3.6](#练习题36)
- [练习题3.7](#练习题37)
- [练习题3.8](#练习题38)
- [联系题3.9](#联系题39)
- [练习题3.10](#练习题310)
- [练习题3.11](#练习题311)
- [练习题3.12](#练习题312)
- [练习题3.13](#练习题313)
- [练习题3.14](#练习题314)
- [练习题3.15](#练习题315)
- [练习题3.16](#练习题316)
- [练习题3.17](#练习题317)
- [练习题3.18](#练习题318)
- [练习题3.19](#练习题319)
- [练习题3.20](#练习题320)
- [练习题3.21](#练习题321)
- [练习题3.22](#练习题322)
- [联系题3.23](#联系题323)
- [练习题3.24](#练习题324)
- [练习题3.25](#练习题325)
- [练习题3.26](#练习题326)
- [练习题3.27](#练习题327)
- [练习题3.28](#练习题328)
- [练习题3.29](#练习题329)
- [练习题3.30](#练习题330)
- [练习题3.31](#练习题331)
- [练习题3.32](#练习题332)
- [练习题3.33](#练习题333)
- [练习题3.34](#练习题334)
- [练习题3.35](#练习题335)
- [练习题3.36](#练习题336)
- [练习题3.37](#练习题337)
- [练习题3.38](#练习题338)
- [练习题3.39](#练习题339)
- [练习题3.40](#练习题340)
- [练习题3.41](#练习题341)
- [练习题3.42](#练习题342)
- [练习题3.43](#练习题343)
- [练习题4.44](#练习题444)
- [练习题3.45](#练习题345)
- [练习题3.46](#练习题346)
- [练习题3.47](#练习题347)
- [练习题3.48](#练习题348)
- [练习题3.49](#练习题349)
- [练习题3.50](#练习题350)
- [练习题3.51](#练习题351)
- [练习题3.52](#练习题352)
- [练习题3.53](#练习题353)
- [练习题3.54](#练习题354)
- [练习题3.55](#练习题355)
- [练习题3.56](#练习题356)
- [练习题3.57](#练习题357)

> # Chapter3 程序的机器级表示

## 练习题3.1
> 假设下面的值存放在指明的内存地址和寄存器中:
>    | 地址  | 值   |
>    | ----- | ---- |
>    | 0x100 | 0xFF |
>    | 0x104 | 0xAB |
>    | 0x108 | 0x13 |
>    | 0x10C | 0x11 |
> 
>    | 寄存器 | 值    |
>    | ------ | ----- |
>    | %rax   | 0x100 |
>    | %rcx   | 0x1   |
>    | %rdx   | 0x3   |
>
> 填写下表，给出所有操作数的值

| 操作数          | 值        |
| --------------- | --------- |
| %rax            | **0x100** |
| 0x104           | **0x104** |
| $0x108          | **0x13**  |
| (%rax)          | **0xFF**  |
| 4(%rax)         | **0xAB**  |
| 9(%rax, %rdx)   | **0x11**  |
| 260(%rcx, %rdx) | **0x13**  |
| 0xFC(, %rcx, 4) | **0x13**  |
| (%rax, %rdx, 4) | **0x11**  |

## 练习题3.2
> 对于下面汇编代码的每一行，根据操作数，确定适当的指令后缀。（例如，`mov`可以被重写成`movb`，`movw`，`movl`或者，`movq`）
```
movl  %eax,(%rsp)
movw  (%rax),%dx
movb  $0xFF,%bl
movb  (%rsp, %rdx, 4),%dl
movq  (%rdx),%rax
movb  %dx,(%rax)
```

## 练习题3.3
> 当我们调用汇编器的时候，下面代码每一行都会产生一个错误消息。解释每一行都是哪里出了错。  

`movb  $0xF,(%ebx)`  寄存器寻址必须是64位寄存器  
`movl  %rax,(%rsp)`  指令`movl`传送4个字节但寄存器`%rax`包含8个字节数据  
`movw  (%rax),4(%rsp)`  不能从内存讲数据传送至内存  
`movb  %al,%sl`  没有名为`%sl`的寄存器  
`movq  %rax,$0x123`  目的操作数不能是直接数  
`movl  %eax,%rdx`  目的寄存器`%rdx`大小(8字节)与指令指定的大小(4字节)不匹配  
`movb  %si,8(%rbp)`  指令`movb`传送1个自己但寄存器`%si`包含2个字节数据

## 练习题3.4
> 假设变量sp和dp被声明为类型  
> `src_t *sp;`  
> `dest_t *dp;`  
> 这里src_t和dest_t是用typedef声明的数据类型。我们想使用适当的数据传送指令来实现下面的操作  
> `*dp = (dest_t) *sp;`  
> 
> 假设sp和dp的值分别存储在寄存器%rdi和%rsi中。对于表中的每个表项，给出实现指定数据传送的两条指令。
> 其中第一条指令应该从内存中读数，做适当的转换，并设置寄存器号rax 的适当部分。
> 然后，第二条指令要把%rax的适当部分写到内存。
> 在这两种情况中，寄存器的部分可以是%rax、%eax、%ax 或%a1，两者可以互不相同。
> 记住，当执行强制类型转换既涉及大小变化又涉及C语言中符号变化时，操作应该先改变大小。

| src_t         | dest_t        | 指令                 |
| ------------- | ------------- | -------------------- |
| long          | long          | `movq (%rdi),%rax`   |
|               |               | `movq %rax,(%rsi)`   |
| char          | int           | `movsbl (%rdi),%eax` |
|               |               | `movl %eax,(%rsi)`   |
| char          | unsigned      | `movsbl (%rdi),%eax` |
|               |               | `movl %eax,(%rsi)`   |
| unsigned char | long          | `movzbl (%rdi),%eax` |
|               |               | `movl %eax,(%rsi)`   |
| int           | char          | `movb (%rdi),%al`    |
|               |               | `movb %al,(%rsi)`    |
| unsigned      | unsigned char | `movb (%rdi),%al`    |
|               |               | `movb %al,(%rsi)`    |
| char          | short         | `movsbw (%rdi),%ax`  |
|               |               | `movw %ax,(%rsi)`    |

## 练习题3.5
> 已知信息如下。将一个原型为  
> `void decode1(long *xp, long *yp, long *zp);`  
> 的函数编译成汇编代码，得到如下代码
> ```
>   void decode1 (long *xp, long *yp, long *zp)
>   xp in %rdi, yp in %rsi, zp in %rdx
> decode1:
>   movq  (%rdi), %r8
>   movq  (%rsi), %rcx
>   movq  (%rdx), %rax
>   movq  %r8, (%rsi)
>   movq  %rcx, (%rdx)
>   movq  %rax, (%rdi)
>   ret
> ```
> 参数xp、v和20分别存储在对应的寄存器号rdi.grsi和grdx中。请写出等效于上面汇编代码的 decode1 的C代码。

```
void decode1(long *xp, long *yp, long *zp)
{
    long temp1 = *xp, temp2 = *yp, temp3 = *zp;
    *yp = temp1;
    *zp = temp2;
    *xp = temp3;
    return temp3;
}
```

## 练习题3.6
> 假设寄存器%rax的值为x，%rcx的值为y。填写下表，指明下面每条汇编代码指令存储在寄存器%rdx中的值：

| 表达式                         | 结果         |
| ------------------------------ | ------------ |
| `leag 6(%rax), %rdx`           | `x + 6`      |
| `leaq (%rax，%rcx), %rdx`      | `x + y`      |
| `leaq (%rax，%rcx, 4), %rdx`   | `x + 4y`     |
| `leaq 7(%rax, %rax, 8), %rdx`  | `x + 8y + 7` |
| `leaq 0xA(, %rcx, 4), %rdx`    | `4y + 10`    |
| `leaq 9 (%rax, %rcx, 2), &rdx` | `x + 2y + 9` |

## 练习题3.7
> 考虑下面的代码，我们省略了被计算的表达式：
> ```
> 1ong scale2 (1ong x, 1ong y, 1ong z) {
>     long t = _______________;
>     return t;
> }
> ```
> 用 GCC 编译实际的函数得到如下的汇编代码：
> ```
>   1ong scale2(1ong x, 1ong y, 10ng z)
>   x in %rdi, y in %rsi, z in %rdx
> scale2:
> leaq (%rdi, %rdi, 4), %rax
> leag (%rax, %rsi, 2), %rax
> leaq (%rax, %rdx, 8), %rax
> ret
> ```
> 填写出代码中缺失的表达式

```
1ong scale2 (1ong x, 1ong y, 1ong z) {
    long t = 5x + 2y + 8z;
    return t;
}
```

## 练习题3.8
> 假设下面的值存放在指明的内存地址和寄存器中:
>    | 地址  | 值   |
>    | ----- | ---- |
>    | 0x100 | 0xFF |
>    | 0x104 | 0xAB |
>    | 0x108 | 0x13 |
>    | 0x10C | 0x11 |
> 
>    | 寄存器 | 值    |
>    | ------ | ----- |
>    | %rax   | 0x100 |
>    | %rcx   | 0x1   |
>    | %rdx   | 0x3   |
>
> 填写下表，给出下面指令的效果，说明将被更新的寄存器或内存位置，以及得到的值：

| 指令                         | 目的                                     | 值    |
| ---------------------------- | ---------------------------------------- | ----- |
| `addq %rcx, (%rax)`          | 在寄存器`%rax`保存的值加上`%rcx`保存的值 | 0x100 |
| `subq %rdx, 8(%rax)`         | 从地址`%rax + 8`保存的值中减去`%rdx`的值 | 0xA8  |
| `imulq $16, (%rax, %rdx, 8)` | 将地址`(%rax + 8 * %rdx)`保存的值乘以16  | 0x110 |
| `incq 16(%rax)`              | 将地址`%rax + 16`保存的值加1             | 0x14  |
| `decq %rcx`                  | 将寄存器`%rcx`保存的值减1                | 0x0   |
| `subq %rdx, %rax`            | 从寄存器`%rax`的值中减去`%rdx`的值       | 0xFD  |

## 联系题3.9
> 假设我们想生成以下C函数的汇编代码：
> ```
> long shift_left4_rightn (long x, long n)
> {
>     x <<= 4:
>     x >>= n;
>     return x;
> }
> ```
> 下面这段汇编代码执行实际的移位，并将最后的结果放在奇存器`%rax`中。此处省略了两条关键的指令。
> 参数`x`和`n`分别存放在寄存器`%rdi`和`%rsi`中。
>
> 根据右边的注释，填出缺失的指令。请使用算术右移操作。

```
  1ong shift_left4_rightn (1ong x, 1ong n)
  x in %rdi, n in %rsi
shift_left4_rightn:
  movq  %rdi, %rax  # Get x
  salq  $4, %rax    # x <<= 4
  movl  %esi, %ecx  # Get n (4 bytes)
  sarq  %cl, %rax   # x >>= n
  ret
```

## 练习题3.10
> 下面的函数是图3-11a中函数一个变种，其中有些表达式用空格替代  
> 实现这些表达式的汇编代码如下：
> ```
>   long arith2 (long ×, long y, long z)
>   x in %rdi, y in %rsi, z in %rdx
> arith2:
>   orq   %rsi, %rdi
>   sarq  $3, %rdi
>   notq  %rdi
>   movq  %rdx, %rax
>   subq  %rdi, %rax
>   ret
> ```
> 基于这些汇编代码，填写C语言代码中缺失的部分。

```
long arith2(long x, long y, long z)
{
    long t1 = x | y;
    long t2 = t1 >> 3;
    long t3 = ~t2;
    long t4 = z - t3;
    return t4;
}
```

## 练习题3.11
> 常常可以看见以下形式的汇编代码行  
> `xorq %rdx, %rdx`  
> 但是在产生这段汇编代码的C代码中，并没有出现 EXCLUSIVE-OR 操作。  
> 1. 解释这条特殊的 EXCIUSIVE-OR 指今的效果，它实现了什么有用的操作。  
> 2. 更直接地表达这个操作的汇编代码是什么？  
> 3. 比较同样一个操作的两种不同实现的编码字节长度  

1. 该指令将`%rdx`的值与自身异或后更新`%rdx`。该指令实现了将寄存器保存的所有位重置为0。
2. `movl $0, %edx`
3. 使用异或指令只需要2个字节(指令+寄存器)，使用移动指令需要5个字节(指令+直接数+寄存器)

## 练习题3.12
> 考虑如下函数，它计算两个无符号64位数的商和余数：
> ```
> void uremdiv (unsigned long x, unsigned long y,
>               unsigned long *qp, unsigned long *rp) {
>   unsigned long q = x/y;
>   unsigned long r = x%y;
>   *qp = 0;
>   *rp =r;
> }
> ```
> 修改有符号除法的汇编代码来实现这个函数。

```
  void uremdiv (unsigned 1ong X, unsigned 1ong y
                unsigned long *qp, unsigned long *rp)
  x in %rdi, y in %rsi, ap in %rdx, rp in %rcx
uremdiv:
  movq  %rdx, %r8       # Copy qp
  movq  %rdi, %rax      # Move x to lower 8 bytes of dividend
  xorl  %rdx, %rdx      # Zero-extend to upper 8 bytes of dividend
  divq  %rsi            # Divide by y
  movq  %rax, (%r8)     # Store quitient at qp
  movq  %rdx, (%rcx)    # Store remainder at rp
  ret
```

## 练习题3.13
> 考虑下列的C语言代码：
> ```
> int comp(data_t a, data_t b){
>   return a COMP b;
> }
> ```
> 它给出了参数`a`和`b`之间比较的一般形式，这里，参数的数据类型`data_t`
> (通`过typedef`)被声明为表3-1中列出的某种整数类型，可以是有符号的也可以是无符号的。
> `comp`通过`#define`来定义。
>   
> 假设`a`在`%rdi`中某个部分，上在品`%rsi`中某个部分。
> 对于下面每个指令序列，确定哪种数据类型`data_t`和比较`COMP`会导致编译器产生这样的代码。
> （可能有多个正确答案，请列出所有的正确答案。

1. `cmpl  %esi, %edi`  比较32位数据 
   `setl  %al`  设置条件是有符号的小于，因此`data_t`为`int`，`COMP`为`<`
2. `cmpw  %si, %di`  比较16位数据  
   `setge %al`  设置条件是有符号的大于等于，因此`data_t`为`short`，`COMP`为`>=`  
3. `cmpb  %sil, %dil`  比较8位数据  
   `%setbe  %al`  设置条件是无符号的低于等于，因此`data_t`为`unsigned char`，`COMP`为`<=`  
4. `cmpq  %rsi, %rdi`  比较64位数据  
   `setne %al`  设置条件是不等，因此data_t可能为`long`或`char*`，`COMP`为`!=`

## 练习题3.14
> 考虑下面的C语言代码：
> ```
> int test(data_t a){
>   return a TEST 0;  
> }
> ```
> 它给出了参数a和口之问比较的一般形式，这里，我们可以用`typedef`来声明`data_t`，从而设置参数的数据类型；用`#define`来声明`Test`，从而设置比较的类型。
> 
> 对于下面每个指令序列，确定哪种数据类型`data_t` 和比较`TEST`会导致编译器产生这样的
代码。（可能有多个正确答案，请列出所有的正确答案。)

1. `testq  %rdi, %rdi`  比较64位数据  
   `setge  %al`  设置条件是有符号的大于等于，因此`data_t`为`long`，`COMP`为`>=`
2. `testw  %di, %di`  比较16位数据  
   `sete %al`  设置条件是等于，因此`data_t`为`short`，`COMP`为`==`  
3. `testb  %dil, %dil`  比较8位数据  
   `%seta  %al`  设置条件是无符号的超过，因此`data_t`为`unsigned char`，`COMP`为`>`  
4. `testl  %edi, %edi`  比较32位数据  
   `setne %al`  设置条件是不等，因此data_t可能为`int`或`unsigned int`，`COMP`为`!=`

## 练习题3.15
> 在下面这些反汇编二进制代码节选中，有些信息被`X`代替了。回答下列关于这些指令的问题。
> 1. 下面je指令的目标是什么？（在此，你不需要知道任何有关callq指令的信息。）  
> ```
>    4003fa: 74 02              je    XXXXXX
>    4003fc: ff d0              callq *%rax  
> ```
> 2. 下面je指令的目标是什么？  
> ```
>    40042f: 74 f4              je    XXXXXX
>    400431: 5d                 pop   %rbp
> ```
> 3. ja和pop指令的地址是多少？  
> ```
>    XXXXXX: 77 02              ja    400547
>    XXXXXX: 5d                 pop   %rbp
> ```
> 4. 在下面的代码中，跳转目标的编码是PC相对的，且是一个4宇节补码数。
> 字节按照从最低位到最高位的顺序列出，反映出x86-64的小端法字节顺序。
> 跳转目标的地址是什么？  
> ```
>    4005e8: e9 73 ff ff ff     jmpq  XXXXXX 
>    4005ed: 90                 nop
> ```

1. `0x4003fe`
2. `0x400425`
3. `0x400543`
   `0x400545`
4. `0x400560`

## 练习题3.16
> 已知下列C代码:
> ```
> void cond(long a, long *p)
> {
>     if (p && a > *p)
>         *p = a;
> }
> ```
> CCC会产生下面的汇编代码:
> ```
>   void cond(long a, long *p)
>   a in %rdi, p in %rsi
> cond:
>   testq  %rsi, %rsi
>   je     .L1
>   cmpq   %rdi, (%rsi)
>   jge    .L1
>   movq   %rdi, (%rsi)
> .L1:
>   rep; ret
> ```
> 1. 按照图3-166中所示的风格，用C语言写一个8ot0版本，执行同样的计算，并模拟汇编代码的控制流。
> 像示例中那样给汇编代码加上注解可能会有所帮助。
> 2. 请说明为什么C语言代码中只有一个if语句，而汇编代码包含两个条件分支。

1. goto风格c
```
void cond(long a, long *p)
{
   if (p == 0)
      goto L1;
   if (*p - a >= 0)
      goto L1;
   *p = a;
L1:
   return;
}
```
2. 由于c代码中的if语句由两个独立的表达式用逻辑与连接，因此实际上分别需要对两个条件独立进行判断。

## 练习题3.17
> 将if语句翻译成goto代码的另一种可行的规则如下:
> ```
>    t = test-expr;
>    if (t)
>       goto true;
>    else-statement;
>    goto done;
> true:
>    then-statement;
> done:
> ```
> 1. 基于这种规则，重写absdiff_se的goto版本。
> 2. 你能想出选用一种规则而不选用另一种规则的理由吗？

1. absdiff_se的goto版本
```
long absdiff_se(long x, long y)
{
   long result;
   if (x < y)
      goto true;
   ge_cnt++;
   result = x - y;
   goto done;
true:
   le_cnt++;
   result = y - x;
done:
   return result;
}
```
2. 编译器采用的规则更容易应用在没有else的if语句中

---
以下开始练习题不再录入题目正文
## 练习题3.18
```
long test(long x, long y, long z){
   long val = x + y + z;
   if (x + 3 < 0){
      if (y - z < 0)
         val = x * y;
      else
         val = y * z;
   }else if (x - 2 > 0)
      val = z * x;
   return val;
}
```

## 练习题3.19
1. 预测错误的处罚大约30个周期
2. 分支预测错误时需要46个周期

## 练习题3.20
1. `#define OP /`
2. 汇编代码
```
  long arith(long x)
  x in %rdi
arith:
  leaq   7(%rdi), %rax     # x为负时需要为x加上偏移量
  testq  %rdi, %rdi        # 测试x的符号
  cmovns %rdi, %rax        # x非负时将x传送至%rax, 直接进行位移
  sarq   $3, %rax          # 算数右移3位
  ret
```

## 练习题3.21
```
long test(long x, long y){
   long val = 8 * x;
   if (y > 0){
      if (x - y >= 0)
         val = x & y;
      else
         val = y - x;
   }else if (y + 2 <= 0)
      val = x + y;
   return val
}
```

## 练习题3.22
用以下程序计算能表示的最大的n的值
```
#define DATATYPE int
int is_overflowed(DATATYPE x, DATATYPE y){
   if (!y) {
      return false;
   }

   DATATYPE xy = x * y;
   return xy / y != x;
}

int max_n(){
   DATATYPE i = 1, prod = 1;
   while (!is_overflowed(prod, i)){
      prod *= i;
      i++;
   }
   return i - 1;
}
```
1. 32位的`int`能表示的最大n为12
2. 64位的`long`能表示的最大n为20

## 联系题3.23
1. `y`存放在寄存器`%rcx`中，`n`存放在寄存器`%rdx`中，`x`存放在寄存器`%rax`中（用于返回）
2. 由于p在程序运行过程中所保存的地址始终没有变化，编译器将对通过`p`的间接引用转变为直接引用`x`，并将其与`x += y;`的语句用同一个`leaq`指令完成。
3. 汇编代码如下
```
  long dw_loop(long x)
  x initially in %rdi
dw_loop:
  movq   %rdi, %rax              # prepare return value
  movq   %rdi, %rcx              # prepare multiplation
  imulq  %rdi, %rcx              # compute y = x * x
  leaq   (%rdi, %rdi), %rdx      # compute n = 2 * x
.L2:
  leaq   1(%rcx, %rax), %rax     # compute x = x + y + 1
  subq   $1, %rdx                # decrement n
  testq  %rdx, %rdx              # test n
  jg     .L2                     # if >, goto .L2 then loop
  rep; ret                       # return
```

## 练习题3.24
补全的C代码
```
long loop_while(long a, long b)
{
   long result = 1;
   while(a < b){
      result *= (a + b);
      a++;
   }
   return result;
}
```

## 练习题3.25
补全的C代码
```
long loop_while2(long a, long b)
{
   long result = b;
   while(b > 0){
      result *= a;
      b -= a;
   }
   return result;
}
```

## 练习题3.26
1. jump to middle
2. 补全的C代码
```
long fun_a(unsigned long x){
   long val = 0;
   while(x > 0){
      val ^= x;
      x >>= 1;
   }
   return val & 1;
}
```
3. 该程序对输入的`x`所有位进行了异或操作，用于判断`x`位表示中1的数量是否为奇数，个数为偶数（包含0）则返回值为`0`，个数为奇数则返回值为`1`

## 练习题3.27
goto风格代码
```
long fact_for_guarded_do(long n)
{
   long i = 2;
   long result = 1;
   if (i > n){
      goto done;
   }
loop:
   result *= i;
   i++;
   if (i <= n){
      goto loop;
   }
done:
   return result;
}
```

## 练习题3.28
1. 补全的C代码
```
long fun_b(unsigned long x){
   long val = 0;
   long i;
   for (i = 64; i != 0; i--){
      int mask = x & 1;
      val += val;
      val |= mask;
      x >>= 1;
   }
   return val;
}
```
2. 循环变量`i`的初始值已经满足了循环条件，所以编译器优化了进入循环前的测试部分。
3. 该程序将输出`x`位模式顺序反转后的位模式

## 练习题3.29
1. 直接按照规则翻译将得到如下代码
```
long sum = 0;
long i = 0;
while(i < 10){
   if (i & 1)
      continue;
   sum += 1;
   i++;
}
```
此时一旦循环内条件分支满足，`i`的自增将会一并被跳过，从而导致死循环

2. 代码可以修改如下
```
long sum = 0;
long i = 0;
while(i < 10){
   if (i & 1)
      goto update;
   sum += 1;
update:
   i++;
}
```

## 练习题3.30
1. 标号值有`-1, 0, 1, 2, 4, 5, 7`
2. `case 0`与`case 2`有多个标号

## 练习题3.31
补全的C代码
```
void switcher(long a, long b, long c, long *dest)
{
   long val;
   switch(a){
      case 5:
         c = b ^ 0xF;
      case 0:
         val = 112 + c;
         break;
      case 2:
      case 7:
         val = (b + c) << 2;
         break;
      case 4:
         val = a;
         break;
      default:
         val = b;
   }
   *dest = val;
}
```

## 练习题3.32
| 标号 | PC       | 指令                  | %rdi | %rsi | %rax | %rsp           | *%rsp    | 描述                 |
| ---- | -------- | --------------------- | ---- | ---- | ---- | -------------- | -------- | -------------------- |
| M1   | 0x400560 | callq                 | 10   | -    | -    | 0x7fffffffe820 | -        | 调用first(10)        |
| F1   | 0x400548 | lea 0x1(%rdi), %rsi   | 10   | -    | -    | 0x7fffffffe818 | 0x400565 | 进入first(10)        |
| F2   | 0x40054c | sub &0x1, %rdi        | 10   | 11   | -    | 0x7fffffffe818 | 0x400565 | 执行first(10)语句    |
| F3   | 0x400550 | callq 400540 \<last\> | 10   | 11   | -    | 0x7fffffffe818 | 0x400565 | 调用last(10, 11)     |
| L1   | 0x400540 | mov %rdi, %rax        | 10   | 11   | -    | 0x7fffffffe810 | 0x400555 | 进入last(10, 11)     |
| L2   | 0x400543 | imul %rsi, %rax       | 10   | 11   | 10   | 0x7fffffffe810 | 0x400555 | 执行last(10, 11)语句 |
| L3   | 0x400547 | ret                   | 10   | 11   | 110  | 0x7fffffffe810 | 0x400555 | 返回first(10)        |
| F4   | 0x400555 | repz retq             | 10   | 11   | 110  | 0x7fffffffe818 | 0x400565 | 返回main             |
| M2   | 0x400565 | mov %rax, %rdx        | 10   | 10   | 110  | 0x7fffffffe820 | -        | 继续执行main语句     |

## 练习题3.33
1. `size_t procprob(int a, short b, long *u, char *v);`
2. `size_t procprob(int b, short a, long *v, char *u);`

## 练习题3.34
1. `x`~`x + 5`
2. `x + 6`, `x + 7`
3. 总共要保存8个局部变量，但是被调用者保存寄存器只有6个，无法全部保存

## 练习题3.35
1. 递归的传入参数`x`
2. 补全的C代码
```
long rfun(unsigned long x){
   if (x == 0)
      return 0;
   unsigned long nx = x >> 2;
   long rv = rfun(nx);
   return x + rv;
}
```

## 练习题3.36
| 数组 | 元素大小 | 整个数组大小 | 起始地址 | 元素i      |
| ---- | -------- | ------------ | -------- | ---------- |
| S    | 2        | 14           | $x_S$    | $x_S + 2i$ |
| T    | 8        | 24           | $x_T$    | $x_T + 8i$ |
| U    | 8        | 48           | $x_U$    | $x_U + 8i$ |
| V    | 4        | 32           | $x_V$    | $x_V + 4i$ |
| W    | 8        | 4            | $x_W$    | $x_W + 8i$ |

## 练习题3.37
| 表达式       | 类型     | 值                | 汇编代码                        |
| ------------ | -------- | ----------------- | ------------------------------- |
| `S+1`        | `short*` | $x_S + 2$         | `leaq 2(%rdx), %rax`            |
| `S[3]`       | `short`  | $M[x_S + 6]$      | `movw 6(%rdx), %ax`             |
| `&S[i]`      | `short*` | $x_S + 2i$        | `leaq (%rdx, %rcx, 2), %rax`    |
| `S[4*i + 1]` | `short`  | $M[x_S + 8i + 2]$ | `movw 2(%rdx, %rcx, 8), %ax`    |
| `S + i-5`    | `short*` | $x_S + 2i - 10$   | `leaq -10(%rdx, %rcx, 2), %rax` |

## 练习题3.38
$M = 5, N = 7$

## 练习题3.39
该程序中操作的数组是一个$16 * 16$的定长数组，按照3.1的计算公式  
$\&A[i][0] = x_A + 4(16i+ 0) = x_A + 64i$  
$\&B[0][k] = x_A + 4(0 + k) = x_A + 4k$
$\&B[N][k] = x_A + 4(16*16 + k) = x_A + 1024 + 4k$  
分别对应汇编代码3～5行的计算结果

## 练习题3.40
优化后的C代码如下
```
#define N 16
typedef int fix_matrix[N][N];

void fix_set_diag_opt(fix_matrix A, int val)
{
   long i = 0;
   int *Aptr = &A[0][0];
   for (i = 0; i < N; ++i){
      *Aptr = val;
      Aptr += N + 1;
   }
}
```

## 练习题3.41
1. `p`: 0, `s.x`: 8, `x.y`: 12, `next`: 16
2. 总共需要24个字节
3. 补全的C代码如下
```
void sp_init(struct prob *sp){
   sp->s.x = sp->s.y;
   sp->p = &s.x;
   sp->next = sp;
}
```

## 练习题3.42
1. C代码如下
```
long fun(struct ELE *ptr)
{
   long sum = 0;
   while(ptr)
   {
      sum += ptr->v;
      ptr = ptr->p;
   }
   return sum;
}
```
2. 这个结构实现了一个单链表，fun用于计算给定结点之后的部分（包含给定结点）所有节点的v的和

## 练习题3.43
| $expr$               | $type$  | 代码                         |
| -------------------- | ------- | ---------------------------- |
| `up->t1.u`           | `long`  | `movq (%rdi), %rax`          |
|                      |         | `movq %rax, (%rsi)`          |
| `up->t1.v`           | `short` | `movw 8(%rdi), %ax`          |
|                      |         | `movw %ax, (%rsi)`           |
| `&up->t1.w`          | `char*` | `leaq 12(%rdx), %rax`        |
|                      |         | `movq %rax, (%rsi)`          |
| `up->t2.a`           | `int*`  | `movq %rdi, (%rsi)`          |
| `up->t2.a[up->t1.u]` | `int`   | `movl (%rdi), %eax`          |
|                      |         | `movl (%rdi, %rax, 4), %edx` |
|                      |         | `movl %edx, (%rsi)`          |
| `*up->t2.p`          | `char`  | `movq 8(%rdi), %rax`         |
|                      |         | `movb (%rax), %dl`           |
|                      |         | `movb %dl, (%rsi)`           |

## 练习题4.44
1. 偏移量分别为0, 4, 8, 12。结构总大小为16，对其要求为4的倍数
2. 偏移量分别为0, 4, 5, 8。结构总大小为16，对其要求为8的倍数
3. 偏移量分别为0, 6。结构总大小为10，对其要求为2的倍数。
4. 偏移量分别为0, 16。结构总大小为40，对其要求为8的倍数
5. 偏移量分别为0, 24。结构总大小为40，对其要求为8的倍数

## 练习题3.45
1. 偏移量分别为0， 8， 16， 24， 28， 32， 40， 48。整个结构的对其要求为8的倍数
2. 总的大小为56字节
3. 重新排列后的结构如下
```
struct {
   char    *a;
   double   c;
   long     g;
   float    e;
   int      h;
   short    b;
   char     d;
   char     f;
} rec;
```
重排后的偏移量分别为0, 8, 16, 24, 28, 32, 34, 35。总大小为40

## 练习题3.46
1. 如下 
| 栈底                    | 说明             |
| ----------------------- | ---------------- |
| 00 00 00 00 00 40 00 76 | 返回地址         |
| EF CD AB 89 67 45 23 01 | 保存%rbx         |
| ?? ?? ?? ?? ?? ?? ?? ?? | 栈分配的缓存空间 |
| ?? ?? ?? ?? ?? ?? ?? ?? | 栈分配的缓存空间 |
2. 调用gets后栈的状态如下
| 栈底                    | 说明             |
| ----------------------- | ---------------- |
| 00 00 00 00 00 40 00 34 | 返回地址         |
| 33 32 31 30 39 38 37 36 | 保存%rbx         |
| 35 34 33 32 31 30 39 38 | 栈分配的缓存空间 |
| 37 36 35 34 33 32 31 30 | 栈分配的缓存空间 |
3. 程序将会试图返回到`0x400034`
4. `%rbx`保存的值被破坏
5. 为`result`分配的空间没有包含`\0`的长度，在复制的字符串时也将再次越界。在尝试用`malloc`分配堆空间后没有对分配结果进行判断就直接拷贝字符串。

## 练习题3.47
1. 约为$2^{13}$字节
2. 约$2^6$次

## 练习题3.48
1. 对于不带保护者的版本，缓冲区`buf`为调用`iptoa`之前栈顶的前24个字节，`v`保存在在栈顶开始的第24字节；对于带保护者的版本，缓冲区`buf`为调用`iptoa`之前栈顶的第16～39字节，`v`保存在第8字节，金丝雀值保存在第40字节
2. `v`比`buf`更靠近栈顶，即使缓冲区溢出也能避免`v`被破坏。

## 练习题3.49
1. $s_2 - s_1 = (22 + 8n) \& (-16)= 22 + 8n - (22 + 8n)mod16 = 22 + 8n - (6 + 8n mod16)mod16$当n为奇数时，分配的栈空间为$8+8n$，当n为偶数时分配的空间为$16+8n$
2. $\%r8 = (s_2 + 7)/_w^u 8 *_w^u = (s_2 + 7) - (s_2 + 7)mod8$该计算能找到离栈顶最近的地址为8的整数倍的地址。
3. 结果如下
| $n$ | $s_1$  | $s_2$  | $p$    | $e_1$ | $e_2$ |
| --- | ------ | ------ | ------ | ----- | ----- |
| 5   | 0x2065 | 0x2035 | 0x2038 | 3     | 5     |
| 6   | 0x2064 | 0x2024 | 0x2028 | 4     | 12    |
4. 保证`p`的8字节对齐。保证为数组分配的整个栈空间是16字节的整数倍

## 练习题3.50
`val1: d, val2: i, val3: l, val4: f`

## 练习题3.51
| $T_x$    | $T_y$    | 指令                            |
| -------- | -------- | ------------------------------- |
| `long`   | `double` | `vcvtsi2sdq %rdi, %xmm0`        |
| `double` | `int`    | `vcvttsd2si %xmm0, %eax`        |
| `double` | `float`  | `vmovddup %xmm0, %xmm0`         |
|          |          | `vcvtpd2psx %xmm0, %xmm0`       |
| `long`   | `float`  | `vcvtsi2ssq %rdi, %xmm0, %xmm0` |
| `float`  | `long`   | `vcvttss2siq %xmm0, %rdi`       |

## 练习题3.52
1. `a`在`%xmm0`，`b`在`%rdi`，`c`在`%xmm1`，`d`在`%esi`
2. `a`在`%edi`，`b`在`%rsi`，`c`在`%rdx`，`d`在`%rcx`
3. `a`在`%rdi`，`b`在`%xmm0`，`c`在`%esi`，`d`在`%xmm1`
4. `a`在`%xmm0`，`b`在`%rdi`，`c`在`%xmm1`，`d`在`%xmm2`

## 练习题3.53
1. `p: int, q: long, r: float, s: double`
2. `p: int, q: float, r: long, s: double`

## 练习题3.54
C语言版本如下
```
double funct2(double w, int x, float y, long z)
{
   return (double) x * y - w / z;
}
```

## 练习题3.55
此处的保存的16进制值为`0x404000000000`，其中  
阶码部分二进制值为`0x404`，偏移处理后阶码值为`5`；  
小数部位为`0`，加1得到尾数部分`1.0`  
因此该双精度浮点数表示值为`1.0 * 2^5 = 32.0`

## 练习题3.56
1. `abs(double x);`
2. `x = 0;`
3. `-x`

## 练习题3.57
C版本代码如下
```
double funct3(int *ap, double b, long c, float *dp){
   int a = *ap;
   float d = *dp;
   if(a <= f){
      return c + 2 * d;
   } else{
      return (double) c * d;
   }
}
```