#include <stdio.h>
#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    init_gdtidt(); // 初始化GDT、IDT
    init_pic();    // 初始化PIC
    io_sti();      // 开放中断

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    // init pit
    init_pit();

    struct FIFO8 timerfifo, timerfifo2, timerfifo3;
    struct TIMER *timer, *timer2, *timer3;
    char timerbuf[8], timerbuf2[8], timerbuf3[8];

    fifo8_init(&timerfifo, 8, timerbuf);
    timer = timer_alloc();
    timer_init(timer, &timerfifo, 1);
    timer_settime(timer, 1000);

    fifo8_init(&timerfifo2, 8, timerbuf2);
    timer2 = timer_alloc();
    timer_init(timer2, &timerfifo2, 1);
    timer_settime(timer2, 300);

    fifo8_init(&timerfifo3, 8, timerbuf3);
    timer3 = timer_alloc();
    timer_init(timer3, &timerfifo3, 1);
    timer_settime(timer3, 50);

    // enable keyboard and mouse
    io_out8(PIC0_IMR, 0xf8); /* PIC1和允许键盘还有定时器(11111000)*/
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

    init_palette(); // 初始化调色板

    // 初始化图层控制
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);

    // 图层声明
    sht_back = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);

    // 图层内存声明
    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 68);

    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    sheet_setbuf(sht_win, buf_win, 160, 68, -1);

    init_screen8(buf_back, binfo->scrnx, binfo->scrny); // 初始化屏幕图像buf
    init_mouse_cursor8(buf_mouse, 99);                  // 初始化鼠标图像buf

    make_window8(buf_win, 160, 68, "counter"); // 绘制计数器窗口

    sheet_slide(sht_back, 0, 0);
    // 屏幕中心坐标计算
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);

    sheet_slide(sht_win, 80, 72);

    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);

    sprintf(s, "memory %dKB   free : %dKB", memtotal / (1024), memman_total(memman) / 1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

    for (;;)
    {
        sprintf(s, "%010d", timerctl.count);
        boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
        putfonts8_asc(buf_win, 160, 40, 28, COL8_000000, s);
        sheet_refresh(sht_win, 40, 28, 120, 44);
        io_cli();

        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo) + fifo8_status(&timerfifo2) + fifo8_status(&timerfifo3)) == 0)
        {
            io_sti();
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
                sheet_refresh(sht_back, 0, 16, 16, 32);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                // 获取鼠标当前数据
                key_data = fifo8_get(&mousefifo);
                io_sti();
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
                    sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);

                    // 移动光标
                    mx += mdec.x;
                    my += mdec.y;

                    // 屏幕边缘处理
                    // 左侧
                    if (mx < 0)
                    {
                        mx = 0;
                    }
                    // 上侧
                    if (my < 0)
                    {
                        my = 0;
                    }
                    // 右侧
                    if (mx > binfo->scrnx - 1)
                    {
                        mx = binfo->scrnx - 1;
                    }
                    // 下测
                    if (my > binfo->scrny - 1)
                    {
                        my = binfo->scrny - 1;
                    }

                    // 显示鼠标坐标
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); // 隐藏坐标
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); // 显示坐标
                    sheet_refresh(sht_back, 0, 0, 80, 16);
                    // 更新鼠标位置
                    sheet_slide(sht_mouse, mx, my); // 描画鼠标
                }
            }
            else if (fifo8_status(&timerfifo) != 0)
            {
                int i = fifo8_get(&timerfifo); /* 首先读入（为了设定起始点） */
                io_sti();
                putfonts8_asc(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, "10[sec]");
                sheet_refresh(sht_back, 0, 64, 56, 80);
            }
            else if (fifo8_status(&timerfifo2) != 0)
            {
                int i = fifo8_get(&timerfifo2);
                io_sti();
                putfonts8_asc(buf_back, binfo->scrnx, 0, 80, COL8_FFFFFF, "3[sec]");
                sheet_refresh(sht_back, 0, 80, 48, 96);
            }
            else if (fifo8_status(&timerfifo3) != 0)
            {
                int i = fifo8_get(&timerfifo3);
                io_sti();
                if (i != 0) // 光标显示
                {
                    timer_init(timer3, &timerfifo3, 0); // 设定定时器数据0

                    boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                }
                else // 光标隐藏
                {
                    timer_init(timer3, &timerfifo3, 1); // 设定定时器数据1
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
                }

                timer_settime(timer3, 50); // 设定下一轮定时器
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
        }
    }
}

/* 创建窗口
    - *buf 内容缓冲区
    - xsize 窗口宽度
    - ysize 窗口高度
    - *title 标题
 */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
{
    // 关闭按钮
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"};
    int x, y;
    char c;
    // 绘制窗口本体
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
    // 绘制标题
    putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);

    // 绘制关闭按钮
    for (y = 0; y < 14; y++)
    {
        for (x = 0; x < 16; x++)
        {
            c = closebtn[y][x];
            if (c == '@')
            {
                c = COL8_000000;
            }
            else if (c == '$')
            {
                c = COL8_848484;
            }
            else if (c == 'Q')
            {
                c = COL8_C6C6C6;
            }
            else
            {
                c = COL8_FFFFFF;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}
