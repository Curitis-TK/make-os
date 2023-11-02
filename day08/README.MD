# Day 08

## 解析 asmhead.nas

### 变量/常量声明

```assembly
BOTPAK	EQU		0x280000		; bootpack的加载地址
DSKCAC	EQU		0x100000		; 磁盘缓存的位置(受保护的虚拟内存地址模式)
DSKCAC0	EQU		0x8000			; 磁盘缓存的位置(真实模式：实际地址模式)

CYLS	EQU		0x0ff0			; 引导扇区设置；后续时候BOOTINFO结构体时通过ADR_BOOTINFO指定到这里进行声明
LEDS	EQU		0x0ff1			; 键盘LED
VMODE	EQU		0x0ff2			; 关于颜色数量的信息
SCRNX	EQU		0x0ff4			; 分辨率X
SCRNY	EQU		0x0ff6			; 分辨率Y
VRAM	EQU		0x0ff8			; 图形缓冲的起始地址

ORG		0xc200					; 程序装载位置
```

### 设置显卡模式

```assembly
MOV		AL,0x13					; 模式: VGA 图形模式，320 × 200 × 8位彩色模式，调色板模式
MOV		AH,0x00
INT		0x10					; 调用BIOS
MOV		BYTE [VMODE],8			; 显示模式: 8位彩色模式
MOV		WORD [SCRNX],320		; 分辨率X: 320
MOV		WORD [SCRNY],200		; 分辨率Y: 200
MOV		DWORD [VRAM],0x000a0000	; 显卡内存位置
```

#### 可选的模式（部分）

| 值   | 描述                                                         |
| ---- | ------------------------------------------------------------ |
| 0x03 | 16色字符模式，80 × 25 ，独特的4面存储模式                    |
| 0x12 | VGA 图形模式，640 × 480 × 4位彩色模式                        |
| 0x13 | VGA 图形模式，320 × 200 × 8位彩色模式，调色板模式            |
| 0x6a | 扩展VGA 图形模式，800 × 600 × 4位彩色模式，独特的4面存储模式（有的显卡不支持这个模式） |

### 从BIOS获取键盘的LED状态

```assembly
MOV		AH,0x02
INT		0x16		; 调用BIOS
MOV		[LEDS],AL	; AL为获取到的键盘LED状态
```

### 停用 PIC 的中断功能

根据AT兼容机的规格，如果要初始化PIC，必须在CLI之前进行，否则有时会挂起。

当CPU进行模式转换时也不能出现中断。

在此先停用中断功能，随后在bootpack中再进行初始化PIC，一举两得。

```assembly
MOV     AL,0xff
OUT     0x21,AL			; 给0x21端口发送数据: 0xff
NOP                     ; 如果连续执行OUT指令，有些机种会无法正常运行；什么都不做，让CPU休息一个时钟长的时间
OUT     0xa1,AL			; 给0xa1端口发送数据: 0xff

CLI                     ; 禁止CPU级别的中断
```

以上代码在C中呈现是

```c
io_out(PIC0_IMR, 0xff); // 禁止主PIC的全部中断
io_out(PIC1_IMR, 0xff); // 禁止从PIC的全部中断
io_cli(); 				// 禁止CPU级别的中断
```

### 允许CPU访问1MB以上的内存空间

设定A20GATE

```assembly
CALL    waitkbdout
MOV     AL,0xd1
OUT     0x64,AL
CALL    waitkbdout
MOV     AL,0xdf         ; 激活 A20
OUT     0x60,AL
CALL    waitkbdout

;...

waitkbdout:
    IN		AL,0x64
    AND		AL,0x02
    IN      AL,0x60        	; 空读（为了清空数据接收缓冲区中的垃圾数据）
    JNZ		waitkbdout		; 如果AND的结果不是0，跳转到waitkbdout（循环）
    RET
```

以上代码在C中呈现是

```c
#define PORT_KEYCMD 0x64	// 命令端口
#define PORT_KEYDAT 0x60	// 数据端口

#define KEYCMD_WRITE_OUTPORT    0xd1	// 命令写入端口
#define KBC_OUTPORT_A20G_ENABLE 0xdf	// 命令数据

// A20GATE的设定
wait_KBC_sendready();	// 等待指令完成执行
io_out8(PORT_KEYCMD, KEYCMD_WRITE_OUTPORT);	// 告诉控制电路准备接收0x60数据端口
wait_KBC_sendready();
io_out8(PORT_KEYDAT, KBC_OUTPORT_A20G_ENABLE);	// 通过0x60端口发送A20G_ENABLE数据
wait_KBC_sendready();
```

### 切换到保护模式

> **CR0**
>
> 非常重要的寄存器，只有操作系统才能操作
>
> 包含了与操作系统内核和内存管理有关的配置信息

