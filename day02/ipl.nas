; hello-os code
; 部分文本编辑器支持读取TAB间隔配置
; TAB=4

ORG 0x7c00  ; 指定程序在内存中的起始位置

JMP entry

; 标准FAT12格式软盘专用的代码
DB  0x90
DB  "HELLOIPL"      ; 启动扇区名称（8字节）
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
DB  "HELLO-OS   "   ; 磁盘的名称（必须为11字节）
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
    MOV ES,AX       ; 初始化附加段寄存器

    MOV SI,msg      ; 将msg的内存地址赋值到源变址寄存器

;; 循环体
putloop:
    MOV AL,[SI]     ; 将指定地址的放入累加寄存器低位
    ADD SI,1        ; 内存地址+1
    CMP AL,0        ; 比对 累加寄存器低位 和 0
    JE  fin         ; 如果结果相等
    MOV AH,0x0e     ; 显示一个文字
    MOV BX,15       ; 指定字符颜色
    INT 0x10        ; 调用中断：BIOS显卡功能
    JMP putloop     ; 返回循环顶部

;; 结束
fin:
		HLT						; 让CPU停止，等待指令
		JMP		fin             ; 无限循环


;; msg
msg:
    DB  0x0a,0x0a       ; 2个换行符
    DB  "Hello World"   ; 主要文字内容 ASCII字符集中，一个字符占用1字节
    DB  0x0a            ; 换行
    DB  0               ; 结束标记


; 启动区是512字节，根据IBM的规范，是0x7c00到0x7cff
; 下面填充0x00直到0x7dfe，预留2比特给结束标识
RESB  0x7dfe-$

DB 0x55, 0xaa   ; IPL启动区结束标识