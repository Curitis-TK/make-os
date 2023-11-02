; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; 制作目标文件的模式
[INSTRSET "i486p"]      ; 告诉NASK。程序是给486使用的
[BITS 32]               ; 制作32位模式用的机械语言

; 制作目标文件的信息
[FILE "naskfunc.nas"]   ; 源文件名信息

; 程序中包含的函数名
GLOBAL  _io_hlt, _io_cli, _io_sti, _io_stihlt
GLOBAL  _io_in8,  _io_in16,  _io_in32
GLOBAL  _io_out8, _io_out16, _io_out32
GLOBAL  _io_load_eflags, _io_store_eflags
GLOBAL _load_gdtr, _load_idtr
GLOBAL	_asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
GLOBAL _load_cr0, _store_cr0
GLOBAL _memtest_sub

EXTERN	_inthandler21, _inthandler27, _inthandler2c

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

; 
; void load_gdtr(int limit, int addr);
_load_gdtr:
        ; 由于GDTR是48位寄存器，给它赋值的时候，唯一的方法就是指定一个内存地址，从指定的地址读取6个字节，再使用LGDT命令完成赋值
        ; 假设limit为0xffff，addr是0x270000
        ; ESP+4开始的内容应该是 [FF FF 00 00 / 00 00 27 00 ]（要注意低位放在内存地址小的字节里2）（使用/区分2个入参）
        ; 使用AX取出0xffff [FF FF]
        MOV     AX,[ESP+4]
        ; 从ESP+6偏移开始写入
        MOV     [ESP+6],AX
        ; 此时的ESP变为 [FF FF FF FF / 00 00 27 00 ]
        ; 从ESP+6偏移开始读入6字节到GDTR寄存器
        LGDT    [ESP+6]
        RET

; IDTR与GDTR结构体基本上是一样的，程序也非常相似。
; void load_idtr(int limit, int addr);
_load_idtr:
        MOV     AX,[ESP+4]
        MOV     [ESP+6],AX
        LIDT    [ESP+6]
        RET

_asm_inthandler21:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV		EAX,ESP
        PUSH	EAX
        MOV		AX,SS
        MOV		DS,AX
        MOV		ES,AX
        CALL	_inthandler21
        POP		EAX
        POPAD
        POP		DS
        POP		ES
        IRETD

_asm_inthandler27:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV		EAX,ESP
        PUSH	EAX
        MOV		AX,SS
        MOV		DS,AX
        MOV		ES,AX
        CALL	_inthandler27
        POP		EAX
        POPAD
        POP		DS
        POP		ES
        IRETD

_asm_inthandler2c:
        PUSH	ES
        PUSH	DS
        PUSHAD
        MOV		EAX,ESP
        PUSH	EAX
        MOV		AX,SS
        MOV		DS,AX
        MOV		ES,AX
        CALL	_inthandler2c
        POP		EAX
        POPAD
        POP		DS
        POP		ES
        IRETD

; 读取CR0寄存器
; int load_cr0(void)
_load_cr0:
        MOV     EAX,CR0
        RET

; 写入CR0寄存器
; int store_cr0(int cr0)
_store_cr0:
        MOV     EAX,[ESP+4]
        MOV     CR0,EAX

; 内存检查子函数，使用C编写会被编译器优化流程，故采用汇编完成
; unsigned int memtest_sub(unsigned int start, unsigned int end)
_memtest_sub:
        PUSH    EDI                     ;  （由于还要使用EBX, ESI, EDI）
        PUSH    ESI
        PUSH    EBX
        MOV     ESI,0xaa55aa55          ; pat0 = 0xaa55aa55;
        MOV     EDI,0x55aa55aa          ; pat1 = 0x55aa55aa;
        MOV     EAX,[ESP+12+4]          ; i = start;
mts_loop:
        MOV     EBX,EAX
        ADD     EBX,0xffc               ; p = i + 0xffc;
        MOV     EDX,[EBX]               ; old = *p;
        MOV     [EBX],ESI               ; *p = pat0;
        XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff;
        CMP     EDI,[EBX]               ; if (*p != pat1) goto fin;
        JNE     mts_fin
        XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff;
        CMP     ESI,[EBX]               ; if (*p != pat0) goto fin;
        JNE     mts_fin
        MOV     [EBX],EDX               ; *p = old;
        ADD     EAX,0x1000              ; i += 0x1000;
        CMP     EAX,[ESP+12+8]          ; if (i <= end) goto mts_loop;

        JBE     mts_loop
        POP     EBX
        POP     ESI
        POP     EDI
        RET
mts_fin:
        MOV     [EBX],EDX               ; *p = old;
        POP     EBX
        POP     ESI
        POP     EDI
        RET