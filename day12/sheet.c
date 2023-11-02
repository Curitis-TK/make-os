/* 图层 */
#include "bootpack.h"

/* 初始化图层控制
    - *ctl: 图层控制
    - *memman: 内存管理
    - *vram: 显存地址
    - xsize: 屏幕宽度
    - ysize: 屏幕高度
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *ctl;
    int i;
    // 声明内存空间
    ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0)
    { // 内存分配失败
        goto err;
    }
    // 初始化Map
    ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0)
    {
        memman_free_4k(memman, (int)ctl, sizeof(struct SHTCTL));
        goto err;
    }
    // 写入基本配置
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    // 初始化图层
    ctl->top = -1;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        ctl->sheets0[i].flags = 0; /* 标记为未使用 */
        ctl->sheets0[i].ctl = ctl;
    }
err:
    return ctl;
}

/* 声明图层
    - *ctl: 图层控制
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++)
    {
        if (ctl->sheets0[i].flags == 0)
        { // 寻找
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE; /* 标记为正在使用*/
            sht->height = -1;       /* 隐藏 */
            return sht;
        }
    }
    return 0; /* 所有的SHEET都处于正在使用状态*/
}

/* 设定图层内容
    - *sht: 图层
    - *buf: 内容
    - xsize: 窗口宽度
    - ysize: 窗口高度
    - col_inv: 用于透明效果的色号
 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

/* 设定图层高度
    - *ctl: 图层控制
    - *sht: 图层
    - height: 图层高度
 */
