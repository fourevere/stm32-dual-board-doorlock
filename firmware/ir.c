#include "device_driver.h"

#define IR_PIN		10		// PC10

volatile unsigned int IR_Data  = 0;
volatile int          IR_Ready = 0;

static volatile unsigned int ir_buf = 0;
static volatile int          ir_cnt = -1;		

void IR_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 2);				
	Macro_Set_Bit(RCC->APB2ENR, 14);			

	Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, IR_PIN*2);	
	Macro_Write_Block(GPIOC->PUPDR, 0x3, 0x1, IR_PIN*2);	
	Macro_Write_Block(SYSCFG->EXTICR[2], 0xf, 0x2, 8);

	Macro_Set_Bit(EXTI->FTSR, IR_PIN);		
	Macro_Clear_Bit(EXTI->RTSR, IR_PIN);
	EXTI->PR = 0x1 << IR_PIN;

	TIM2_IR_Start();

	Macro_Set_Bit(EXTI->IMR, IR_PIN);
	NVIC_EnableIRQ((IRQn_Type)40);				
}

void IR_Edge_ISR(void)			
{
	unsigned int gap = TIM2->CNT;				
	TIM2->CNT = 0;

	if(gap > 12000 && gap < 15000)				
	{
		ir_buf = 0;
		ir_cnt = 0;
	}
	else if(ir_cnt >= 0 && ir_cnt < 32)
	{
		ir_buf >>= 1;

		if(gap > 1800 && gap < 2700)			
			ir_buf |= 0x80000000;
		else if(!(gap > 800 && gap < 1500))		
		{
			ir_cnt = -1;
			return;
		}

		if(++ir_cnt == 32)
		{
			if(!IR_Ready)						
			{
				IR_Data  = ir_buf;
				IR_Ready = 1;
			}
			ir_cnt = -1;
		}
	}
}