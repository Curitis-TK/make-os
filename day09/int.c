#include "bootpack.h"
#define PORT_KEYDAT     0x0060

// 初始化PIC
void init_pic(void){
    /*
    ICW: 初始化控制数据 initial control word

    IMR: 中断屏蔽寄存器 interrupt mask register

    INT 0x00~0x1f不能用于IRQ，是CPU内部预留用于系统保护通知的

    对于部分机种而言，随着PIC的初始化，会产生一次IRQ7中断，如果不对该中断处理程序执行STI，操作系统的启动会失败。
     */
    io_out8(PIC0_IMR,  0xff  ); /* 禁止所有中断 */
    io_out8(PIC1_IMR,  0xff  ); /* 禁止所有中断 */

    io_out8(PIC0_ICW1, 0x11  ); /* 边沿触发模式（edge trigger mode） */
    io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7由INT20-27接收 */
    io_out8(PIC0_ICW3, 1 << 2); /* PIC1由IRQ2连接*/
    io_out8(PIC0_ICW4, 0x01  ); /* 无缓冲区模式 */

    io_out8(PIC1_ICW1, 0x11  ); /* 边沿触发模式（edge trigger mode） */
    io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15由INT28-2f接收 */
    io_out8(PIC1_ICW3, 2     ); /* PIC1由IRQ2连接 */
    io_out8(PIC1_ICW4, 0x01  ); /* 无缓冲区模式 */

    io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外全部禁止 */
    io_out8(PIC1_IMR,  0xff  ); /* 11111111 禁止所有中断 */

    return;
}

void inthandler27(int *esp){
    // 通知PIC: IRQ-07已经受理完毕
	io_out8(PIC0_OCW2, 0x67);
	return;
}
