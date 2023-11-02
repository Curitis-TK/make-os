#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

#define MEMMAN_FREES 4090 // 大约是32KB
#define MEMMAN_ADDR         0x003c0000

// 可用信息结构体
struct FREEINFO
{
    unsigned int addr, size; // 可用开始地址, 可用大小
};

// 内存管理器结构体
struct MEMMAN
{
    int frees, maxfrees, lostsize, losts; // 可用信息条目, 用于观察可用状况（frees的最大值）, 累计释放失败的内存大小, 释放失败次数
    struct FREEINFO free[MEMMAN_FREES];   // 可用空间记录表
};

// 初始化内存管理
void memman_init(struct MEMMAN *man);
// 获取可用内存
unsigned int memman_total(struct MEMMAN *man);
// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容
    init_gdtidt();                                            // 初始化GDT、IDT
    init_pic();                                               // 初始化PIC
    io_sti();                                                 // 开放中断
    init_palette();                                           // 初始化调色板
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);    // 初始化屏幕
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    init_mouse_cursor8(mcursor, COL8_008484);
    // 屏幕中心坐标计算
    int mx, my;
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    // 画鼠标
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    io_out8(PIC0_IMR, 0xf9); /* PIC1和允许键盘(11111001)*/
    io_out8(PIC1_IMR, 0xef); /* 允许鼠标(11101111) */
    int key_data;
    init_keyboard();
    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    // memory manager
    unsigned int memtotal;
    memtotal = memtest(0x00400000, 0xbfffffff);
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;  // memman放在0x003c0000（需要32KB）
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000开始  0x0009efff可用 （632KB）
    // memman需要32KB
    memman_free(memman, 0x00400000, memtotal - 0x00400000); // 0x00400000开始  （内存总量-开始地址）可用 （28672KB）


    sprintf(s, "memory %dKB   free : %dKB",
            memtotal / (1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    for (;;)
    {
        io_cli();

        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0)
        {
            io_stihlt();
        }
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                key_data = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "Key: %02X", key_data);
                boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
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
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* 隐藏鼠标 */

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

                    sprintf(s, "(%3d, %3d)", mx, my);
                    // 显示鼠标坐标
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /* 隐藏坐标  */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /* 显示坐标 */
                    // 更新鼠标位置
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* 描画鼠标 */
                }
            }
        }
    }
}

// 初始化内存管理
void memman_init(struct MEMMAN *man)
{
    man->frees = 0;    // 可用信息数目
    man->maxfrees = 0; // 用于观察可用状况：frees的最大值
    man->lostsize = 0; // 释放失败的内存的大小总和
    man->losts = 0;    // 释放失败次数
    return;
}

// 获取可用内存
unsigned int memman_total(struct MEMMAN *man)
{
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++)
    {
        t += man->free[i].size;
    }
    return t;
}

// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
    unsigned int i, a;

    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].size >= size)  // 寻找内存可用段
        {
            a = man->free[i].addr;      // 记录分配到的内存地址
            man->free[i].addr += size;  // 地址减去已分配部分
            man->free[i].size -= size;  // 可用空间减去已分配部分

            // 删除size为0的可用空间片段
            if (man->free[i].size == 0)
            {
                man->frees--;                   // 可用段-1

                // 记录向前位移
                for (; i < man->frees; i++)
                {
                    man->free[i] = man->free[i + 1];
                }
            }

            // 返回空间地址
            return a;
        }
        // 找不到可用空间
        return 0;
    }
}

// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i, j;
    // 找到往后最近的地址
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }

    if (i > 0){ // 前方有可用内存

        if(man->free[i - 1].addr + man->free[i - 1].size == addr){  // 并且释放内存地址与前一部分可用区相连
            man->free[i - 1].size += size;  // 合并内存空间

            if (i < man->frees) {   // 后方有可用内存
                if (addr + size == man->free[i].addr) { // 如果释放内存与后一部分可用区相连
                    // 把后面的内存空间合并
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    // 把后面的可用内存往前挪
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            // 完成
            return 0;
        }
    }

    // 前方没有可用空间可合并
    if (i < man->frees) {
       if (addr + size == man->free[i].addr) {  // 如果释放内存与后一部分可用区相连
            // 把后面的内存空间合并
            man->free[i].addr = addr;
            man->free[i].size += size;
            // 完成
            return 0;
       }
   }

    // 前后都没有可用空间合并，判断可用空间数是否达到限制
    if (man->frees < MEMMAN_FREES) {
        // free[i]之后的，向后移动，腾出位置用来放新地址的可用空间
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;   // 可用空间数量+1

        // 更新最大值
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees;
        }

        // 写入新空间的信息
        man->free[i].addr = addr;
        man->free[i].size = size;

        // 成功完成
        return 0;
    }

    // 无法合并可用内存，并且达到了限制
    man->losts++;
    man->lostsize += size;

    // 释放失败
    return -1;
}
