#ifndef __RTOSKERNEL_H
#define __RTOSKERNEL_H
#define PERIODIC_TASK_PERIOD	10000
#define PERIODIC_TASK			1
#define SR_UIF					(1U<<0)
#include "stm32h747xx.h"
void rtosKernelInit();
void rtosKernelLaunch(uint32_t cycleQuanta);
uint8_t rtosKernelAddThread(void(*thread0)(void), void(*thread1)(void), void(*thread2)(void));
void rtosThreadYield();
void task3();
void tim2_1hz_interrupt_init();
void rtosSempahoreInit(int32_t *semaphoreCnt, int32_t initVal);
void rtosSemaphoreCntGive(int32_t *semaphoreCnt);
void rtosSemaphoreCntTake(int32_t *semaphoreCnt);
#endif
