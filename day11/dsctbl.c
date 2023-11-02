#include "bootpack.h"

// 设置一个段描述符
/* 设置一个段描述符
- struct SEGMENT_DESCRIPTOR *sd：一个指向段描述符结构的指针
- unsigned int limit：段的限制（大小）
- int base：段的基地址
- int ar：段的访问权限和其他属性（Access Rights）
 */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    /*
        ar的高4位被称为“扩展访问权”，这高4位的访问属性在80286的时代还不存在，到386以后才可以使用；
        这4位是由“GD00”构成的，
        其中G是指刚才所说的G bit，决定是否使用4KB页分段（1:是，0:否）
        D是指段的模式，1是指32位模式，0是指16位模式

        ar的低8位，具体含义详见README
    */
    // 如果段大小大于0xfffff，则将G位设置成1，代表段的大小是以4KB页面为单位
    if (limit > 0xfffff)
    {
        ar |= 0x8000;    /* G_bit = 1 将0x8000按位或到ar上，设置G位为1*/
        limit /= 0x1000; // 将limit除以0x1000来将其大小调整为以页面为单位
    }
    sd->limit_low = limit & 0xffff;                               // 将limit的低16位（0-15位）存储在limit_low字段中
    sd->base_low = base & 0xffff;                                 // 将base的低16位（0-15位）存储在base_low字段中
    sd->base_mid = (base >> 16) & 0xff;                           // 利用位偏移，将base的中间8位（16-23位）存储在base_mid字段中。
    sd->access_right = ar & 0xff;                                 // 将ar的低8位存储在access_right字段中
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0); // 将limit的高4位（16-19位）与ar的高4位（8-11位）合并后存储在limit_high字段中
    sd->base_high = (base >> 24) & 0xff;                          // 将base的最高8位（24-31位）存储在base_high字段中。
    return;
}

void init_gdtidt(void)
{
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT; // 全局段号记录表地址
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *)ADR_IDT;       // 中断记录表地址
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    int i;

    /* 初始化GDT */
    for (i = 0; i <= LIMIT_GDT / 8; i++)
    {
        // 全部置零
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    /*
       设置 GDT 中的第2个描述符（通常 GDT 的第一个描述符是保留给空描述符或者代码段描述符的）
       - 段的大小: 4GB
       - 段的起始地址: 0
       - 访问权限: 可以被读取和写入
     */
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);
    /*
        为bootpack.hrb而准备的段
        - 设置 GDT 中的第3个描述符
        - 段的大小: BOOTPACK大小
        - 段的起始地址: BOOTPACK地址
        - 访问权限: 可以被读取和执行
     */
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    /*
        设置到全局段寄存器
        - 段数量
        - 全局段号记录表地址
     */
    load_gdtr(LIMIT_GDT, ADR_GDT);

    /* IDT初始化 */
    for (i = 0; i <= LIMIT_IDT / 8; i++)
    {
        set_gatedesc(idt + i, 0, 0, 0);
    }

    // 中断段数量，记录表地址
    load_idtr(LIMIT_IDT, ADR_IDT);

    /* IDT设置 */
    set_gatedesc(idt + 0x21, (int)asm_inthandler21, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x27, (int)asm_inthandler27, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int)asm_inthandler2c, 2 * 8, AR_INTGATE32);

    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
    return;
}
