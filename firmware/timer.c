#include "device_driver.h"


#define TIM1_FREQ	(8000000)

#define TIM2_TICK         	(20) 				// usec
#define TIM2_FREQ 	  		(1000000/TIM2_TICK)	// Hz
#define TIME2_PLS_OF_1ms  	(1000/TIM2_TICK)
#define TIM2_MAX	  		(0xffffu)

#define TIM3_FREQ 	  			(8000000) 	      	// Hz
#define TIM3_TICK	  			(1000000/TIM3_FREQ)	// usec
#define TIME3_PLS_OF_1ms  		(1000/TIM3_TICK)


#define TIM3_SERVO_TICK		(1)					// usec
#define TIM3_SERVO_FREQ		(1000000/TIM3_SERVO_TICK)
#define SERVO_PERIOD		(20000)				// 20ms
#define SERVO_MIN_US		(500)				// 0도
#define SERVO_MAX_US		(2500)				// 180도

#define TIM4_TICK	  		(20) 				// usec
#define TIM4_FREQ 	  		(1000000/TIM4_TICK) // Hz
#define TIME4_PLS_OF_1ms  	(1000/TIM4_TICK)
#define TIM4_MAX	  		(0xffffu)


void TIM4_Repeat(int time)
{
	Macro_Set_Bit(RCC->APB1ENR, 2);

	TIM4->CR1 = (1<<4)|(0<<3);
	TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5) - 1;
	TIM4->ARR = TIME4_PLS_OF_1ms * time - 1;

	Macro_Set_Bit(TIM4->EGR, 0);
	Macro_Clear_Bit(TIM4->SR, 0);
	Macro_Set_Bit(TIM4->CR1, 0);
}

int TIM4_Check_Timeout(void)
{
	if(Macro_Check_Bit_Set(TIM4->SR, 0))
	{
		Macro_Clear_Bit(TIM4->SR, 0);
		return 1;
	}

	return 0;
}

void TIM4_Stop(void)
{
	Macro_Clear_Bit(TIM4->CR1, 0);
}

// void TIM4_Change_Value(int time)
// {
// 	TIM4->ARR = TIME4_PLS_OF_1ms * time - 1;
// }

// void TIM4_Repeat_Interrupt_Enable(int en, int time)
// {
// 	if(en)
// 	{
// 		Macro_Set_Bit(RCC->APB1ENR, 2);

// 		TIM4->CR1 = (1<<4)|(0<<3);
// 		TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5) - 1;
// 		TIM4->ARR = TIME4_PLS_OF_1ms * time - 1;

// 		Macro_Set_Bit(TIM4->EGR, 0);
// 		Macro_Clear_Bit(TIM4->SR, 0);
// 		NVIC_ClearPendingIRQ(30);
// 		Macro_Set_Bit(TIM4->DIER, 0);
// 		NVIC_EnableIRQ(30);
// 		Macro_Set_Bit(TIM4->CR1, 0);
// 	}
// 	else
// 	{
// 		NVIC_DisableIRQ(30);
// 		Macro_Clear_Bit(TIM4->CR1, 0);
// 		Macro_Clear_Bit(TIM4->DIER, 0);
// 	}
// }


void TIM3_Servo_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 1);		// GPIOB
	Macro_Set_Bit(RCC->APB1ENR, 1);		// TIM3

	Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 0);	
	Macro_Write_Block(GPIOB->AFR[0], 0xf, 0x2, 0);	

	TIM3->PSC = (unsigned int)(TIMXCLK/(double)TIM3_SERVO_FREQ + 0.5) - 1;
	TIM3->ARR = SERVO_PERIOD - 1;

	Macro_Write_Block(TIM3->CCMR2, 0xff, 0x68, 0);	
	TIM3->CCER = (0<<9)|(1<<8);						
	TIM3->CCR3 = 1500;								

	Macro_Set_Bit(TIM3->EGR, 0);
	TIM3->CR1 = (1<<7)|(1<<4)|(0<<3)|(1<<0);		
}

void TIM3_Servo_Set_Angle(int angle)
{
	if(angle < 0)   angle = 0;
	if(angle > 180) angle = 180;

	TIM3->CCR3 = SERVO_MIN_US
	           + (SERVO_MAX_US - SERVO_MIN_US) * angle / 180;
}

void TIM4_Delay(int ms)
{
	TIM4_Repeat(ms);
	while(!TIM4_Check_Timeout());
	TIM4_Stop();
}

void TIM2_IR_Start(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 0);

	TIM2->CR1 = (1<<7);						
	TIM2->PSC = (unsigned int)(TIMXCLK/1000000.0 + 0.5) - 1;	
	TIM2->ARR = 0xffffffff;					

	Macro_Set_Bit(TIM2->EGR, 0);
	Macro_Set_Bit(TIM2->CR1, 0);
}


void TIM1_Out_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 0);		
	Macro_Set_Bit(RCC->APB2ENR, 0);		

	Macro_Write_Block(GPIOA->MODER,  0x3, 0x2, 9*2);		
	Macro_Write_Block(GPIOA->AFR[1], 0xf, 0x1, (9-8)*4);	

	Macro_Write_Block(TIM1->CCMR1, 0xff, 0x68, 8);			
	TIM1->CCER = (1<<4);									
	TIM1->BDTR = (1<<15);									
}

void TIM1_Out_Freq_Generation(unsigned short freq)
{
	TIM1->PSC  = (unsigned int)(TIMXCLK/(double)TIM1_FREQ + 0.5) - 1;
	TIM1->ARR  = (double)TIM1_FREQ/freq - 1;
	TIM1->CCR2 = TIM1->ARR/2;

	Macro_Set_Bit(TIM1->EGR, 0);
	TIM1->CR1 = (1<<7)|(1<<0);
}

void TIM1_Out_Stop(void)
{
	TIM1->CCR2 = 0;
	Macro_Clear_Bit(TIM1->CR1, 0);
}

void TIM5_Delay(int ms)
{
	Macro_Set_Bit(RCC->APB1ENR, 3);		

	TIM5->PSC = (unsigned int)(TIMXCLK/1000000.0 + 0.5) - 1;	
	TIM5->CR1 = (1<<4)|(1<<3);		
	TIM5->ARR = 1000;				

	for(int i = 0; i < ms; i++)
	{
		Macro_Set_Bit(TIM5->EGR, 0);
		Macro_Clear_Bit(TIM5->SR, 0);
		Macro_Set_Bit(TIM5->CR1, 0);
		while(Macro_Check_Bit_Clear(TIM5->SR, 0));
	}
}
