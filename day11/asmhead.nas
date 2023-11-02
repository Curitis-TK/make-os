; haribote-os boot asm
; TAB=4

BOTPAK	EQU		0x280000		; bootpack的加载地址
DSKCAC	EQU		0x100000		; 磁盘缓存的位置
DSKCAC0	EQU		0x8000			; 磁盘缓存的位置(真实模式)

; BOOT_INFO 变量位置
CYLS	EQU		0x0ff0			; 引导扇区设置
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 关于颜色数量的信息。多少比特的颜色?
SCRNX	EQU		0x0ff4			; 分辨率X
SCRNY	EQU		0x0ff6			; 分辨率Y
VRAM	EQU		0x0ff8			; 图形缓冲的起始地址

; 程序内存地址
ORG		0xc200

; 画面设定

		MOV		AL,0x13			; VGA图形，320x200 8bit色彩
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 画面モードをメモする（C言語が参照する）
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 让BIOS告知键盘的LED状态

		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL


; PIC关闭一切中断
;   根据AT兼容机的规格，如果要初始化PIC，
;   必须在CLI之前进行，否则有时会挂起。
;   随后进行PIC的初始化。

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 让CPU休息一个时钟长的时间; 如果连续执行OUT指令，有些机种会无法正常运行
		OUT		0xa1,AL

		CLI						; 禁止CPU级别的中断
; 同等与C程序中的:
; io_out(PIC0_IMR, 0xff); 	/* 禁止主PIC的全部中断 */
; io_out(PIC1_IMR, 0xff); 	/* 禁止从PIC的全部中断 */
; io_cli(); 				/* 禁止CPU级别的中断*/


; 为了让CPU能够访问1MB以上的内存空间，设定A20GATE
		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout
; 同等于C程序中的:
; #define KEYCMD_WRITE_OUTPORT    0xd1
; #define KBC_OUTPORT_A20G_ENABLE 0xdf
;     /* A20GATE的设定 */
;     wait_KBC_sendready();
;     io_out8(PORT_KEYCMD, KEYCMD_WRITE_OUTPORT);
;     wait_KBC_sendready();
;     io_out8(PORT_KEYDAT, KBC_OUTPORT_A20G_ENABLE);
;     wait_KBC_sendready(); /* 这句话是为了等待完成执行指令 *
; 和键盘控制程序十分相似




; 切换到保护模式

[INSTRSET "i486p"]				; “想要使用486指令”的叙述，之后就可以使用32位功能了

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 设第31位为0（禁止分页）
		OR		EAX,0x00000001	; 设第0位为1（切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush
pipelineflush:
		MOV		AX,1*8			;  可读写的段 32位
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的转送

		MOV		ESI,bootpack	; 转送源地址
		MOV		EDI,BOTPAK		; 转送目的地址
		MOV		ECX,512*1024/4	; 转送数据的大小 512KB
		CALL	memcpy			; 调用内存复制

; 磁盘数据最终转送到它本来的位置去

; 首先从启动扇区IPL开始

		MOV		ESI,0x7c00		; 转送源地址
		MOV		EDI,DSKCAC		; 转送目的地址
		MOV		ECX,512/4		; 转送数据的大小 512字节
		CALL	memcpy			; 调用内存复制


; 所有剩下的

		MOV		ESI,DSKCAC0+512	; 转送源地址
		MOV		EDI,DSKCAC+512	; 转送目的地址
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数变换为字节数/4  512字节*18个柱面*2磁头
		SUB		ECX,512/4		; 减去 IPL	512字节
		CALL	memcpy

; asmheadでしなければいけないことは全部し終わったので、
;	あとはbootpackに任せる

; bootpackの起動

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 転送するべきものがない
		MOV		ESI,[EBX+20]	; 転送元
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 転送先
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; スタック初期値
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; ANDの結果が0でなければwaitkbdoutへ
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 引き算した結果が0でなければmemcpyへ
		RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

		ALIGNB	16
GDT0:
		RESB	8				; ヌルセレクタ
		DW		0xffff,0x0000,0x9200,0x00cf	; 読み書き可能セグメント32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 実行可能セグメント32bit（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
