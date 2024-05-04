#include "uart.h"
#include "stm32h747xx.h"

#define GPIOAEN				   	(1U<<0)
#define SYS_FREQ				64000000
#define APB1_CLK				SYS_FREQ
#define UART_BAUD_RATE			115200
static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
static void uart_write(int ch);

int __io_putchar(int ch){
	uart_write(ch);
	return ch;
}
void uart_tx_init(void){

	RCC->AHB4ENR |= GPIOAEN; //Starting clock for GPIOA as using pin PA9 AND PA10 for USART

	//Setting the mode as
	GPIOA->MODER &= ~(1U<<18);
	GPIOA->MODER |= (1U<<19);

	/*GPIOA->MODER &= ~(1U<<20);
	GPIOA->MODER |= (1U<<21);*/

	//Setting GPIO Alternate function for PA9 AND PA10
	GPIOA->AFR[1] |= ((1U<<4)|(1U<<5)|(1U<<6));
	GPIOA->AFR[0] &= ~(1U<<7);

	/*GPIOA->AFR[1] |= ((1U<<8)|(1U<<9)|(1U<<10));
	GPIOA->AFR[0] &= ~(1U<<11);*/

	//Enabling clock for USART1
	RCC->APB2ENR |= (1<<4);

	//Setting USART1 Baud Rate
	uart_set_baudrate(APB1_CLK, UART_BAUD_RATE);

	//Configuring Transfer Direction
	USART1->CR1 = (1U<<3);

	//Enable the USART Module
	USART1->CR1 |= (1U<<0);
}

static void uart_write(int ch){
	//Make sure data register is not full
	while(!(USART1->ISR & (1U<<7))){}

	//Write to data transmit register
	USART1->TDR = (ch & 0xFF);

}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate){
	USART1->BRR = (periph_clk + (baudrate/2U)) / baudrate;
}

