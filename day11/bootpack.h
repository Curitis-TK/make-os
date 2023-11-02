/* asmhead.nas */
struct BOOTINFO
{               /* 0x0ff0-0x0fff */
    char cyls;  /* ブートセクタはどこまでディスクを読んだのか */
    char leds;  /* ブート時のキーボードのLEDの状態 */
    char vmode; /* ビデオモード  何ビットカラーか */
    char reserve;
    short scrnx, scrny; /* 画面解像度 */
    char *vram;
};
#define ADR_BOOTINFO 0x00000ff0

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
int load_cr0(void);
void store_cr0(int cr0);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
                 int pysize, int px0, int py0, char *buf, int bxsize);

// 颜色常量
#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

/* dsctbl.c */
// 8字节结构体
struct SEGMENT_DESCRIPTOR
{
    /*
        base（32位）, base又分为low（2字节）\mid（1字节）\high（1字节），为了与80286时代的程序兼容。
        limit（24位）, 其中段属性会放在高4位
        access（8位）, 段属性的另外一部分
     */
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

struct GATE_DESCRIPTOR
{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT 0x0026f800
#define LIMIT_IDT 0x000007ff
#define ADR_GDT 0x00270000
#define LIMIT_GDT 0x0000ffff
#define ADR_BOTPAK 0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_INTGATE32 0x008e

/* int.c */
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1

/* fifo */
#define FLAGS_OVERRUN 0x0001
// FIFO结构体
struct FIFO8
{
    // 缓冲区内存地址
    unsigned char *buf;
    // 下一写入位置
    int p;
    // 下一读取位置
    int q;
    // 缓冲区大小
    int size;
    // 缓冲区剩余空间
    int free;
    // 状态记录
    int flags;
};

// 初始化FIFO缓冲区
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
// 向FIFO缓冲区写入数据
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
// 从FIFO缓冲区获取数据
int fifo8_get(struct FIFO8 *fifo);
// 获取FIFO已用空间
int fifo8_status(struct FIFO8 *fifo);

/* keyboard */
// 键盘数据端口
#define PORT_KEYDAT 0x0060
// 键盘控制器状态数据端口
#define PORT_KEYSTA 0x0064
// 键盘命令端口
#define PORT_KEYCMD 0x0064
// 等待键盘控制电路准备完毕
void wait_KBC_sendready();
// 初始化键盘
void init_keyboard();

/* mouse */
// 鼠标解码数据结构体
struct MOUSE_DEC
{
    // 暂存当前鼠标数据, 读取阶段
    unsigned char buf[3], phase;
    int x, y, btn;
};
// 初始化鼠标
void enable_mouse(struct MOUSE_DEC *mdec);
// 鼠标解码
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory */
// 内存测试
unsigned int memtest(unsigned int start, unsigned int end);
// 内存测试子函数
unsigned int memtest_sub(unsigned int start, unsigned int end);

// 内存管理最大可用记录数量
#define MEMMAN_FREES 4090 // 大约是32KB
// 内存管理器的内存地址
#define MEMMAN_ADDR 0x003c0000

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
// 分配内存(4K)
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
// 释放内存(4K)
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet */
// 最大图层数
#define MAX_SHEETS 256
// 图层正在使用设定 0001
#define SHEET_USE 1
// 图层
struct SHEET
{
    unsigned char *buf; // 图层内容内存位置
    int bxsize, bysize; // 图层的整体大小
    int vx0, vy0;       // 图层在画面上位置的坐标
    int col_inv;        // 透明色色号
    int height;         // 图层高度
    int flags;          // 图层设定信息
    struct SHTCTL *ctl; // 自带图层控制地址
};

// 图层控制结构体
struct SHTCTL
{
    // 显示内存地址
    unsigned char *vram;
    // map内存地址
    unsigned char *map;
    // 屏幕宽度/屏幕宽度/最高图层号
    int xsize, ysize, top;
    // 经过排序的所有图层的内存地址
    struct SHEET *sheets[MAX_SHEETS];
    // 存放所有图层的信息
    struct SHEET sheets0[MAX_SHEETS];
};

/* 初始化图层控制
    - *ctl: 图层控制
    - *memman: 内存管理
    - *vram: 显存地址
    - xsize: 屏幕宽度
    - ysize: 屏幕高度
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
/* 声明图层
    - *ctl: 图层控制
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
/* 设定图层内容
    - *sht: 图层
    - *buf: 内容
    - xsize: 窗口宽度
    - ysize: 窗口高度
    - col_inv: 用于透明效果的色号
 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
/* 设定图层高度
    - *sht: 图层
    - height: 图层高度
 */
void sheet_updown(struct SHEET *sht, int height);
/* 刷新图层
    - *sht: 图层
    - bx0: 相对于图层的刷新区域坐标x0
    - by0: 相对于图层的刷新区域坐标y0
    - bx1: 相对于图层的刷新区域坐标x1
    - by1: 相对于图层的刷新区域坐标y1
 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
/* 图层移动
    - *sth: 图层
    - vx0: 空间区域x轴位置
    - vy0: 空间区域y轴位置
*/
void sheet_slide(struct SHEET *sht, int vx0, int vy0);

/* 刷新范围刷新画面图层
    - *ctl: 图层控制
    - vx0: 空间区域坐标x0
    - vy0: 空间区域坐标y0
    - vx1: 空间区域坐标x1
    - vy1: 空间区域坐标y1
    - h0: 刷新最小高度
    - h1: 刷新最大高度
 */
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);

/* 释放图层
    - *sht: 图层
 */
void sheet_free(struct SHEET *sht);

/* 刷新map
    - *ctl: 图层控制
    - vx0: 空间区域坐标x0
    - vy0: 空间区域坐标y0
    - vx1: 空间区域坐标x1
    - vy1: 空间区域坐标y1
    - h0: 刷新最低高度
 */
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);