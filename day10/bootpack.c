#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容
    init_gdtidt();                                            // 初始化GDT、IDT
    init_pic();                                               // 初始化PIC
    io_sti();                                                 // 开放中断
    init_palette();                                           // 初始化调色板

    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    // enable keyboard and mouse
    io_out8(PIC0_IMR, 0xf9); /* PIC1和允许键盘(11111001)*/
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标(11101111) */
    int key_data;
    init_keyboard();
    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    // memory manager
    unsigned int memtotal;
    memtotal = memtest(0x00400000, 0xbfffffff);
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR; // memman放在0x003c0000（需要32KB）
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000开始  0x0009efff可用 （632KB）
    // memman需要32KB
    memman_free(memman, 0x00400000, memtotal - 0x00400000); // 0x00400000开始  （内存总量-开始地址）可用 （28672KB）

    // sheet control
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse;
    unsigned char *buf_back, buf_mouse[256];

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);

    init_screen8(buf_back, binfo->scrnx, binfo->scrny); // 初始化屏幕图像buf
    init_mouse_cursor8(buf_mouse, 99);                  // 初始化鼠标图像buf

    sheet_slide(shtctl, sht_back, 0, 0);
    // 屏幕中心坐标计算
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(shtctl, sht_mouse, mx, my);

    sheet_updown(shtctl, sht_back, 0);
    sheet_updown(shtctl, sht_mouse, 1);

    sprintf(s, "memory %dKB   free : %dKB", memtotal / (1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, 48);

    for (;;)
    {
        io_cli();

        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0)
        {
            io_stihlt();
        }
        else
        {
            if (fifo8_status(&keyfifo) != 0) // 键盘缓冲区有数据
            {
                key_data = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%02X", key_data);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                // 获取鼠标当前数据
                key_data = fifo8_get(&mousefifo);
                if (mouse_decode(&mdec, key_data) != 0)
                {
                    /* 3字节都凑齐了，所以把它们显示出来*/

                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                    {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0)
                    {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0)
                    {
                        s[2] = 'C';
                    }
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    sheet_refresh(shtctl, sht_back, 32, 16, 32 + 15 * 8, 32);

                    // 移动光标
                    mx += mdec.x;
                    my += mdec.y;
                    // 屏幕边缘处理
                    if (mx < 0)
                    {
                        mx = 0;
                    }
                    if (my < 0)
                    {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16)
                    {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16)
                    {
                        my = binfo->scrny - 16;
                    }

                    // 显示鼠标坐标
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 隐藏坐标
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // 显示坐标
                    sheet_refresh(shtctl, sht_back, 0, 0, 80, 16);
                    // 更新鼠标位置
                    sheet_slide(shtctl, sht_mouse, mx, my); // 描画鼠标
                }
            }
        }
    }
}
