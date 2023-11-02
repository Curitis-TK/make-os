#include "bootpack.h"

// 初始化FIFO缓冲区
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
{
    fifo->size = size;
    fifo->free = size;
    fifo->buf = buf;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    return;
}

// 向FIFO缓冲区写入数据
int fifo8_put(struct FIFO8 *fifo, unsigned char data)
{
    if (fifo->free == 0)
    {
        // 发生溢出（使用或运算，覆盖指定位置值）
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }

    fifo->buf[fifo->p] = data;

    fifo->p++;
    if (fifo->p == fifo->size)
    {
        fifo->p = 0;
    }

    fifo->free--;
    return 0;
}

// 从FIFO缓冲区获取数据
int fifo8_get(struct FIFO8 *fifo)
{
    if (fifo->free == 0)
    {
        // 缓冲区为空
        return -1;
    }

    int data = fifo->buf[fifo->q];
    fifo->q++;

    if (fifo->q == fifo->size)
    {
        fifo->q = 0;
    }

    fifo->free++;

    return data;
}

// 获取FIFO已用空间
int fifo8_status(struct FIFO8 *fifo)
{
    return fifo->size - fifo->free;
}

/* 初始化FIFO缓冲区
    - *fifo 缓冲区地址
    - size 缓冲区大小
    - *buf 缓冲区数据地址
 */
void fifo32_init(struct FIFO32 *fifo, int size, int *buf)
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    return;
}

/* 向FIFO缓冲区写入数据
    - *fifo 缓冲区
    - data 写入数据
 */
int fifo32_put(struct FIFO32 *fifo, int data)
/*给FIFO发送数据并储存在FIFO中*/
{
    if (fifo->free == 0) // 没有空余空间，溢出
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size)
    {
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

/* 从FIFO缓冲区获取数据
    - *fifo 缓冲区
 */
int fifo32_get(struct FIFO32 *fifo)
{
    int data;
    if (fifo->free == fifo->size) // 缓冲区为空
    {
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size)
    {
        fifo->q = 0;
    }
    fifo->free++;

    return data;
}

/* 获取FIFO已用空间
    - *fifo 缓冲区
 */
int fifo32_status(struct FIFO32 *fifo)
{
    return fifo->size - fifo->free;
}