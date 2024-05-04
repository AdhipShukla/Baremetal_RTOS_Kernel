#include "rtoskernel.h"

#define MAX_NUM_OF_THREADS		10U
#define STACK_SIZE				400U
#define SYS_CLOCK				64000000U //64Mhz
#define CTRL_ENABLE				(1U<<0)
#define CTRL_TICKINT			(1U<<1)
#define CTRL_CLKSRC				(1U<<2)
#define CTRL_COUNTFLAG			(1U<<16)
#define SYSTICK_RST				0U
#define INTCTRL					(*(volatile uint32_t*)(0xE000ED04)) //Interrupt control and state register

uint32_t Millsec_Clock_Cyc;
uint32_t periodTick;
uint32_t threadsCreated=0;
struct threadControlBlock{
	int32_t *ptrStack;
	struct threadControlBlock *nextThread;
};

typedef struct threadControlBlock tcb;

tcb tcbArr[MAX_NUM_OF_THREADS];
tcb *ptrCurrTCB;

int32_t threadStack[MAX_NUM_OF_THREADS][STACK_SIZE];

void rtosSchedulerLaunch();
void rtosSchedulerSwitch();

void rtosKernelThreadStackInit(int idx){
	//Defining the current stack pointer for the particular thread
	tcbArr[idx].ptrStack = &threadStack[idx][STACK_SIZE-16]; //Minus 16 as 16 register are saved in the stack placing the head of the stack at size - 16
	threadStack[idx][STACK_SIZE-1] = (1U<<24); //Program status register value remains at the tail of the stack. Setting 21 bit of PSR to 1 to enable thumb mode

	//Defining other optional registers
	//First all the registers required to save the context
	threadStack[idx][STACK_SIZE-3] = 0xBBBBBBBB; //R14(LR)
	threadStack[idx][STACK_SIZE-4] = 0xBBBBBBBB; //R12
	threadStack[idx][STACK_SIZE-5] = 0xBBBBBBBB; //R3
	threadStack[idx][STACK_SIZE-6] = 0xBBBBBBBB; //R2
	threadStack[idx][STACK_SIZE-7] = 0xBBBBBBBB; //R1
	threadStack[idx][STACK_SIZE-8] = 0xBBBBBBBB; //R0
	//Other core registers in Arm Cortex M4
	threadStack[idx][STACK_SIZE-9] = 0xBBBBBBBB; //R11
	threadStack[idx][STACK_SIZE-10] = 0xBBBBBBBB;//R10
	threadStack[idx][STACK_SIZE-11] = 0xBBBBBBBB;//R9
	threadStack[idx][STACK_SIZE-12] = 0xBBBBBBBB;//R8
	threadStack[idx][STACK_SIZE-13] = 0xBBBBBBBB;//R7
	threadStack[idx][STACK_SIZE-14] = 0xBBBBBBBB;//R6
	threadStack[idx][STACK_SIZE-15] = 0xBBBBBBBB;//R5
	threadStack[idx][STACK_SIZE-16] = 0xBBBBBBBB;//R4
}

void initThreadNext(){
	for(uint32_t i = 0; i< threadsCreated; i++){
		if(i==threadsCreated-1){
			tcbArr[i].nextThread = &tcbArr[0];
		} else {
			tcbArr[i].nextThread = &tcbArr[i+1];
		}
	}
}

/*uint8_t rtosKernelAddThread(void(*thread0)(void), void(*thread1)(void), void(*thread2)(void)){
	__disable_irq(); //Disabling global interrupts
	tcbArr[0].nextThread = &tcbArr[1];
	tcbArr[1].nextThread = &tcbArr[2];
	tcbArr[2].nextThread = &tcbArr[0];

	ptrCurrTCB = &tcbArr[0];

	rtosKernelThreadStackInit(0);
	threadStack[0][STACK_SIZE-2] = (uint32_t)(thread0); //Setting the program counter to function pointer

	rtosKernelThreadStackInit(1);
	threadStack[1][STACK_SIZE-2] = (uint32_t)(thread1); //Setting the program counter to function pointer

	rtosKernelThreadStackInit(2);
	threadStack[2][STACK_SIZE-2] = (uint32_t)(thread2); //Setting the program counter to function pointer

	__enable_irq();
	return 1;
}*/
uint8_t rtosKernelAddThread(void(*threadFunc)(void), uint32_t threadID){
	__disable_irq(); //Disabling global interrupts

	rtosKernelThreadStackInit(threadID);
	threadStack[threadID][STACK_SIZE-2] = (uint32_t)(threadFunc); //Setting the program counter to function pointer

	threadsCreated++;
	initThreadNext();
	__enable_irq();
	return 1;
}

