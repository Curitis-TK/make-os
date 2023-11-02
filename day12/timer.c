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
    timerctl.next = 0xffffffff;

    int i;
    // 设置未使用状态
    timerctl.using = 0;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = 0;
    }

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
void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
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
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    e = io_load_eflags();
    io_cli();

    for (i = 0; i < timerctl.using; i++)
    {
        if (timerctl.timers[i]->timeout >= timer->timeout)
        {
            break;
        }
    }

    for (j = timerctl.using; j > i; j--) // i号之后全部后移一位
    {
        timerctl.timers[j] = timerctl.timers[j - 1];
    }

    timerctl.using ++; // 活跃计时器+1

    timerctl.timers[i] = timer;                  // 写入列表
    timerctl.next = timerctl.timers[0]->timeout; // 更新下一刻

    io_store_eflags(e);

    return;
}

/* 0号中断调用 */
void inthandler20(int *esp)
{
    io_out8(PIC0_OCW2, 0x60); // 把IRQ-00信号接收完了的信息通知给PIC
    timerctl.count++;         // 设定计数器

    if (timerctl.next > timerctl.count) // 如果还未到下一时刻时间
    {
        return;
    }

    int i, j;
    for (i = 0; i < timerctl.using; i++)
    {
        if (timerctl.timers[i]->timeout > timerctl.count) // 未超时
        {
            break;
        }

        // 超时
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }

    // 减去已超时的定时器
    timerctl.using -= i;
    // 位移定时器
    for (j = 0; j < timerctl.using; j++)
    {
        timerctl.timers[j] = timerctl.timers[i + j];
    }
    // 更新下一时刻
    if (timerctl.using > 0)
    {
        timerctl.next = timerctl.timers[0]->timeout;
    }
    else
    {
        timerctl.next = 0xffffffff;
    }

    return;
}
