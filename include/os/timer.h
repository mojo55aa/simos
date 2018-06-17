#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H
#include "stdint.h"

#define CLOCK_VECTOR 0x20

void mil_sleep(uint32_t m_second);
void timer_init();

#endif