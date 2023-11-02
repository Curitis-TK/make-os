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
