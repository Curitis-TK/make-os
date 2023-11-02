#include "bootpack.h"

// 键盘控制电路未就绪
#define KEYSTA_SEND_NOTREADY    0x02
// 命令写入模式
#define KEYCMD_WRITE_MODE       0x60
// 键盘模式
#define KBC_MODE                0x47

// 等待键盘控制电路准备完毕
void wait_KBC_sendready(){
    for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

// 初始化键盘
void init_keyboard(){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

struct FIFO8 keyfifo;

//PS/2键盘中断
void inthandler21(int *esp){
    // 通知PIC: IRQ-01已经受理完毕
    io_out8(PIC0_OCW2, 0x61);

    // 通过键盘数据端口获取按键信息
    unsigned char data = io_in8(PORT_KEYDAT);

    fifo8_put(&keyfifo, data);

	return;
}
