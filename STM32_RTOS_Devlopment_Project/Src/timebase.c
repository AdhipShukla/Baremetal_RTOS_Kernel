#include "timebase.h"
#include "stm32h747xx.h"

#define QUARTER_SEC_LOAD	16000000 //Clock speed is 64MHz
#define CTRL_ENABLE			(1U<<0)
#define CTRL_TICKINT		(1U<<1)
#define CTRL_CLKSRC			(1U<<2)
#define CTRL_COUNTFLAG		(1U<<16)
#define MAX_DELAY			0XFFFFFFFFU //Largest 32 bit number
volatile uint32_t global_curr_tick;
volatile uint32_t global_curr_tick_prime;
volatile uint32_t tick_freq = 1;


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

void timebase_init(void){
	//Reload the timer with number of clock cycles to count to zero
	SysTick->LOAD = QUARTER_SEC_LOAD-1;

	//Clear SysTick current value register
	SysTick->VAL = 0;

	//Select internal clock as source
	SysTick->CTRL = CTRL_CLKSRC;

	//Enable Interrupt
	SysTick->CTRL |= CTRL_TICKINT;

	//Enable SysTick
	SysTick->CTRL |= CTRL_ENABLE;

	//Enable interrupts
	__enable_irq();
}

void SysTick_Handler(void){
	tick_increment(); //Incrementing the global counter
}
