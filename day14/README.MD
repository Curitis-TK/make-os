# Day14

## 使用高分辨率模式

```
; VBE的640x480x8bit色彩
MOV     BX,0x4101
MOV     AX,0x4f02
INT     0x10
MOV     BYTE [VMODE],8
MOV     WORD [SCRNX],640
MOV     WORD [SCRNY],480
MOV     DWORD [VRAM],0xe0000000
```

## VESA

在早期，由IBM规定了画面模式和相关规格，各显卡公司也依照规范来制作显卡。

后来，其他显卡公司的技术力已经超越了IBM，出现了不少IBM还未制定的规格，出现了各种画面模式和使用方法，这给程序设计造成了一定困扰；

于是这些显卡公司成立了VESA协会，制作了一个专用BIOS，称为VESA BIOS extension（VESA-BIOS扩展，简略为VBE），用于提供几乎通用的设定方法，利用它就可以在大部分机型调用显卡的高分辨率功能。

传统画面模式使用

```
AH=0;
AL=画面模式号码;
```

VBE画面模式使用

```
AX=0x4f02;
BX=画面模式号码;
```

VBE提供的画面模式号码如下

| 参数值 | 分辨率    | 颜色模式  |
| ------ | --------- | --------- |
| 0x101  | 640*480   | 8bit/彩色 |
| 0x103  | 800*600   | 8bit/彩色 |
| 0x105  | 1024*768  | 8bit/彩色 |
| 0x107  | 1280*1024 | 8bit/彩色 |

还有其他模式暂未列举。

> QEMU中无法使用`0x107`模式

使用这些画面模式时，需要加上`0x4000`再赋值给BX，否则无法正常使用。

## 判断VBE是否可用

使用高分辨率模式判断显卡是否支持VBE。

显卡能使用VBE的信息会写入在内存的ES:DI开始的512字节中；

```assembly
; 确认VBE是否存在
MOV     AX,0x9000
MOV     ES,AX
MOV     DI,0
MOV     AX,0x4f00
INT     0x10
CMP     AX,0x004f
JNE     scrn320
```

执行INT 10后，如果显卡支持VBE，AX会变成0x4f，否则只能使用传统画面模式（320*200）

## 判断VBE版本

在HariboteOS中，启用高分辨率需要判断显卡VBE版本是否高于2.0，否则无法使用高分辨模式。

```assembly
; 检查VBE的版本
MOV     AX,0x9000
MOV     ES,AX
MOV     DI,0
MOV     AX,[ES:DI+4]
CMP     AX,0x0200
JB      scrn320         ; if (AX < 0x0200) goto scrn320
```

如果`AX < 0x200`即代表版本不满足

## 判断画面模式是否可用

```assembly
VBEMODE EQU     0x105
; 取得画面模式信息
MOV     CX,VBEMODE
MOV     AX,0x4f01
INT     0x10
CMP     AX,0x004f
JNE     scrn320
```

如果AX是`0x004f`以外的值，即代表指定的画面模式无法使用。

取得的画面模式信息会写入在ES:DI开始的256字节中，由于ES、DI值未变化，会覆盖此前获取的VBE版本信息，但问题不大。

### 画面模式信息

此处仅列举较为重要的

| 类型  | 地址         | 描述                                     |
| ----- | ------------ | ---------------------------------------- |
| WORD  | [ES:DI+0x00] | 模式属性（第7位代表是否能加上0x4000）    |
| WORD  | [ES:DI+0x12] | 分辨率x                                  |
| WORD  | [ES:DI+0x14] | 分辨率y                                  |
| BYTE  | [ES:DI+0x19] | 颜色数（必须为8）                        |
| BYTE  | [ES:DI+0x1b] | 颜色指定的方法（必须为4；4是调色板模式） |
| DWORD | [ES:DI+0x28] | VRAM的地址                               |

画面模式信息可以直接获取了，无需再手动维护了。

### 判断

```assembly
CMP     BYTE [ES:DI+0x19],8
JNE     scrn320					; 如果颜色数不是8
CMP     BYTE [ES:DI+0x1b],4
JNE     scrn320					; 如果颜色指定的方法不是4
MOV     AX,[ES:DI+0x00]
AND     AX,0x0080				; 第七位是否能加上0x4000
JZ      scrn320
```

