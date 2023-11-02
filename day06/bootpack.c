#include <stdio.h>
#include "bootpack.h"

void HariMain(void){
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容

    init_palette();				// 初始化调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    init_gdtidt();

    init_pic();

    io_sti();


    char s[40], mcursor[256];

    // putfonts8_asc(binfo->vram, binfo->scrnx, 10, 10, 0, "HELLO WORLD");
    // putfonts8_asc(binfo->vram, binfo->scrnx, 9, 9, 2, "HELLO WORLD");

    // sprintf(s, "scrnx = %d", binfo->scrnx);
    // putfonts8_asc(binfo->vram, binfo->scrnx, 10, 26, 0, s);
    // putfonts8_asc(binfo->vram, binfo->scrnx, 9, 25, 2, s);

    init_mouse_cursor8(mcursor, COL8_008484);

    // 屏幕中心坐标计算
    int mx, my;
    mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;


	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	// sprintf(s, "(%d, %d)", mx, my);
    // putfonts8_asc(binfo->vram, binfo->scrnx, 10, 40, COL8_FFFFFF, s);
    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	for(;;){
		io_hlt(); 
	}
}