void rtosKernelClkInit(){
	Millsec_Clock_Cyc = (SYS_CLOCK/1000); //Note: Bus speed 64000000 cycles/sec so 64000 cycles in one ms
}

void rtosKernelLaunch(uint32_t cycleQuanta){
	//Reset SysTick
	SysTick->CTRL = SYSTICK_RST;

	//Clear SysTick current value register
	SysTick->VAL = 0;

	//Reload the timer with number of clock cycles to count to zero
	SysTick->LOAD = (cycleQuanta * Millsec_Clock_Cyc) - 1;

	//Setting the priority of systick less than all the other hardware interrupts
	NVIC_SetPriority(SysTick_IRQn, 15);

	//Select internal clock as source
	SysTick->CTRL = CTRL_CLKSRC;

	//Enable SysTick
	SysTick->CTRL |= CTRL_ENABLE;

	//Enable Interrupt
	SysTick->CTRL |= CTRL_TICKINT;

	//Setting first thread 0 as first thread to run
	ptrCurrTCB = &tcbArr[0];

	//Launching Scheduler
	rtosSchedulerLaunch();
}

__attribute__((naked))void SysTick_Handler(){

	//Suspend the current thread
	//First disable global interrupts
	__asm("CPSID	I");
	//Save registers whiCh are not saved when excpetion occured by default
	__asm("PUSH {R4-R11}");
	//Load address of current tcb pointer into R0
	__asm("LDR R0, =ptrCurrTCB"); // R0 is holding pointer to pointer to tcb
	//Load R1 from address equals r0, i.e. R1 = ptrCurrTCB
	__asm("LDR R1, [R0]");
	//Store CortexM SP to memory pointed by R1, i.e. save SP to tcb
	__asm("STR SP, [R1]");


	//Fetching the next thread
	//Load R1 from a location 4 bytes above address R1, i.e. R1 = ptrCurrTCB->next
	__asm("LDR R1,[R1, #4]");
	//Store R1 at address equals R0, i.e ptrCurrTCB = R1
	__asm("STR R1, [R0]");
	//Load Cortex M SP from address equals R1, i.e. SP = ptrCurrTCB->ptrStack
	__asm("LDR SP, [R1]");
	//Restore R4, R5, R6, R7, R8, R9, R10, R11
	__asm("POP {R4-R11}");
	//Enable global interrupt
	__asm("CPSIE	I");
	//Return from exception and automatically restore R0, R1, R2, R3, R12, LR, PC, PSR from newly loaded stack
	__asm("BX	LR");
}

void rtosSchedulerLaunch(){

	//Load address of ptrCurrTCB into R0
	__asm("LDR R0, =ptrCurrTCB");
	//Load ptrCurrTCB to R2 from its address i.e. R0
	__asm("LDR R2, [R0]");
	//Load Cortex M stack pointer from ptrCurrTCB, i.e. SP = ptrCurrTCB->ptrStack
	__asm("LDR SP, [R2]");
	//Fetch all the initial register values from the stack
	__asm("POP {R4-R11}");
	//Fetch other registers which are saved by default
	__asm("POP {R0-R3}");
	//Fetch register 12 which is saved by default
	__asm("POP {R12}");
	//Skip to LR in stack
	__asm("ADD SP, SP, #4");
	//Setting LR as PC which lead to task0 function pointer
	__asm("POP {LR}");
	//Skip to PSR
	__asm("ADD SP, SP, #4");
	//Enable global interrupt
	__asm("CPSIE	I");
	//Return from exception
	__asm("BX	LR");
}

void rtosThreadYield(){
	SysTick->VAL = 0; //By writing any value to this register the value is overwritten
	INTCTRL = (1U<<26);
}

void rtosSempahoreInit(int32_t *semaphoreCnt, int32_t initVal){
	*semaphoreCnt = initVal;
}

void rtosSemaphoreCntGive(int32_t *semaphoreCnt){
	__disable_irq();
	*semaphoreCnt = *semaphoreCnt + 1;
	__enable_irq();
}

void rtosSemaphoreCntTake(int32_t *semaphoreCnt){
	__disable_irq();
	while(*semaphoreCnt <= 0){
		__disable_irq();
		__enable_irq();
		rtosThreadYield();
	}
	*semaphoreCnt = *semaphoreCnt - 1;
	__enable_irq();
}
