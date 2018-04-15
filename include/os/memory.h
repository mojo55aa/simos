#ifndef __MM_MEMORY_H
#define __MM_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

struct virtual_addrr{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
} 


#endif