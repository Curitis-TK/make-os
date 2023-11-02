#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain(void){
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容

    init_gdtidt();

    init_pic();

    io_sti();

    init_palette();				// 初始化调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

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

	for(;;){
		io_cli(); 

        if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0){
            io_stihlt();
        } else {
            if(fifo8_status(&keyfifo) != 0){
                key_data = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "Key: %02X", key_data);
                boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
            } else if(fifo8_status(&mousefifo) != 0){
                // 获取鼠标当前数据
                key_data = fifo8_get(&mousefifo);
                if (mouse_decode(&mdec, key_data) != 0) {
                    /* 3字节都凑齐了，所以把它们显示出来*/

                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0) {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0) {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0) {
                        s[2] = 'C';
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* 隐藏鼠标 */

                    mx += mdec.x;
                    my += mdec.y;
                    // 屏幕边缘处理
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
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
