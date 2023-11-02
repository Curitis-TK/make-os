#include <stdio.h>

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
void init_screen(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

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

static char font_A[16] = {
    0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
    0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
};

struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

void HariMain(void){
	int xsize,ysize;		    // 32位整数型
	char *vram;	                // 用于表示BYTE型地址（2字节8位，实际上还是4字节32位的）
    struct BOOTINFO *binfo;     // 声明结构体binfo为内存地址

    init_palette();				// 初始化调色板

    binfo = (struct BOOTINFO *) 0x0ff0; // 设置结构体内存地址，从asmhead中设定的位置获取内容
    xsize = (*binfo).scrnx;
    ysize = (*binfo).scrny;
    vram = (*binfo).vram;

    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    char s[40];     // 声明40字节的字符数组
    char mcursor[256];     // 声明256字节的鼠标图像数组

    sprintf(s, "scrnx = %d", binfo->scrnx);

    putfonts8_asc(binfo->vram, binfo->scrnx, 10, 10, 0, "HELLO WORLD!");
    putfonts8_asc(binfo->vram, binfo->scrnx, 9, 9, 2, "HELLO WORLD!");

    putfonts8_asc(binfo->vram, binfo->scrnx, 10, 26, 0, s);
    putfonts8_asc(binfo->vram, binfo->scrnx, 9, 25, 2, s);

    init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, 50, 50, mcursor, 16);
	

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
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    }
    return;
}

// 初始化屏幕
void init_screen(char *vram, int x, int y){
// 桌面背景色
	boxfill8(vram, x, COL8_008484, 0, 0, x - 1 , y );
	// 任务栏顶部描边
    boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, x - 1, y - 28);
    boxfill8(vram, x, COL8_FFFFFF, 0, y - 27, x - 1, y - 27);
	// 任务栏背景
    boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, x -  1, y -  1);

	// 开始按钮描边
    boxfill8(vram, x, COL8_FFFFFF, 3, y - 24, 59, y - 24);
    boxfill8(vram, x, COL8_FFFFFF, 2, y - 24,  2, y -  4);
    boxfill8(vram, x, COL8_848484, 3, y -  4, 59, y -  4);
    boxfill8(vram, x, COL8_848484, 59, y - 23, 59, y -  5);
	// 开始按钮阴影
    boxfill8(vram, x, COL8_000000, 2, y - 3, 59, y -  3);
    boxfill8(vram, x, COL8_000000, 60, y - 24, 60, y -  3);

	// 图标区描边
    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
    boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
    boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
}

// 绘制单个字符
void putfont8(char *vram, int xsize, int x, int y, char color, char *font){
     int i;
    char *p, d /* data */;
    // 循环读取每一行文字数据
    for (i = 0; i < 16; i++) {
        // 写入位置 = 显存起始位置 + (开始位置y + i) * 屏幕宽度 + 开始位置x
        p = vram + (y + i) * xsize + x;
        d = font[i];
        /* 
            利用与运算，判断单行需要上色的位置，一行8列

            例如字母A的第二行为：0001 1000
            则会把颜色写入第四列、第五列
         */
        if ((d & 0x80) != 0) { p[0] = color; }  // 1000 0000: 第1列
        if ((d & 0x40) != 0) { p[1] = color; }  // 0100 0000: 第2列
        if ((d & 0x20) != 0) { p[2] = color; }  // 0010 0000: 第3列
        if ((d & 0x10) != 0) { p[3] = color; }  // 0001 0000: 第4列
        if ((d & 0x08) != 0) { p[4] = color; }  // 0000 1000: 第5列
        if ((d & 0x04) != 0) { p[5] = color; }  // 0000 0100: 第6列
        if ((d & 0x02) != 0) { p[6] = color; }  // 0000 0010: 第7列
        if ((d & 0x01) != 0) { p[7] = color; }  // 0000 0001: 第8列
    }
    return;
}

// 正序绘制字符
void putfonts8_asc(char *vram, int xsize, int x, int y, char color, unsigned char *s)
{
    extern char hankaku[4096];          // 声明有一个4096字节的外部数据hankaku

    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, color, hankaku + *s * 16);   // hankaku内存位置 + (ASCII对应编码 * 16)偏移
        x += 8;
    }
    return;
}

// 初始化光标外观
void init_mouse_cursor8(char *mouse, char bc){
    /* 准备鼠标指针（16×16） */
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };

    // 遍历设置所有像素颜色
    int x, y;

    for (y = 0; y < 16; y++){
        for (x = 0; x < 16; x++){
            if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
        }
    }
    return;
}

/* 
    绘制内容块
    vram 显示内存地址
    vxsize  显示宽度
    pxsize  预期绘制宽度
    pysize  预期绘制高度
    px0     绘制位置x
    py0     绘制位置y
    buf     绘制内容内存地址
    bxsize  内容每一行的像素量（与pxsize类似，以后可能会单独指定使用）
 */
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize){
    int x, y;
    for (y = 0; y < pysize; y++) {
        for (x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}