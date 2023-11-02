#include <stdio.h>
#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    init_gdtidt(); // 初始化GDT、IDT
    init_pic();    // 初始化PIC
    io_sti();      // 开放中断

    // 初始化32位FIFO
    int fifobuf[128];
    struct FIFO32 fifo;
    fifo32_init(&fifo, 128, fifobuf);

    // 激活键鼠
    io_out8(PIC0_IMR, 0xf8); /* PIC1和允许键盘还有定时器(11111000)*/
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标(11101111) */
    int key_data;
    init_keyboard(&fifo, 256);
    struct MOUSE_DEC mdec;
    enable_mouse(&fifo, 512, &mdec);

    // 初始化可编程定时器
    init_pit();

    struct TIMER *timer, *timer2, *timer3;

    timer = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_settime(timer, 1000);

    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);

    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);

    // 初始化内存管理
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
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

    int i, count = 0;

    for (;;)
    {
        count++;
        // sprintf(s, "%010d", timerctl.count);
        // putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);

        io_cli();

        if (fifo32_status(&fifo) == 0)
        {
            io_sti();
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();

            if (256 <= i && i <= 511) // 键盘缓冲数据
            {
                sprintf(s, "%02X", i - 256);

                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
            }
            else if (512 <= i && i <= 767) // 鼠标缓冲数据
            {
                // 获取鼠标当前数据
                if (mouse_decode(&mdec, i - 512) != 0)
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

                    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

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
                    // 下侧
                    if (my > binfo->scrny - 1)
                    {
                        my = binfo->scrny - 1;
                    }

                    // 显示鼠标坐标
                    sprintf(s, "(%3d, %3d)", mx, my);

                    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    // 更新鼠标位置
                    sheet_slide(sht_mouse, mx, my); // 描画鼠标
                }
            }
            else if (i == 10) // 10秒定时器
            {
                putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
                sprintf(s, "%010d", count);
                putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
            }
            else if (i == 3) // 3秒定时器
            {
                putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
                count = 0;
            }
            else if (i == 0) // 光标显示
            {
                timer_init(timer3, &fifo, 0);
                boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
            else if (i == 1) // 光标隐藏
            {
                timer_init(timer3, &fifo, 1);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
        }
    }
}
