// void write_mem8(int addr, int data);
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

// 就算在同一个文件里，如果想在定义前使用，须事先声明
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);

// 颜色常量
#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15

void HariMain(void){
	int xsize,ysize;		// 32位整数型
	char *vram;	// 用于表示BYTE型地址（2字节8位，实际上还是4字节32位的）

	init_palette();					// 初始化调色板

	vram = (char *) 0xa0000;			// 指定用于显示的内存地址
	xsize = 320;
    ysize = 200;

	// 桌面背景色
	boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize - 1 , ysize );
	// 任务栏顶部描边
    boxfill8(vram, xsize, COL8_C6C6C6,  0, ysize - 28, xsize - 1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF,  0, ysize - 27, xsize - 1, ysize - 27);
	// 任务栏背景
    boxfill8(vram, xsize, COL8_C6C6C6,  0, ysize - 26, xsize -  1, ysize -  1);

	// 开始按钮描边
    boxfill8(vram, xsize, COL8_FFFFFF,  3, ysize - 24, 59, ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF,  2, ysize - 24,  2, ysize -  4);
    boxfill8(vram, xsize, COL8_848484,  3, ysize -  4, 59, ysize -  4);
    boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 59, ysize -  5);
	// 开始按钮阴影
    boxfill8(vram, xsize, COL8_000000,  2, ysize -  3, 59, ysize -  3);
    boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 60, ysize -  3);

	// 图标区描边
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);

	for(;;){
		io_hlt(); 
	}
}


// 初始化调色板
void init_palette(void){
	static unsigned char table_rgb[16*3] = {
		0x00, 0x00, 0x00,   /*  0:黑 */
        0xff, 0x00, 0x00,   /*  1:亮红 */
        0x00, 0xff, 0x00,   /*  2:亮绿 */
        0xff, 0xff, 0x00,   /*  3:亮黄 */
        0x00, 0x00, 0xff,   /*  4:亮蓝 */
        0xff, 0x00, 0xff,   /*  5:亮紫 */
        0x00, 0xff, 0xff,   /*  6:浅亮蓝 */
        0xff, 0xff, 0xff,   /*  7:白 */
        0xc6, 0xc6, 0xc6,   /*  8:亮灰 */
        0x84, 0x00, 0x00,   /*  9:暗红 */
        0x00, 0x84, 0x00,   /* 10:暗绿 */
        0x84, 0x84, 0x00,   /* 11:暗黄 */
        0x00, 0x00, 0x84,   /* 12:暗青 */
        0x84, 0x00, 0x84,   /* 13:暗紫 */
        0x00, 0x84, 0x84,   /* 14:浅暗蓝 */
        0x84, 0x84, 0x84    /* 15:暗灰 */
	};

	set_palette(0, 15, table_rgb);

	return;
}

// 设置调色板
void set_palette(int start, int end, unsigned char *rgb){
	int i, eflags;
	eflags = io_load_eflags();
	io_cli();
	io_out8(0x03c8,start);
	for (i = start; i <= end; i++){
		// 暂时不知为啥除4；已知的是，不除以4时，暗色都无法正常显示
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);	
		rgb += 3;						// 下一轮RGB
	}
	io_store_eflags(eflags);
	return;
}

// 使用8位颜色绘制盒子
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    }
    return;
}