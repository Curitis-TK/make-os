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
