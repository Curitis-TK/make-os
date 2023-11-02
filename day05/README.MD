# Day 05

## 结构体 struct 

### 声明结构体类型

```c
struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};
```

### 使用结构体

```c
struct BOOTINFO obj;

obj.scrnx = 320;
obj.scrny = 200;
obj.vram  = 0xa0000;
```

借助指针将数据带入结构体

```c
binfo = (struct BOOTINFO *)0x0ff0;
```

### 读取结构体内容

点运算符（.）用于结构体变量访问成员。

箭头运算符（->）用于结构体指针变量访问成员。

#### 点运算符

```c
a = obj.scrnx;
b = (*binfo).scrnx;
```

#### 箭头运算符

```
c = binfo->scrnx
```

## stdio.h

标准头文件

## sprintf

它是printf函数的同类，与printf函数的功能很相近。

这个sprintf函数，是本次使用的名为GO的C编译器附带的函数。

printf则需要操作系统实现。

