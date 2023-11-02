#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define TIMER_FLAGS_ALLOC 1 // 状态: 已配置
#define TIMER_FLAGS_USING 2 // 状态: 定时器运行中

/* 计时器控制 */
struct TIMERCTL timerctl;

/* 初始化可编程间隔型定时器 */
void init_pit(void)
{
    // 设定中断周期: 0x2e9c; 11932大约是100Hz; 1秒钟会发生100次中断
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);

    // 设定计数器
    timerctl.count = 0;

    // 设置未使用状态
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = 0;
    }

    struct TIMER *t; // 哨兵
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;

    timerctl.t0 = t; // 初始化时，只有哨兵计时器
    timerctl.next = 0xffffffff;

    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 48, COL8_FFFFFF, "pit init");

    return;
}

/* 分配定时器 */
struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        if (timerctl.timers0[i].flags == 0)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }

    return 0;
}

/* 释放定时器
    - *timer 定时器
*/
void timer_free(struct TIMER *timer)
{
    timer->flags = 0; /* 未使用 */
    return;
}

/* 设置计时器参数
    - *timer 定时器
    - *fifo 缓冲区
    - data 输出数据
 */
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

/* 设置计时器时间，并开始计时
    - *timer 定时器
    - timeout 超时时间
 */
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    int e, i, j;

    struct TIMER *t, *s;

    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    e = io_load_eflags();
    io_cli();

    t = timerctl.t0; // 保存原定下一个定时器地址

    if (timer->timeout <= t->timeout) // 如果新定时器会在原有定时器之前超时
    {
        timerctl.t0 = timer;            // 将下一查询的定时器设定为新定时器
        timer->next = t;                // 新定时器的next设定为原有定时器
        timerctl.next = timer->timeout; // 更新下一超时时间
        io_store_eflags(e);
        return;
    }

    // TODO: 此处注释阅读困难，需要修改

    for (;;) // 寻找插入位置
    {
        s = t;       // 保存原定下一个定时器地址
        t = t->next; // 设定为下一个定时器地址

        if (timer->timeout <= t->timeout) // 插入s和t之间
        {
            s->next = timer; // 替换原定下一个定时器的next地址
            timer->next = t; // 新定时器绑定原定下一个定时器地址
            io_store_eflags(e);
            return;
        }
    }
}

/* 0号中断调用 */
void inthandler20(int *esp)
{
    struct TIMER *timer;

    io_out8(PIC0_OCW2, 0x60); // 把IRQ-00信号接收完了的信息通知给PIC
    timerctl.count++;         // 设定计数器

    if (timerctl.next > timerctl.count) // 如果还未到下一时刻时间
    {
        return;
    }

    // 下一个到达的定时器
    timer = timerctl.t0;

    for (;;)
    {
        if (timer->timeout > timerctl.count) // 如果未到超时时间，中断循环
        {
            break;
        }

        // 发生超时
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next; // 将下一个定时器的地址赋给timer
    }

    // 位移更优解
    timerctl.t0 = timer;

    // 更新下一时刻
    timerctl.next = timerctl.t0->timeout;

    return;
}
