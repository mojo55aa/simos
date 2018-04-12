#ifndef __KERNEL_BITMAP_H
#define __KERNEL_BITMAP_H
#include "stdint.h"
#include "global.h"

#define BITMAP_MASK 1
#define BIT_PER_LONG 8  /*位图中一个单位的bit数*/

/*bits在位图中的索引*/
#define BITS_TO_INDEX(nbits) ((nbits) / BIT_PER_LONG)

/*bits位在索引中位置*/
#define BITS_IN_INDEX(nbits) ((nbits) % 8)

/*位图结构体*/
struct bitmap{
    uint32_t bitmap_bytes_len;  /*位图字节数*/
    uint8_t *bitmap;    /*位图数组指针*/
};

/*bit位枚举*/
enum bit_status
{
    bit_clear,      /*清空bit位*/
    bit_set         /*bit位置位*/
};

/*初始化位图*/
void bitmap_zero(struct bitmap *bitmap);
/*判断任意位是否为1*/
bool bitmap_test_bit(struct bitmap *bitmap,uint32_t bit_idx);
/*在位图中连续申请n个可用位*/
int64_t bitmap_applyt(struct bitmap *bitmap, uint32_t nbits);
/*将位图中位置value*/
void bitmap_set_bit(struct bitmap *bitmap, uint32_t bit_idx, enum bit_status);

#endif