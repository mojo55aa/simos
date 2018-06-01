#include "ide.h"
#include "kstdio.h"
#include "stdio.h"
#include "stdint.h"
#include "debug.h"
#include "io.h"
#include "timer.h"
#include "global.h"

/* 硬盘各寄存器的端口号 */
#define reg_data(channel)	    (channel->port_base + 0)
#define reg_error(channel)	    (channel->port_base + 1)
#define reg_sect_cnt(channel)	(channel->port_base + 2)
#define reg_lba_l(channel)	    (channel->port_base + 3)
#define reg_lba_m(channel)	    (channel->port_base + 4)
#define reg_lba_h(channel)	    (channel->port_base + 5)
#define reg_dev(channel)	    (channel->port_base + 6)
#define reg_status(channel)	    (channel->port_base + 7)
#define reg_cmd(channel)	    (reg_status(channel))
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel)	    reg_alt_status(channel)

/* 读操作时状态寄存器的关键位 */
#define BIT_ALT_STATUS_BSY  0x80    /* 硬盘忙 */
#define BIT_ALT_STATUS_DEDR 0x40    /* 设备就绪 */
#define BIT_ALT_STATUS_DRQ  0x08    /* 数据准备就绪 */

/* 写操作时状态寄存器关键位 */
#define BIT_DEV_MBS     0xa0    /* 第7位，第5位固定1 */
#define BIT_DEV_LBA     0x40    /* LBA寻址模式 */
#define BIT_DEV_DEV     0x10    /* 主盘0，从盘1 */

/* 硬盘操作指令 */
#define CMD_IDENTIFY        0xec    /* identify */
#define CMD_READ_SECTOR     0x20    /* 读扇区 */
#define CMD_WRITE_SECTOR    0x10    /* 写扇区 */


#define SECTOR_TO_BYTES(sec_cnt) ((sec_cnt == 0)?(256 * 512):(sec_cnt * 512))


uint8_t g_channel_cnt;  /* 根据硬盘数量计算的通道数 */
struct ide_channel g_channels[2]; /* 两个通道 */


/**********************************************************************************
 *                  硬盘操作流程
 * (1) 选择通道，向通道的sector count寄存器写入操作的扇区数
 * (2) 向通道的三个LBA寄存器写入起始扇区
 * (3) 向device寄存器写入控制字，写入LBA 24~27位
 * (4) 向通道的command寄存器写入操作命令 identify:0xec,read sector:0x20,write sector:0x30
 * (5) 读取通道status寄存器，判断硬盘是否工作完成
 * (6) 写硬盘到此结束，读硬盘此时可以取出数据
 * **********************************************************************************/



/**
 * select_hd --选择使用的硬盘
 * 向硬盘所在的通道device寄存器写入值
 * @hd_disk: 指向初始化后的硬盘的指针
*/
static void disk_select_hd(struct disk* hd_disk)
{
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if(hd_disk->dev_no == 1)
    {
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd_disk->_channel), reg_device);
}

/**
 * select_sector --设置硬盘读写扇区起始地址及扇区数
 * LBA方式device寄存器的低4位放置lba24~27位
 * @hd_disk: 操作的硬盘
 * @lba_base: 起始扇区
 * @sec_cnt: 扇区数
*/
static void disk_select_sector(struct disk* hd_disk, uint32_t lba_base, uint8_t sec_cnt)
{
    /* 写入扇区数 */
    out_b(reg_sect_cnt(hd_disk->_channel), sec_cnt);    /* 如果扇区数0，表示要操作256个扇区
                                                        硬盘每完成一个扇区会将寄存器中的值减1，如果
                                                        操作中断，寄存器中的值表示剩余扇区数 */
    /* 写入扇区起始值 */
    out_b(reg_lba_l(hd_disk->_channel), lba_base >> 0);
    out_b(reg_lba_m(hd_disk->_channel), lba_base >> 8);
    out_b(reg_lba_h(hd_disk->_channel), lba_base >> 16);
    /* LBA第24~27位在device寄存器低4位，重新写入 */
    uint8_t device_data = inb(reg_dev(hd_disk->_channel));
    out_b(reg_dev(hd_disk->_channel), device_data | (lba_base >> 24));
}

