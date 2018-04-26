/**
 * ISSUSE
 * 1.fine_next_bit()循环次数过多，每次需要扫描整个位图
 *      solution：参数中加入一个最大扫描距离
*/
#include "stdint.h"
#include "bitmap.h"
#include "string.h"
#include "debug.h"
#include "global.h"

static int64_t find_next_bit(struct bitmap *bitmap, uint32_t offest, uint32_t max_offest);
static inline bool test_bit(struct bitmap *bitmap, uint32_t bit_idx);
static int64_t find_next_zero_bit(struct bitmap *bitmap, uint32_t offest);


/**
 * bitmap_zero --初始化一个位图,清零全部bit位
 * @bitmap: 待初始化位图
*/
void bitmap_zero(struct bitmap* bitmap)
{
    ASSERT(bitmap != NULL && bitmap->bitmap != NULL);
    if (bitmap->bitmap_bytes_len == 0)
    {
        *(bitmap->bitmap) = 0;
    }
    else
    {
        memset(bitmap->bitmap, 0, bitmap->bitmap_bytes_len);
    }
}

/**
 * bitmap_test_bit --测试bit位是否位1
 * @bitmap: 位图
 * @bit_idx: bit位在位图中索引
*/
bool bitmap_test_bit(struct bitmap* bitmap, uint32_t bit_idx)
{
    ASSERT(bitmap != NULL && bitmap->bitmap != NULL);
    ASSERT(bit_idx < (bitmap->bitmap_bytes_len * BIT_PER_LONG));
    return (bitmap->bitmap[BITS_TO_INDEX(bit_idx)] & (BITMAP_MASK << BITS_IN_INDEX(bit_idx)));
}

/**
 * bitmap_set_bit --设置bit位状态
 * @bitmap: 位图
 * @bit_idx: bit位在位图中索引
 * @bit: enum bit_status枚举bit位状态
*/
void bitmap_set_bit(struct bitmap* bitmap, uint32_t bit_idx, enum bit_status bit)
{
    ASSERT(bitmap != NULL && bitmap->bitmap != NULL);
    ASSERT(bit_idx < (bitmap->bitmap_bytes_len * BIT_PER_LONG));
    if (bit)
    {
        bitmap->bitmap[BITS_TO_INDEX(bit_idx)] |= (BITMAP_MASK << BITS_IN_INDEX(bit_idx));
    }
    else
    {
        bitmap->bitmap[BITS_TO_INDEX(bit_idx)] &= ~(BITMAP_MASK << BITS_IN_INDEX(bit_idx));
    }
}

/**
 * bitmap_apply --申请n个连续可用位
 * @bitmap: 位图
 * @nbits: 申请的bit数
 * return: 成功返回bit位索引，失败返回-1
*/
int64_t bitmap_apply(struct bitmap* bitmap, uint32_t nbits)
{
    ASSERT(bitmap != NULL && bitmap->bitmap != NULL);
    /*从头开始找第一个可用位，找不到返回-1*/
    int64_t __base_idx = find_next_zero_bit(bitmap, 0);
    if(__base_idx == -1)
        return -1;
    int64_t __top_idx = 0;
    uint64_t __btmp_bits = bitmap->bitmap_bytes_len * BIT_PER_LONG;

    for (; __base_idx < __btmp_bits || __top_idx < __btmp_bits; )
    {
        /*__top_idx后移,找到下一个已经使用的bit位，比较和__base_idx的距离*/
        __top_idx = find_next_bit(bitmap, __base_idx + 1, nbits - 1);
        /*如果找不到已用位并且位图总bit偏移 - __base_idx数比nbits大，则找到,否则返回-1*/
        if(__top_idx == -1)
        {
            if((__btmp_bits - 1 - __base_idx) >= nbits){
                return __base_idx;
            }
            else{
                return -1;
            }
        }
        /*如果区间 >= nbits,找到*/
        if(__top_idx - __base_idx >= nbits)
            return __base_idx;
        /*__base_idx后移*/
        __base_idx = find_next_zero_bit(bitmap, __top_idx + 1);
    }
    return -1;
}

/**
 * find_next_zero_bit --找到下一个可用位
 * @bitmap: 位图
 * @offest: 起始bit偏移,包括offest本身
 * return: 成功返回bit在位图中偏移，失败返回-1
*/
static int64_t find_next_zero_bit(struct bitmap* bitmap, uint32_t offest)
{
    ASSERT(bitmap != NULL && bitmap->bitmap != NULL);
    ASSERT(offest < (bitmap->bitmap_bytes_len * BIT_PER_LONG));
    do
    {
        if(!test_bit(bitmap, offest))
        {
            return offest;
        }
    } while ((++offest) < (bitmap->bitmap_bytes_len * BIT_PER_LONG));
    return -1;
}

/**
 * find_next_bit --返回下一个已用位
 * @bitmap: 位图
 * @offest: 起始bit偏移,包括offest本身
 * @max_offest: 查找次数
 * return: 成功返回bit在位图中偏移，失败返回-1
*/
static int64_t find_next_bit(struct bitmap* bitmap, uint32_t offest, uint32_t max_offest)
{
    ASSERT(bitmap != NULL && bitmap->bitmap != NULL);
    ASSERT(offest < (bitmap->bitmap_bytes_len * BIT_PER_LONG));
    uint32_t count = 0;
    do
    {
        if (test_bit(bitmap, offest))
        {
            return offest;
        }
    } while ((++offest) < (bitmap->bitmap_bytes_len * BIT_PER_LONG) && ++count < max_offest);
    return -1;
}

/**
 * inline test_bit --返回指定bit位状态
 * @bitmap: 位图
 * @bit_idx: bit在位图中偏移
*/
static inline bool test_bit(struct bitmap* bitmap, uint32_t bit_idx)
{
    return (bitmap->bitmap[BITS_TO_INDEX(bit_idx)] & (BITMAP_MASK << BITS_IN_INDEX(bit_idx)));
}