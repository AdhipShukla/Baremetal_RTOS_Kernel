#ifndef __TIMEBASE_H__
#define __TIMEBASE_H__
#include <stdint.h>
void tim2_1MS_tick_init();
uint32_t get_tick(void);
void delay(uint32_t delay);
void timebase_init(void);
#endif
