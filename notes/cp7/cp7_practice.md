# Chapter7 链接

## 练习题7.1
| 符号  | .symtab条目？ | 符号类型 | 在哪个模块中定义 | 节    |
| ----- | ------------- | -------- | ---------------- | ----- |
| buf   | yes           | EXTERNAL   | m.o              | .data |
| bufp0 | yes           | GLOABL   | swap.o           | .data |
| bufp1 | yes           | GLOBAL   | swap.o           | .bss  |
| swap  | yes           | FUNC     | swap.o           | .text |
| temp  | no            | -        | -                | -     |

## 练习题7.2
1. (a) REF(main.1) -> DEF(main.1)
   (b) REF(main.2) -> DEF(main.1)
2. (a) REF(main.1) -> DEF(错误)
   (b) REF(main.2) -> DEF(错误)
3. (a) REF(x.1) -> DEF(x.2)
   (b) REF(x.2) -> DEF(x.2)

## 练习题7.3
1. `gcc -static -o p p.o libx.a`
2. `gcc -static -o p p.o libx.a liby.a`
3. `gcc -static -o p p.o libx.a liby.a libx.a`

## 练习题7.4
1. `0x4004df`
2. `0x5`

## 练习题7.5
`*refptr = ADDR(swap) + r.addend - ADDR(.text) - r.offset = 0xa`