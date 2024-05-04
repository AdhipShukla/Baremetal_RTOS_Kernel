#ifndef __RTOSKERNEL_H
#define __RTOSKERNEL_H
#define SR_UIF					(1U<<0)
#include "stm32h747xx.h"
void rtosKernelClkInit();
void rtosKernelLaunch(uint32_t cycleQuanta);
//uint8_t rtosKernelAddThread(void(*thread0)(void), void(*thread1)(void), void(*thread2)(void));
uint8_t rtosKernelAddThread(void(*threadFunc)(void), uint32_t threadID);
void rtosThreadYield();
void task3();
void rtosSempahoreInit(int32_t *semaphoreCnt, int32_t initVal);
void rtosSemaphoreCntGive(int32_t *semaphoreCnt);
void rtosSemaphoreCntTake(int32_t *semaphoreCnt);
#endif
