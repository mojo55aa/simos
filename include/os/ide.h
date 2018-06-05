#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "sync.h"
#include "global.h"

/* 分区结构 */
struct partition{
    uint32_t start_lba;                     /* 起始扇区号 */
    uint32_t sector_cnt;                    /* 扇区数 */
    struct disk *_disk;                     /* 分区所属的硬盘 */
    struct kernel_list *_queue;             /* 队列 */
    char _name[8];                          /* 分区名字 */
    struct super_block *_sb;                /* 超级块 */
    struct bitmap block_bitmap;             /* 块位图 */
    struct bitmap inode_bitmap;             /* inode位图 */
    struct general_queue open_inodes_queue; /* 打开的inode队列 */
};

/* 硬盘结构 */
struct disk{
    char _name[8];                      /* 硬盘名字 */
    struct ide_channel *_channel;       /* 硬盘IDE通道 */
    uint8_t dev_no;                     /* 主盘1，从盘0 */
    struct partition prim_parts[4];     /* 主分区最多4个 */
    struct partition logic_parts[8];    /* 逻辑分区最多支持8个 */
    uint8_t prim_cnt;
    uint8_t logic_cnt;
};

/* IDE通道结构 */
struct ide_channel{
    char _name[8];              /* 通道名字 */
    uint16_t port_base;         /* 端口起始地址 */
    uint8_t irq_no;             /* 通道中断号 */
    struct lock _lock;          /* 通道锁 */
    bool wait_intr;             /* 等待硬盘中断标志 */
    struct semaphore disk_ok;   /* 驱动程序的阻塞唤醒 */
    struct disk _devices[2];    /* 一个通道上可以连接一个主盘，一个从盘 */
};

void ide_init();

#endif