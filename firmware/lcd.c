#include "device_driver.h"

#define LCD_ADDR	(0x27 << 1)		
#define LCD_BL		0x08			
#define LCD_EN		0x04
#define LCD_RS		0x01

static void I2C_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 1);		
	Macro_Set_Bit(RCC->APB1ENR, 21);	

	
	for(int p = 8; p <= 9; p++)
	{
		Macro_Write_Block(GPIOB->MODER,  0x3, 0x2, p*2);	
		Macro_Set_Bit(GPIOB->OTYPER, p);					
		Macro_Write_Block(GPIOB->PUPDR,  0x3, 0x1, p*2);	
		Macro_Write_Block(GPIOB->AFR[1], 0xf, 0x4, (p-8)*4);
	}

	Macro_Clear_Bit(I2C1->CR1, 0);		
	I2C1->CR2   = 42;					
	I2C1->CCR   = 210;					
	I2C1->TRISE = 43;
	Macro_Set_Bit(I2C1->CR1, 0);		
}

static void I2C_Send(unsigned char data)
{
	while(Macro_Check_Bit_Set(I2C1->SR2, 1));		

	Macro_Set_Bit(I2C1->CR1, 8);					
	while(Macro_Check_Bit_Clear(I2C1->SR1, 0));		

	I2C1->DR = LCD_ADDR;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 1));		
	(void)I2C1->SR2;								

	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));		
	I2C1->DR = data;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));		

	Macro_Set_Bit(I2C1->CR1, 9);					
}

static void L_Delay(int us)
{
	for(volatile int i = 0; i < us * 12; i++);
}

static void L_Nibble(unsigned char n, unsigned char rs)
{
	unsigned char d = (n & 0xf0) | LCD_BL | rs;

	I2C_Send(d | LCD_EN);
	L_Delay(2);
	I2C_Send(d & ~LCD_EN);
	L_Delay(60);
}

static void L_Write(unsigned char val, unsigned char rs)
{
	L_Nibble(val & 0xf0, rs);
	L_Nibble(val << 4,   rs);
}

void LCD_Cmd(unsigned char cmd)
{
	L_Write(cmd, 0);
	if(cmd == 0x01 || cmd == 0x02) TIM5_Delay(2);
}

void LCD_Char(char c)
{
	L_Write((unsigned char)c, LCD_RS);
}

void LCD_Str(const char *s)
{
	while(*s) LCD_Char(*s++);
}

void LCD_Goto(int row, int col)
{
	LCD_Cmd(0x80 | (row ? 0x40 : 0x00) | col);
}

void LCD_Clear(void)
{
	LCD_Cmd(0x01);
}

void LCD_Init(void)
{
	I2C_Init();
	TIM5_Delay(50);

	L_Nibble(0x30, 0); TIM5_Delay(5);
	L_Nibble(0x30, 0); TIM5_Delay(1);
	L_Nibble(0x30, 0); TIM5_Delay(1);
	L_Nibble(0x20, 0); TIM5_Delay(1);	

	LCD_Cmd(0x28);		
	LCD_Cmd(0x08);		
	LCD_Cmd(0x01);		
	TIM5_Delay(2);
	LCD_Cmd(0x06);		
	LCD_Cmd(0x0C);		
}