```assembly
[INSTRSET "i486p"]	; 声明使用486指令；之后就可以使用32位功能了

    LGDT	[GDTR0]			; 设定临时GDT
    MOV		EAX,CR0			; 读取CR0到EAX
    AND		EAX,0x7fffffff	; 使用与运算，将第31位设置为0（禁止分页）
    ; EAX & 0111 1111 1111 1111 1111 1111 1111 1111
    OR		EAX,0x00000001	; 设第0位为1（切换到保护模式）
    ; EAX | 0000 0000 0000 0000 0000 0000 0000 0001
    MOV		CR0,EAX			; 设置完成后回写CR0
    JMP		pipelineflush

; 为了在 x86 架构上切换到保护模式，开启禁止分页等设置
; 将数据段寄存器 DS、ES、FS、GS 和栈段寄存器 SS 的值设置为AX的值
pipelineflush:
    MOV		AX,1*8			; 代表可读写的32位段
    MOV		DS,AX
    MOV		ES,AX
    MOV		FS,AX
    MOV		GS,AX
    MOV		SS,AX

;...

GDT0:
        RESB    8               			; NULL selector
        DW      0xffff,0x0000,0x9200,0x00cf ; 可以读写的段（segment）32bit
        DW      0xffff,0x0000,0x9a28,0x0047 ; 可以执行的段（segment）32bit（bootpack用）

        DW      0
    
GDTR0:
    DW		8*3-1
    DD		GDT0

	ALIGNB	16
```

#### ALIGNB

一直添加DB 0，直到时机合适的时候为止。

ALIGNB 16的情况下，地址能被16整除的时候，就称为“时机合适”。

如果最初的地址能被16整除，则ALIGNB指令不作任何处理。

#### GDT0

一种特定的GDT。0号是空区域（null sector），不能够在那里定义段。1号和2号分别由下式设定。

```c
set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);
set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
```





#### 为什么在这里使用JMP

变成保护模式后，机器语言的解释会发生变化。

在 x86 处理器中，流水线是一种执行指令的方式，多个指令可以同时在不同阶段执行。如果不进行流水线刷新，之前的指令可能还在流水线中，这可能导致不确定的行为或错误的执行结果。

通过使用 `JMP pipelineflush`，代码会跳转到 `pipelineflush` 标签，等待流水线中的指令执行完毕，然后再继续执行后续的指令。这确保了关键状态更改之前的指令不会对后续的执行产生影响。这种技术称为指令流水线的刷新或者停顿。

#### pipelineflush标签的作用

进入保护模式以后，段寄存器的意思也变了（不再是乘以16后再加算的意思了），除了CS以外所有段寄存器的值都从0x0000变成了0x0008（相当于“gdt + 1”的段）

CS由于特殊原因后面再处理

#### 为什么是0x8

相当于“gdt + 1”的段；

在从实模式切换到保护模式时，这些寄存器需要被重新配置以确保正确的内存访问权限和段选择。

在x86架构中，段寄存器（如DS、ES、FS、GS、SS）用于指示内存中的段，而不是线性地址。每个段都有一个与之相关的描述符，这个描述符定义了段的基地址、限制、特权级别和其他属性。

选择子（通常是16位值）用于选择段描述符。在保护模式下，段选择子的高13位用于索引全局描述符表（GDT）或本地描述符表（LDT），以获取描述符的详细信息。

`0x8` 是用于指示一个合法的数据段描述符在全局描述符表（GDT）或本地描述符表（LDT）中的偏移。

这个描述符应该具有合适的属性，如可读写数据段，特权级别等。因此，将这些寄存器设置为 `0x8` 实际上将它们配置为引用 GDT 或 LDT 中的一个特定数据段，以确保后续的内存访问操作使用了正确的段。

这通常是进入保护模式后的一项重要操作，以确保内存访问权限的正确性和数据的一致性。

### 转送

> **内容回顾**
>
> IPL位于0x7c00，磁盘的第一扇区（512B）
>
> IPL从软盘读取10个扇区的内容到0x8200~0x9600（0x8000~0x81ff是留给IPL的）
>
> 
>
> 磁盘中文件名名会写在0x2600以后的地方
>
> 磁盘中文件的内容会写在磁盘0x4200以后的地方
>
> 
>
> 所以，磁盘中读取到的程序在内存中位置就是0x8000+0x4200=0xc200

