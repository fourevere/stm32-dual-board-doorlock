#include "device_driver.h"

volatile int Keypad_Event = 0;
volatile int Keypad_Col   = -1;

const char KEYMAP[4][4] =
{
	{'1','2','3','A'},
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};
static const unsigned char ROW_PIN[4] = {0, 1, 4, 8};

void Keypad_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 0);		// GPIOA
	Macro_Set_Bit(RCC->AHB1ENR, 1);		// GPIOB

	for(int i = 0; i < 4; i++)
	{
		Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, ROW_PIN[i]*2);
		Macro_Set_Bit(GPIOA->OTYPER, ROW_PIN[i]);
		Macro_Clear_Bit(GPIOA->ODR, ROW_PIN[i]);
	}

	for(int i = 0; i < 4; i++)
	{
		Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, (KEY_COL_BASE+i)*2);
		Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, (KEY_COL_BASE+i)*2);
	}
}

void Keypad_ISR_Enable(int en)
{
	if(en)
	{
		Macro_Set_Bit(RCC->APB2ENR, 14);					// SYSCFG

		Macro_Write_Block(SYSCFG->EXTICR[0], 0xf, 0x1, 12);	// PB3 -> EXTI3
		Macro_Write_Block(SYSCFG->EXTICR[1], 0xf, 0x1, 0);	// PB4 -> EXTI4
		Macro_Write_Block(SYSCFG->EXTICR[1], 0xf, 0x1, 4);	// PB5 -> EXTI5
		Macro_Write_Block(SYSCFG->EXTICR[1], 0xf, 0x1, 8);	// PB6 -> EXTI6

		Macro_Set_Bit(EXTI->FTSR, 3);
		Macro_Set_Bit(EXTI->FTSR, 4);
		Macro_Set_Bit(EXTI->FTSR, 5);
		Macro_Set_Bit(EXTI->FTSR, 6);

		EXTI->PR = (0xf << KEY_COL_BASE);

		NVIC_ClearPendingIRQ((IRQn_Type)9);
		NVIC_ClearPendingIRQ((IRQn_Type)10);
		NVIC_ClearPendingIRQ((IRQn_Type)23);

		Macro_Set_Area(EXTI->IMR, 0xf, KEY_COL_BASE);

		NVIC_EnableIRQ((IRQn_Type)9);	// EXTI3
		NVIC_EnableIRQ((IRQn_Type)10);	// EXTI4
		NVIC_EnableIRQ((IRQn_Type)23);	// EXTI9_5
	}
	else
	{
		Macro_Clear_Area(EXTI->IMR, 0xf, KEY_COL_BASE);
		NVIC_DisableIRQ((IRQn_Type)9);
		NVIC_DisableIRQ((IRQn_Type)10);
	}
}

char Keypad_Scan(int col)
{
	char key = 0;

	for(int r = 0; r < 4; r++)
	{
		for(int i = 0; i < 4; i++)					
			Macro_Set_Bit(GPIOA->ODR, ROW_PIN[i]);

		Macro_Clear_Bit(GPIOA->ODR, ROW_PIN[r]);	
		for(volatile int d = 0; d < 200; d++);

		if(Macro_Check_Bit_Clear(GPIOB->IDR, KEY_COL_BASE + col))
		{
			key = KEYMAP[r][col];
			break;
		}
	}

	for(int i = 0; i < 4; i++)						
		Macro_Clear_Bit(GPIOA->ODR, ROW_PIN[i]);

	return key;
}