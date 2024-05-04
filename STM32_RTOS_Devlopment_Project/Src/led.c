#include "led.h"

#define GPIOIEN				   	(1U<<8)
#define LEDA_PIN				(1U<<12)
void led_init(void){
	//Enable LED PI12 Clock
	RCC->AHB4ENR |= GPIOIEN;

	//Set the pin mode as output
	GPIOI->MODER |= (1U<<24);
	GPIOI->MODER &= ~(1U<<25);
}

void led_on(void){
	//Set led PI12 on
	//GPIOI->BSRR |= LEDA_PIN;
	GPIOI->ODR &= ~LEDA_PIN; //Reset to start LED
}

void led_off(void){
	//Set led PI12 off
	GPIOI->ODR |= LEDA_PIN;
	//GPIOI->BSRR |= (1U<<28);
}