```assembly
; 为了方便阅读，将常量展示于此，在文件顶部已经声明过了
BOTPAK	EQU		0x280000		; bootpack的加载地址
DSKCAC	EQU		0x100000		; 磁盘缓存的位置(受保护的虚拟内存地址模式)
DSKCAC0	EQU		0x8000			; 磁盘缓存的位置(真实模式：实际地址模式)

; bootpack的转送
MOV     ESI,bootpack    ; 转送源
MOV     EDI,BOTPAK      ; 转送目的地
MOV     ECX,512*1024/4	; 转送数据的大小：512KB
CALL    memcpy

; IPL的转送（自动从软盘读取的IPL） 将始于0x7c00的磁盘内容，复制到0x100000
MOV     ESI,0x7c00      ; 转送源
MOV     EDI,DSKCAC      ; 转送目的地
MOV     ECX,512/4		; 转送数据的大小：512字节
CALL    memcpy

; 剩下的 将始于0x8200的磁盘内容，复制到0x100200
MOV		ESI,DSKCAC0+512	; 转送源地址（真实模式利用IPL从磁盘读取到内存的程序+IPL预留空间）
MOV		EDI,DSKCAC+512	; 转送目的地址
MOV		ECX,0
MOV		CL,BYTE [CYLS]
; IMUL 整数乘法
IMUL	ECX,512*18*2/4	; 从柱面数变换为字节数/4  512字节*18个柱面*2磁头	/ 双字节单位
; SUB 减法
SUB		ECX,512/4		; 减去IPL的大小（512字节）
CALL	memcpy

; asmhead最后一行，后续拼bootpack.hrb
bootpack:
```

#### memcpy

复制内存的函数

```c
memcpy(转送源地址, 转送目的地址, 转送数据的大小);
```

转送数据的大小是以double word（4字节）为单位，数据大小需要用字节数除以4来指定

```assembly
memcpy:
        MOV     EAX,[ESI]		; 获取源：从ESI指向的内存获取4字节
        ADD     ESI,4			; ESI地址+4
        MOV     [EDI],EAX		; 写入目标：将获取到的源写入EDI指向的内存
        ADD     EDI,4			; EDI地址+4
        SUB     ECX,1			; 待转移的数据大小-1
        JNZ     memcpy          ; 结果如果不是0，就跳转到memcpy继续转移
        RET
```

##### 例子

```c
memcpy(0x7c00, 0x100000, 512/4);
```

从0x7c00复制512字节到0x100000

### 启动bootpack（这里暂时无法理解...

```assembly
; 必须由asmhead来完成的工作，至此全部完毕
;   以后就交由bootpack来完成

; bootpack的启动

        MOV     EBX,BOTPAK
        MOV     ECX,[EBX+16]
        ADD     ECX,3           ; ECX += 3;
        SHR     ECX,2           ; ECX /= 4; 向右移位2位
        JZ      skip            ; 如果结果是0没有要转送的东西时
        MOV     ESI,[EBX+20]    ; 转送源
        ADD     ESI,EBX
        MOV     EDI,[EBX+12]    ; 转送目的地
        CALL    memcpy
skip:
        MOV     ESP,[EBX+12]    ; 栈初始值
        JMP     DWORD 2*8:0x1b
```

将bootpack.hrb第0x10c8字节开始的0x11a8字节复制到0x00310000号地址

#### SHR: 向右移位

`SHR ECX,2`等效与`ECX >>=2`

#### 特殊的JMP

将2 * 8 代入到CS里，同时移动到0x1b号地址。这里的0x1b号地址是指第2个段的0x1b号地址。第2个段的基地址是0x280000，所以实际上是从0x28001b开始执行的。这也就是bootpack.hrb的0x1b号地址

这样就开始执行bootpack.hrb了

### 内存分布图

| 位置                    | 描述                             | 大小   |
| ----------------------- | -------------------------------- | ------ |
| 0x00000000 - 0x000fffff | 在启动中会多次使用，但之后就变空 | 1MB    |
| 0x00100000 - 0x00267fff | 用于保存软盘的内容               | 1440KB |
| 0x00268000 - 0x0026f7ff | 闲置                             | 30KB   |
| 0x0026f800 - 0x0026ffff | IDT                              | 2KB    |
| 0x00270000 - 0x0027ffff | GDT                              | 64KB   |
| 0x00280000 - 0x002fffff | bootpack.hrb                     | 512KB  |
| 0x00300000 - 0x003fffff | 栈和其他                         | 1MB    |
| 0x00400000后            | 空                               |        |

### 结束

最初状态时，GDT在asmhead.nas里，并不在0x00270000 ~ 0x0027ffff的范围里。IDT连设定都没设定，所以仍处于中断禁止的状态。应当趁着硬件上积累过多数据而产生误动作之前，尽快开放中断，接收数据。

在bootpack.c的HariMain里，应该在进行调色板（palette）的初始化以及画面的准备之前，先赶紧重新创建GDT和IDT，初始化PIC，并执行“io_sti();”。