void sheet_updown(struct SHEET *sht, int height)
{
    int h;
    int old = sht->height; // 保存设置前的高度信息
    struct SHTCTL *ctl = sht->ctl;

    // 修正高度信息
    if (height > ctl->top + 1) // 如果高度过高
    {
        height = ctl->top + 1;
    }

    if (height < -1)
    { // 如果高度过低
        height = -1;
    }

    sht->height = height; // 设定新的图层高度

    // 进行sheets[ ]的重新排列
    if (old > height) // 比之前低，向下挪动
    {
        if (height >= 0) // 如果图层可见
        {
            for (h = old; h > height; h--)
            {
                ctl->sheets[h] = ctl->sheets[h - 1]; // 图层向上挪，空出位置给要移动高度的图层
                ctl->sheets[h]->height = h;          // 更新高度信息
            }
            // 把目标图层放进空出的位置
            ctl->sheets[height] = sht;
        }
        else
        {                       // 高度-1，图层不可见
            if (ctl->top > old) // 如果图层不是最顶层
            {
                // 把上面的图层向下挪，填补原图层让出的空位
                for (h = old; h < ctl->top; h++)
                {
                    ctl->sheets[h] = ctl->sheets[h - 1];
                    ctl->sheets[h]->height = h;
                }
            }
            // 图层总高度-1
            ctl->top--;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
    }
    else if (old < height) // 比之前高
    {
        if (old >= 0) // 如果原来是可见图层
        {
            // 把中间的图层向下挪动
            for (h = old; h < height; h++)
            {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        else // 隐藏图层转为可见图层，从目标高度向上挪动腾出空间
        {
            for (h = ctl->top; h >= height; h--)
            {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }

        // 刷新画面
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }

    return;
}

/* 刷新图层
    - *ctl: 图层控制
    - *sht: 图层
    - bx0: 相对于图层的刷新区域坐标x0
    - by0: 相对于图层的刷新区域坐标y0
    - bx1: 相对于图层的刷新区域坐标x1
    - by1: 相对于图层的刷新区域坐标y1
 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
    struct SHTCTL *ctl = sht->ctl;

    if (sht->height >= 0) // 如果正在显示，则按图层的信息刷新画面
    {
        sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
    return;
}

/* 图层移动
    - *ctl: 图层控制
    - *sth: 图层
    - vx0: 空间区域x轴位置
    - vy0: 空间区域y轴位置
*/
void sheet_slide(struct SHEET *sht, int vx0, int vy0)
{
    struct SHTCTL *ctl = sht->ctl;
    // 取出旧坐标
    int old_vx0 = sht->vx0;
    int old_vy0 = sht->vy0;
    // sht设置新坐标
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0)
    {
        // 刷新map
        sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        // 刷新旧区域，避免残影
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        // 刷新新区域
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

/* 刷新范围刷新画面图层
    - *ctl: 图层控制
    - vx0: 空间区域坐标x0
    - vy0: 空间区域坐标y0
    - vx1: 空间区域坐标x1
    - vy1: 空间区域坐标y1
    - h0: 刷新最小高度
    - h1: 刷新最大高度
 */
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, c, *vram = ctl->vram, *map = ctl->map, sid;
    struct SHEET *sht;

    // 屏幕空间坐标修正
    if (vx0 < 0)
    {
        vx0 = 0;
    }
    if (vy0 < 0)
    {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize)
    {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize)
    {
        vy1 = ctl->ysize;
    }

    for (h = h0; h <= h1; h++)
    {
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;
        // 使用vx0～vy1，对相对图层的刷新区域bx0～by1进行倒推
        // 获取相对于当前图层的刷新区域
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;

        // 图层左侧与刷新区发生重叠
        if (bx0 < 0)
        {
            bx0 = 0;
        }

        // 图层上侧与刷新区发生重叠
        if (by0 < 0)
        {
            by0 = 0;
        }

        // 图层右侧与刷新区发生重叠
        if (bx1 > sht->bxsize)
        {
            bx1 = sht->bxsize;
        }

        // 图层下侧与刷新区发生重叠
        if (by1 > sht->bysize)
        {
            by1 = sht->bysize;
        }

        // 在图层限定区域刷新，图层没有重叠时不会进入该循环
        for (by = by0; by < by1; by++)
        {
            vy = sht->vy0 + by; // 显存y坐标 = 画板y0坐标 + 刷新区y坐标

            for (bx = bx0; bx < bx1; bx++)
            {
                vx = sht->vx0 + bx; // 显存x坐标 = 画板x0坐标 + 刷新区x坐标

                c = buf[by * sht->bxsize + bx]; // 获取buf中的色值

                if (map[vy * ctl->xsize + vx] == sid) // 无需在判断透明色，map中已经排除；去map查找对应位置是否出现重叠情况，如有重叠则不重绘
                {
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                }
            }
        }
    }
    return;
}

/* 释放图层
    - *ctl: 图层控制
    - *sht: 图层
 */
void sheet_free(struct SHEET *sht)
{
    struct SHTCTL *ctl = sht->ctl;

    if (sht->height >= 0)
    {                          // 如果图层当前可见，则需要进行隐藏操作
        sheet_updown(sht, -1); // 设置图层高度为-1
    }
    sht->flags = 0; // 写入"未使用"标志
    return;
}

/* 刷新map
    - *ctl: 图层控制
    - vx0: 空间区域坐标x0
    - vy0: 空间区域坐标y0
    - vx1: 空间区域坐标x1
    - vy1: 空间区域坐标y1
    - h0: 刷新最低高度
 */
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)
{
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char *buf, sid, *map = ctl->map;
    struct SHEET *sht;

    // 屏幕空间坐标修正
    if (vx0 < 0)
    {
        vx0 = 0;
    }
    if (vy0 < 0)
    {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize)
    {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize)
    {
        vy1 = ctl->ysize;
    }
    for (h = h0; h <= ctl->top; h++)
    {
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0; // 图层地址 - 图层信息存储地址 = 图层编号
        buf = sht->buf;

        // 使用vx0～vy1，对相对图层的刷新区域bx0～by1进行倒推
        // 获取相对于当前图层的刷新区域
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;

        // 重叠区域修正
        if (bx0 < 0)
        {
            bx0 = 0;
        }
        if (by0 < 0)
        {
            by0 = 0;
        }
        if (bx1 > sht->bxsize)
        {
            bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize)
        {
            by1 = sht->bysize;
        }

        // 在图层限定区域刷新，图层没有重叠时不会进入该循环
        for (by = by0; by < by1; by++)
        {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = sht->vx0 + bx;
                if (buf[by * sht->bxsize + bx] != sht->col_inv)
                {
                    map[vy * ctl->xsize + vx] = sid;
                }
            }
        }
    }
    return;
}