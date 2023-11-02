#include "bootpack.h"

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

// 初始化鼠标
void enable_mouse(struct MOUSE_DEC *mdec)
{
    wait_KBC_sendready();
    // 如果往键盘控制电路发送指令0xd4，下一个数据就会自动发送给鼠标。
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // 顺利的话，ACK(0xfa)会被送过来
    // 等待0xfa
    mdec->phase = 0;
    return;
}

// 鼠标解码
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
    if (mdec->phase == 0)
    {
        // 等待鼠标初始化激活成功
        if (dat == 0xfa)
        {
            mdec->phase = 1;
        }
        return 0;
    }
    if (mdec->phase == 1)
    {
        // 等待鼠标第一字节的阶段

        // 校验
        // 1100 1000 == 0000 1000
        // 高位 0~3 对移动有反应的部分
        // 低位 8~F 对点击有反应的部分
        if ((dat & 0xc8) == 0x08)
        {
            /* 如果第一字节正确 */
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2)
    {
        // 等待鼠标第二字节的阶段
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3)
    {
        // 等待鼠标第二字节的阶段
        mdec->buf[2] = dat;
        mdec->phase = 1;

        // 鼠标键的状态，在buf[0]的低3位 0000 0111
        mdec->btn = mdec->buf[0] & 0x07;

        // x移动距离
        mdec->x = mdec->buf[1];

        // y移动距离
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0)
        { // 通过第一位数据的 0001 0000 取出当前鼠标是左还是右移动，为1时是左移动
            // 符号扩展，这是一种保持数字的符号（正数或负数）的技术
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0)
        { // 通过第一位数据的 0010 0000 取出当前鼠标是左还是右移动，为1时是左移动
            // 符号扩展，这是一种保持数字的符号（正数或负数）的技术
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y; /* 鼠标的y方向与画面符号相反 */
        return 1;
    }
    return -1; /* 应该不可能到这里来 */
}

struct FIFO8 mousefifo;

// PS/2鼠标中断
void inthandler2c(int *esp)
{
    // 通知PIC1: IRQ-12已经受理完毕
    io_out8(PIC1_OCW2, 0x64);
    // 通知PIC0: IRQ-02已经受理完毕
    io_out8(PIC0_OCW2, 0x62);

    unsigned char data = io_in8(PORT_KEYDAT);

    fifo8_put(&mousefifo, data);

    return;
}
