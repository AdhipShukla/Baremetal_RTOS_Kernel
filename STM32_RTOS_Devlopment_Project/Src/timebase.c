#include "timebase.h"
#include "stm32h747xx.h"
#include "rtoskernel.h"
//#define QUARTER_SEC_LOAD	16000000 //Clock speed is 64MHz
//#define CTRL_ENABLE			(1U<<0)
//#define CTRL_TICKINT		(1U<<1)
//#define CTRL_CLKSRC			(1U<<2)
//#define CTRL_COUNTFLAG		(1U<<16)
#define MAX_DELAY			0XFFFFFFFFU //Largest 32 bit number
#define TIM2EN					(1U<<0)
#define CR1_CEN					(1U<<0)
#define DIER_UIE				(1U<<0)

volatile uint32_t global_curr_tick;
volatile uint32_t global_curr_tick_prime;
volatile uint32_t tick_freq = 1;
uint32_t cntTask4;

void delay(uint32_t delay){
	uint32_t tickstart = get_tick();
	uint32_t wait = delay;
	if(wait<MAX_DELAY){
		wait += tick_freq;
	}
	while((get_tick()-tickstart)<wait){}
}

uint32_t get_tick(void){
	__disable_irq();
	global_curr_tick_prime = global_curr_tick; //Fetch the current value of global counter
	__enable_irq();
	return global_curr_tick_prime;
}

void tick_increment(void){
	global_curr_tick += tick_freq;
}

void tim2_1MS_tick_init(){
	//Enable clock access to timer 2
	RCC->APB1LENR |= TIM2EN;
	//Set timer prescalar
	TIM2->PSC = 6400-1; // Bus clock is 64000000 dividing by prescalar to get 10000 HZ
	//Set auto reload value
	TIM2->ARR = 10-1; //1000/1000 = 1Hz
	//Clear Counter
	TIM2->CNT = 0;
	//Enable timer 2
	TIM2->CR1 = CR1_CEN;
	//Enable interrupt
	TIM2->DIER = DIER_UIE;
	//Enable the timer interrupt in NVIC
	NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(){
	//Clear the interrupt flag
	TIM2->SR &= ~SR_UIF;
	cntTask4++;
	tick_increment();
}
