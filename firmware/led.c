#include "device_driver.h"

#define LED_R	5		// PA5
#define LED_Y	6		// PA6
#define LED_G	7		// PA7

void LED_Init(void)
{
	/* 아래 코드 수정 금지 : Port-A Clock Enable */
	Macro_Set_Bit(RCC->AHB1ENR, 0); 

	// LED를 출력으로 설정하고 초기 OFF
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10);
	Macro_Clear_Bit(GPIOA->OTYPER, 5);
	Macro_Clear_Bit(GPIOA->ODR, 5); 
}


void LED_All_Off(void)
{
	Macro_Clear_Area(GPIOA->ODR, 0x7, 5);
}

void LED_Set(int r, int y, int g)		// 1 = 켜짐 (Active High)
{
	r ? Macro_Set_Bit(GPIOA->ODR,5) : Macro_Clear_Bit(GPIOA->ODR,5);
	y ? Macro_Set_Bit(GPIOA->ODR,6) : Macro_Clear_Bit(GPIOA->ODR,6);
	g ? Macro_Set_Bit(GPIOA->ODR,7) : Macro_Clear_Bit(GPIOA->ODR,7);
}

void LED_Blink(int mask, int cnt, int ms)
{
	LED_All_Off();
	for(int i = 0; i < cnt; i++)
	{
		Macro_Set_Area(GPIOA->ODR, mask, 5);	// On
		TIM5_Delay(ms);
		Macro_Clear_Area(GPIOA->ODR, 0x7, 5);	// Off
		TIM5_Delay(ms);
	}
}

void LED_Progress(int n)
{
	LED_All_Off();
	if(n > 3) n = 3;
	for(int i = 0; i < n; i++)
		Macro_Set_Bit(GPIOA->ODR, 5 + i);
}