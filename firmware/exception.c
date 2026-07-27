#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}

extern volatile int Keypad_Event;
extern volatile int Keypad_Col;

static void Keypad_Notify(int col_idx)
{
	if(Keypad_Event) return;						
	Keypad_Col   = col_idx;
	Keypad_Event = 1;
	Macro_Clear_Area(EXTI->IMR, 0xf, KEY_COL_BASE);	
}

extern void IR_Edge_ISR(void);

void EXTI15_10_IRQHandler(void)
{
	if(Macro_Check_Bit_Set(EXTI->PR, 10))		
	{
		EXTI->PR = 0x1 << 10;
		IR_Edge_ISR();
	}

	NVIC_ClearPendingIRQ(40);
}

void EXTI9_5_IRQHandler(void)
{
	if(Macro_Check_Bit_Set(EXTI->PR, 5))	
	{
		EXTI->PR = 0x1 << 5;
		Keypad_Notify(2);
	}

	if(Macro_Check_Bit_Set(EXTI->PR, 6))		
	{
		EXTI->PR = 0x1 << 6;
		Keypad_Notify(3);
	}

	NVIC_ClearPendingIRQ(23);
}

extern volatile int Uart_Data_In;
extern volatile unsigned char Uart_Data;

void EXTI3_IRQHandler(void)		
{
	EXTI->PR = 0x1 << 3;
	NVIC_ClearPendingIRQ(9);
	Keypad_Notify(0);
}

void EXTI4_IRQHandler(void)		
{
	EXTI->PR = 0x1 << 4;
	NVIC_ClearPendingIRQ(10);
	Keypad_Notify(1);
}

extern void Link_RX_ISR(void);

#ifdef BOARD_A
void USART6_IRQHandler(void) { Link_RX_ISR(); NVIC_ClearPendingIRQ(71); }
#endif
#ifdef BOARD_B
void USART1_IRQHandler(void) { Link_RX_ISR(); NVIC_ClearPendingIRQ(37); }
#endif