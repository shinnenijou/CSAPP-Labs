- [Chapter5 优化程序性能](#chapter5-优化程序性能)
  - [练习题5.1](#练习题51)
  - [练习题5.2](#练习题52)
  - [练习题5.3](#练习题53)
  - [练习题5.4](#练习题54)
  - [练习题5.5](#练习题55)
  - [练习题5.6](#练习题56)
  - [练习题5.7](#练习题57)
  - [练习题5.8](#练习题58)
  - [练习题5.9](#练习题59)
  - [练习题5.10](#练习题510)
  - [练习题5.11](#练习题511)
  - [练习题5.12](#练习题512)

# Chapter5 优化程序性能

## 练习题5.1
当调用过程`xp`等于`yp`时，会在`*yp = *xp - *yp`执行后使指向的数据变成0

## 练习题5.2
过程1: $n \le 2$时是三个版本中最快的  
过程2: $3\le n \le 7$时是三个版本中最快的
过程3: $n \ge 8$时是三个版本中最快的

## 练习题5.3
| 代码 | min | max | incr | square |
| ---- | --- | --- | ---- | ------ |
| A    | 1   | 91  | 90   | 90     |
| B    | 90  | 1   | 90   | 90     |
| C    | 1   | 1   | 90   | 90     |

## 练习题5.4
1. 寄存器`%xmm0`在O1版本中单纯作为乘法计算的容器使用。而O2中则用来保存累计变量，减少了每次循环中的一次内存读取。
2. O2优化版本仍忠实实现了combine3的代码。
3. 在O1优化的版本中，略去循环开支后的两次循环间的代码
```
...
vmovsd  (%rbx), %xmm0
vmulsd  (%rdx), %xmm0, %xmm0
vmovsd  %xmm0, (%rbx)
vmovsd  (%rbx), %xmm0
vmulsd  (%rdx), %xmm0, %xmm0
vmovsd  %xmm0, (%rbx)
...
```
可以发现中间的传送指令`vmovsd  (%rbx), %xmm0`
在传送前后%xmm0的值都没有发生任何变化，因此在O2的优化版本中省去了这一步指令，并且不会导致程序有任何违背预期的行为。

## 练习题5.5
1. 内循环进行了$2n$次乘法$2n$次加法，其中$n$次加法为循环开支。
2. 内循环部分的汇编代码
```
      20: f2 0f 10 5c c7 08     movsd   8(%rdi,%rax,8), %xmm3       // load a[i]
      26: f2 0f 59 da           mulsd   %xmm2, %xmm3                // a[i] * xpwr
      2a: f2 0f 58 c3           addsd   %xmm3, %xmm0                // update result
      2e: f2 0f 59 d1           mulsd   %xmm1, %xmm2                // update xpwr
      32: 48 ff c0              incq    %rax                        // self-increase i
      35: 48 39 c6              cmpq    %rax, %rsi 
      38: 75 e6                 jne     -26 <__Z4polyPddl+0x20>
```
寄存器更新数据流如图  
![dataflow](../res/img/cp5_practice_5.5.png)  
该数据流表明制约程序性能的关键路径为更新`%xmm2`的浮点数乘法，CPE与延迟界限一致。

## 练习题5.6
1. 内循环进行了$n$次乘法$2n$次加法，其中$n$次加法为循环开支。
2. 内循环部分的汇编代码
```
      20: f2 0f 59 c1               mulsd   %xmm1, %xmm0                // x * result
      24: f2 0f 58 44 f7 f0         addsd   -16(%rdi,%rsi,8), %xmm0     // then add up a[i]
      2a: 48 ff ce                  decq    %rsi
      2d: 48 83 fe 01               cmpq    $1, %rsi
      31: 7f ed                     jg      -19 <__Z4polyPddl+0x20>
```
寄存器更新数据流如图  
![dataflow](../res/img/cp5_practice_5.6.png)  
程序性能受到`%xmm0`的更新路径限制，依次经历了一次浮点数乘法与浮点数加法，CPE与延迟界限一致。
3. 尽管练习题5.5中的函数需要更多操作，但运算操作使用了不同的寄存器保存迭代时的数据流，`%xmm2`与`%xmm0`在迭代时不依赖对方指令执行结果，从而执行达到并行。而练习题5.6中的函数在迭代过程中乘法与加法互相依赖对方执行结果，只能串行执行导致性能降低。
## 练习题5.7
$5\times 5$循环展开如下
```
void combine(vec_ptr v, data_t *dest)
{
    long i;
    long length = vec_length(v);
    long limit = length - 4;
    data_t *data = get_vec_start(v);
    data_t acc = IDENT

    // Combine 5 elements at a time
    for (i = 0; i < limit; i+=5){
        acc = (((((acc OP data[i]) OP data[i + 1]) OP data[i + 2]) OP data[i + 3]) OP data[i + 4]);
    }
    
    // Finish any remaining elements
    for(; i < length; ++i){
        acc = acc OP data[i];
    }
    *dest = acc;
}
```
## 练习题5.8
1. $CPE=15/3$
2. $CPE=10/3$
3. $CPE=5/3$
4. $CPE=5/3$
5. $CPE=10/3$

## 练习题5.9
```
void merge(long src1[], long src2[], long dest[], long n){
    long i1 = 0, i2 = 0, id = 0;
    while(i1 < n && i2 < n){
        long v1 = src1[i1], v2 = src2[i2];
        int take1 = v1 < v2;
        i1 += take1;
        i2 += 1 - take1;
        dest[id++] = take1 ? v1 : v2;
    }
    while(i1 < n)
        dest[id++] = src1[i1++];
    while(i2 < n)
        dest[id++] = src2[i2++];
}
```

## 练习题5.10
1. 生成一个$1, 2, ..., 998, 999, 999$的数组
2. 生成一个$0, 0, ..., 0, 0, 0$的数组
3. 问题A的内存读取与存储没有产生数据相关。但问题B每次迭代的读取和上次迭代的存储产生的数据相关。
4. 性能应当与问题A相近或略慢。尽管此时内存读取和存储产生了数据相关，但读取时可以直接从存储缓冲区内读取数据。

## 练习题5.11
寄存器更新数据流如图
![dataflow](../res/img/cp5_practice_5.11.png)  
由于两次迭代间对同一内存位置进行读取和存储，内存读写产生了数据依赖，
内存的更新成为了关键路径，其路径上一共经历`load`，`add`，`s_data`三个指令，CPE预计在9.0左右

## 练习题5.12
重写的代码如下
```
void psum1(float a[], float p[], long n)
{
    float psum = a[0];
    p[0] = psum;
    for (lont i = 0; i < n; i++)
    {
        psum += a[i];
        p[i] = psum;
    }
}
```