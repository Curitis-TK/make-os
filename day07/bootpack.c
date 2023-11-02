#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain(void){
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; // 设置结构体内存地址，从asmhead中设定的位置获取内容

    init_palette();				// 初始化调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    init_gdtidt();

    init_pic();

    io_sti();

    init_keyboard();

    enable_mouse();


    char s[40], mcursor[256], keybuf[32], mousebuf[128];

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

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

    int key_data;

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
                key_data = fifo8_get(&mousefifo);
                io_sti();
                sprintf(s, "Mouse: %02X", key_data);
                boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
            }
        }
        
	}
}



// 初始化键盘
void init_keyboard(){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

// 初始化鼠标
void enable_mouse(){
    wait_KBC_sendready();
    // 如果往键盘控制电路发送指令0xd4，下一个数据就会自动发送给鼠标。
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return; 
    // 顺利的话,键盘控制其会返送回ACK(0xfa)
}

// 等待键盘控制电路准备完毕
void wait_KBC_sendready(){
    for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}