/**
 * channel_control_cmd --向通道发送控制命令
 * @_channel: 通道
 * @_cmd: 命令控制字
*/
static void channel_control_cmd(struct ide_channel* _channel, uint8_t _cmd)
{
    /*  向硬盘发送控制命令后硬盘就开始工作，
    设置通道等待状态，表示正在工作，等待中断到来*/
    _channel->wait_intr = TRUE;
    out_b(reg_cmd(_channel), _cmd);
}

/**
 * disk_sector_to_buf --从硬盘中读取sec_cnt个扇区数据到buf中
 * @hd_disk: 硬盘
 * @sec_cnt: 扇区数
 * @buf: 缓冲区地址
*/
static void disk_sector_to_buf(struct disk* hd_disk, uint8_t sec_cnt, void* buf)
{
    /* 计算数据字节数 */
    uint32_t buf_bytes;
    if(sec_cnt == 0)    /* 扇区数0表示256个扇区，一个扇区512字节 */
    {
        buf_bytes = 256 * 512;
    }
    else
    {
        buf_bytes = sec_cnt * 512;
    }
    insw(reg_data(hd_disk->_channel), buf, buf_bytes / 2);
}

/**
 * disk_buf_to_sector --将buf中sec_cnt个扇区的数据写入硬盘
 * @hd_disk: 硬盘
 * @sec_cnt: 扇区数
 * @buf: 指向数据缓冲区的指针
*/
static void disk_buf_to_sector(struct disk* hd_disk, uint8_t sec_cnt, void* buf)
{
    uint32_t buf_bytes;
    buf_bytes = SECTOR_TO_BYTES(sec_cnt);
    outsw(reg_data(hd_disk->_channel), buf, buf_bytes / 2);
}

/**
 * busy_wait --等待硬盘30s
 * 间隔10ms读取硬盘状态，如果设备不忙，数据准备就绪返回TRUE
 * 否则返回FALSE,等待过程中会让出CPU
 * @hd_disk: 硬盘
*/
static bool busy_wait(struct disk* hd_disk)
{
    uint16_t time_limit = 30 * 1000; /* 最多等待30s */
    while((time_limit -= 10) > 0)
    {
        if(!(inb(reg_status(hd_disk->_channel)) & BIT_ALT_STATUS_BSY))
            return (inb(reg_status(hd_disk->_channel)) & BIT_ALT_STATUS_DRQ);
        else
        {
            mil_sleep(10);
        }
    }
    return FALSE;
}

void ide_init()
{
    printk("ide initialization start\n");
    uint8_t hd_cnt = *((uint32_t *)0xc0000475);  /* 硬盘数量被BIOS保存在0x475位置 */
    ASSERT(hd_cnt > 0);
    g_channel_cnt = DIV_ROUND_UP(hd_cnt, 2);
    struct ide_channel *channel;
    uint8_t channel_no = 0;
    // 初始化每个通道上的硬盘
    while(channel_no < g_channel_cnt)
    {
        channel = &g_channels[channel_no];
        sprintf(channel->_name, "ide%d", channel_no);
        /* 每个通道初始化端口基址和中断向量 */
        switch(channel_no)
        {
            case 0:
            {
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;
            }
            case 1:
            {
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15;
                break;
            }
        }
        channel->wait_intr = FALSE;
        lock_init(&channel->_lock);
        /* 信号量初始化为0，驱动程序sema_down信号量后会阻塞进程，直到
        硬盘准备就绪后由中断处理程序唤醒驱动进程，
        期间CPU可以立即做其他事情 */
        sema_init(&channel->disk_ok, 0);
        channel_no++;
    }
    printk("ide initialization complete\n");
}