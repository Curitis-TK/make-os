#include "bootpack.h"

// 0100 0000 0000 0000 0000
// 第18位 对齐检查，只有486+ CPU才可使用此寄存器
#define EFLAGS_AC_BIT 0x00040000
// 0110 0000 0000 0000 0000 0000 0000 0000
// 29位Not-write through；30位Cache disable
// 全局启用/禁用透写缓存；全局启用/禁用内存缓存
#define CR0_CACHE_DISABLE 0x60000000

// 内存测试
unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 确认CPU是否486+
    eflg = io_load_eflags(); // 读取EFlags
    eflg |= EFLAGS_AC_BIT;   // 把AC位设置成1
    io_store_eflags(eflg);   // 回写EFlags
    eflg = io_load_eflags(); // 再次读取EFlags

    if ((eflg & EFLAGS_AC_BIT) != 0)
    { // 如果是386，即使设定AC=1，AC的值还会自动回到0
        // 当前CPU属于486+
        flg486 = 1;
    }

    eflg &= ~EFLAGS_AC_BIT; // 恢复AC=0
    io_store_eflags(eflg);

    if (flg486 != 0)
    { // 如果CPU属于486+，需要禁用CPU缓存
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    // 调用内存测试子函数，获取可用内存大小
    i = memtest_sub(start, end);
}

// 内存测试子函数，改为使用汇编完成
/* unsigned int memtest_sub(unsigned int start, unsigned int end){
    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;

    for(i = start; i<= end; i += 0x1000) {      // 每次测试4KB
        p = (unsigned int *) (i + 0xffc); // 设置地址（只检查末尾的4个字节）速度+++++++++（理论快1000倍，此前一次只检查4B）
        old = *p;               // 保存原数据
        *p = pat0;              // 试写
        *p ^= 0xffffffff;       // 反转
        if (*p != pat1) {       // 检查是否反转正确
            not_memory:             // 跳转标签；比对失败，此处开始内存不可用
                *p = old;           // 恢复数据
                break;              // 结束循环
        }

        *p ^= 0xffffffff;       // 再次反转
        if (*p != pat0) {       // 检查是否反转正确
            goto not_memory;        // 跳转标签；比对失败
        }
        // 对比完成，本段内存可用，恢复原数据
        *p = old;
    }
}
 */

// 初始化内存管理
void memman_init(struct MEMMAN *man)
{
    man->frees = 0;    // 可用信息数目
    man->maxfrees = 0; // 用于观察可用状况：frees的最大值
    man->lostsize = 0; // 释放失败的内存的大小总和
    man->losts = 0;    // 释放失败次数
    return;
}

// 获取可用内存
unsigned int memman_total(struct MEMMAN *man)
{
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++)
    {
        t += man->free[i].size;
    }
    return t;
}

// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
    unsigned int i, a;

    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].size >= size) // 寻找内存可用段
        {
            a = man->free[i].addr;     // 记录分配到的内存地址
            man->free[i].addr += size; // 地址减去已分配部分
            man->free[i].size -= size; // 可用空间减去已分配部分

            // 删除size为0的可用空间片段
            if (man->free[i].size == 0)
            {
                man->frees--; // 可用段-1

                // 记录向前位移
                for (; i < man->frees; i++)
                {
                    man->free[i] = man->free[i + 1];
                }
            }

            // 返回空间地址
            return a;
        }
    }
    // 找不到可用空间
    return 0;
}

// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i, j;
    // 找到往后最近的地址
    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].addr > addr)
        {
            break;
        }
    }

    if (i > 0)
    { // 前方有可用内存

        if (man->free[i - 1].addr + man->free[i - 1].size == addr)
        {                                  // 并且释放内存地址与前一部分可用区相连
            man->free[i - 1].size += size; // 合并内存空间

            if (i < man->frees)
            { // 后方有可用内存
                if (addr + size == man->free[i].addr)
                { // 如果释放内存与后一部分可用区相连
                    // 把后面的内存空间合并
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    // 把后面的可用内存往前挪
                    for (; i < man->frees; i++)
                    {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            // 完成
            return 0;
        }
    }

    // 前方没有可用空间可合并
    if (i < man->frees)
    {
        if (addr + size == man->free[i].addr)
        { // 如果释放内存与后一部分可用区相连
            // 把后面的内存空间合并
            man->free[i].addr = addr;
            man->free[i].size += size;
            // 完成
            return 0;
        }
    }

    // 前后都没有可用空间合并，判断可用空间数是否达到限制
    if (man->frees < MEMMAN_FREES)
    {
        // free[i]之后的，向后移动，腾出位置用来放新地址的可用空间
        for (j = man->frees; j > i; j--)
        {
            man->free[j] = man->free[j - 1];
        }
        man->frees++; // 可用空间数量+1

        // 更新最大值
        if (man->maxfrees < man->frees)
        {
            man->maxfrees = man->frees;
        }

        // 写入新空间的信息
        man->free[i].addr = addr;
        man->free[i].size = size;

        // 成功完成
        return 0;
    }

    // 无法合并可用内存，并且达到了限制
    man->losts++;
    man->lostsize += size;

    // 释放失败
    return -1;
}

// 分配内存(4K)
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    unsigned int addr;
    // 添加偏移值，以便于按照4KB向下取整
    size = (size + 0xfff) & 0xfffff000;
    addr = memman_alloc(man, size);
    return addr;
}

// 释放内存(4K)
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;
    // 添加偏移值，以便于按照4KB向下取整
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}