; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; 制作目标文件的模式
[INSTRSET "i486p"]      ; 告诉NASK。程序是给486使用的
[BITS 32]               ; 制作32位模式用的机械语言

; 制作目标文件的信息
[FILE "naskfunc.nas"]   ; 源文件名信息

; 程序中包含的函数名
GLOBAL  _io_hlt, _io_cli, _io_sti, io_stihlt
GLOBAL  _io_in8,  _io_in16,  _io_in32
GLOBAL  _io_out8, _io_out16, _io_out32
GLOBAL  _io_load_eflags, _io_store_eflags


; 以下是实际的函数代码
[SECTION .text]         ; 目标文件中写了这些之后再写程序

; 待机
; void io_hlt(void);
_io_hlt:
    HLT
    RET

; 将许可标志全部置为0，禁止所有中断
; void io_cli(void);
_io_cli:
    CLI
    RET

; 将许可标志全部置为1，允许所有中断
; void io_sti(void);
_io_sti:
        STI
        RET

; 允许所有中断，并待机
; void io_stihlt(void);
_io_stihlt:
        STI
        HLT
        RET

; 通过指定端口获取8位数据
; int io_in8(int port);
_io_in8:
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,0
        IN      AL,DX
        RET

; 通过指定端口获取16位数据
; int io_in16(int port);
_io_in16:
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,0
        IN      AX,DX
        RET

; 通过指定端口获取32位数据
; int io_in32(int port);
_io_in32:
        MOV     EDX,[ESP+4]     ; port
        MOV     EAX,0
        IN      EAX,DX
        RET

; 向指定端口发送8位数据
; void io_out8(int port, int data);
_io_out8:   
        MOV     EDX,[ESP+4]     ; port
        MOV     AL,[ESP+8]      ; data
        OUT     DX,AL
        RET
        
; 向指定端口发送16位数据
; void io_out16(int port, int data);
_io_out16:   
        MOV     EDX,[ESP+4]     ; port
        MOV     AL,[ESP+8]      ; data
        OUT     DX,AX
        RET
        
; 向指定端口发送32位数据
; void io_out32(int port, int data);
_io_out32:   
        MOV     EDX,[ESP+4]     ; port
        MOV     AL,[ESP+8]      ; data
        OUT     DX,EAX
        RET

; 获取EFLAGS寄存器
; int io_load_eflags(void);
_io_load_eflags:
    PUSHFD      ; 指 PUSH EFLAGS
    POP     EAX
    RET

; 写入EFLAGS寄存器
; void io_store_eflags(int eflags);
_io_store_eflags:
MOV     EAX,[ESP+4]
PUSH    EAX
POPFD       ; 指 POP EFLAGS
RET

; 内存写入1字节
_write_mem8:    ; void write_mem8(int addr, int data);
    ;; ESP: 栈指针寄存器
    MOV     ECX,[ESP+4]     ; [ESP + 4]中存放的是地址，将其读入ECX（扩展计数寄存器：32位）
    MOV     AL,[ESP+8]      ; [ESP + 8]中存放的是数据，将其读入AL 
    MOV     [ECX],AL
    RET

