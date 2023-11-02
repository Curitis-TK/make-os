; haribote-ipl
; TAB=4

CYLS    EQU     10      ; 声明常量CYLS为10

ORG		0x7c00          ; 指定程序在内存中的起始位置

JMP entry

; 标准FAT12格式软盘专用的代码
DB  0x90
DB  "HARIBOTE"      ; 启动扇区名称（8字节）
DW  512             ; 每个扇区（sector）大小（必须512字节）
DB  1               ; 簇（cluster）大小（必须为1个扇区）
DW  1               ; FAT起始位置（一般为第一个扇区）
DB  2               ; FAT个数（必须为2）
DW  244             ; 根目录大小（一般为224项）
DW  2880            ; 该磁盘大小（必须为2880扇区，计算方式：1440*1024/512）
DB  0x0f            ; 磁盘类型（必须为0xf0）
DW  9               ; FAT的长度（必须是9个扇区）
DW  18              ; 一个磁道（track）有几个扇区（必须为18）
DW  2               ; 磁头数（必须为2）
DD  0               ; 不使用分区，必须是0
DD  2880            ; 重写一次磁盘大小
DB  0,0,0x29        ; 意义不明（但是不能更改）
DD  0xffffffff      ; 卷标号码
DB  "HARIBOTE-OS"   ; 磁盘的名称（必须为11字节）
DB  "FAT12   "      ; 磁盘格式名称（必须为3字节）
RESB    18          ; 空出18字节

; 程序本体
;; 入口
entry:
    ; 初始化寄存器
    MOV AX,0        ; 初始化累加寄存器
    MOV SS,AX       ; 初始化栈段寄存器
    MOV SP,0x7c00   ; 初始化栈指针寄存器
    MOV DS,AX       ; 初始化数据段寄存器

    ; 配置基础磁盘读取参数
    MOV		AX,0x0820   ; 累加寄存器：赋值记录内存地址
    MOV		ES,AX       ; 附加段寄存器赋值：缓冲区大概地址
    MOV     CH,0        ; 计数寄存器高位：柱面0（一共80个柱面：0~79）
    MOV     DH,0        ; 数据寄存器高位：磁头0（一共2个磁头：0, 1）
    MOV     CL,2        ; 计数寄存器低位：扇区2（每个柱面只有18扇区：1~18）

;; 读取循环
readloop:
    MOV SI,0            ; 原变址寄存器：记录失败次数

;; 尝试读取
retry:
    MOV     AH,0x02     ; 累加寄存器高位：读入磁盘操作
    MOV     AL,1        ; 累加寄存器低位：读取1扇区，一个扇区512字节
    MOV     BX,0        ; 基址寄存器：缓冲区详细地址，地址的计算方式是ES*16+BX，例初始化时是：0x0820*16+0，读入的一个扇区数据会写入到内存的0x08200~0x083ff位置（512字节）
    ;;; BX注释：仅使用BX寄存器只能代表64KB大小范围的内存地址，采用这种方式可以扩大到1MB（遥遥领先）
    MOV     DL,0x00     ; 数据寄存器低位：盘符A
    INT     0x13        ; 调用中断：BIOS磁盘功能
    JNC     next        ; 没有发生错误，FLAGS.CF==0，跳转到下一扇区位置
    ADD     SI,1        ; 失败次数+1
    CMP     SI,5        ; 失败次数比对
    JAE     error       ; 失败次数==5，跳转到失败位置
    MOV     AH,0x00     ; 清空累加寄存器高位
    MOV     DL,0x00     ; 清空数据寄存器低位
    INT     0x13        ; 重置驱动器
    JMP     retry       ; 完成重置，跳转回尝试读取

;; 下一扇区
next:
    ; 准备下一个扇区
    MOV     AX,ES       ; 将缓冲区大概位置赋值给累加寄存器，借助累加寄存器的功能进行增加计算
    ADD     AX,0x0020   ; 缓冲区大概位置增加32，32*16=512，也就是增加512字节
    MOV     ES,AX       ; 计算完成，赋值回去
    ADD     CL,1        ; 读取扇区位置+1
    CMP     CL,18       ; 扇区位置比对
    JBE     readloop    ; 小于等于18，继续读取扇区
    ; 准备下一个柱面
    MOV     CL,1        ; 设置读取扇区位置为1
    ADD     DH,1        ; 设置使用磁头号+1
    CMP     DH,2        ; 对比磁头号
    JB      readloop    ; 磁头号小于2，跳转到读取循环位置
    MOV     DH,0        ; 否则，设置磁头号为0
    ADD     CH,1        ; 读取柱面位置+1
    CMP     CH,CYLS     ; 判断柱面
    JB      readloop    ; 柱面小于常量CYLS，跳转到读取循环位置

;; haribote.sys读取完毕，开始执行
    MOV [0x0ff0],CH     ; 记录IPL的读取位置
    JMP 0xc200          ; 跳转到haribote在内存中的位置


;; 错误处理，打印错误文字
error:
    MOV     SI,msg

putloop:
    MOV     AL,[SI]
    ADD     SI,1
    CMP     AL,0
    JE      fin
    MOV     AH,0x0e
    MOV     BX,15
    INT     0x10
    JMP     putloop

fin:
    HLT
    JMP fin

msg:
    DB  0x0a,0x0a
    DB  "load error"
    DB  0x0a
    DB  0


;; IPL程序收尾
RESB    0x7dfe-$

DB      0x55, 0xaa