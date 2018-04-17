#ifndef __MM_MEMORY_H
#define __MM_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

struct virtual_mem_pool{
    struct bitmap vm_bitmap;
    uint32_t vm_start;
};


#endif