#include "device_driver.h"

#ifdef BOARD_A
	#define LINK	USART6
#else
	#define LINK	USART1
#endif

#define STX	0x02
#define ETX	0x03

volatile unsigned char Link_Cmd = 0;
volatile unsigned char Link_Data[8];
volatile unsigned char Link_Len = 0;
volatile int           Link_Packet_Ready = 0;

static unsigned char rx_buf[16];
static int rx_state = 0;		
static int rx_idx = 0;
static unsigned char rx_chk = 0;

void Link_Init(int baud)
{
#ifdef BOARD_A
	Macro_Set_Bit(RCC->AHB1ENR, 2);				
	Macro_Set_Bit(RCC->APB2ENR, 5);				
	Macro_Write_Block(GPIOC->MODER,  0x3, 0x2, 6*2);
	Macro_Write_Block(GPIOC->MODER,  0x3, 0x2, 7*2);
	Macro_Write_Block(GPIOC->AFR[0], 0xf, 0x8, 6*4);
	Macro_Write_Block(GPIOC->AFR[0], 0xf, 0x8, 7*4);
#else
	Macro_Set_Bit(RCC->AHB1ENR, 0);				// GPIOA
	Macro_Set_Bit(RCC->APB2ENR, 4);				// USART1
	Macro_Write_Block(GPIOA->MODER,  0x3, 0x2, 9*2);
	Macro_Write_Block(GPIOA->MODER,  0x3, 0x2, 10*2);
	Macro_Write_Block(GPIOA->AFR[1], 0xf, 0x7, (9-8)*4);
	Macro_Write_Block(GPIOA->AFR[1], 0xf, 0x7, (10-8)*4);
#endif

	LINK->BRR = (unsigned int)(84000000.0/baud + 0.5);
	LINK->CR1 = (1<<13)|(1<<3)|(1<<2)|(1<<5);

#ifdef BOARD_A
	NVIC_EnableIRQ((IRQn_Type)71);
#else
	NVIC_EnableIRQ((IRQn_Type)37);
#endif
}

static void Link_SendByte(unsigned char b)
{
	while(Macro_Check_Bit_Clear(LINK->SR, 7));
	LINK->DR = b;
}

void Link_SendPacket(unsigned char cmd, const unsigned char *data, unsigned char len)
{
	unsigned char chk = cmd ^ len;

	Link_SendByte(STX);
	Link_SendByte(cmd);
	Link_SendByte(len);
	for(int i = 0; i < len; i++)
	{
		Link_SendByte(data[i]);
		chk ^= data[i];
	}
	Link_SendByte(chk);
	Link_SendByte(ETX);
}

void Link_RX_ISR(void)
{
	unsigned char b = (unsigned char)LINK->DR;

	switch(rx_state)
	{
		case 0:	
			if(b == STX) { rx_state = 1; rx_chk = 0; }
			break;

		case 1:	
			rx_buf[0] = b;
			rx_chk = b;
			rx_state = 2;
			break;

		case 2:	
			rx_buf[1] = b;
			rx_chk ^= b;
			rx_idx = 0;
			rx_state = (b > 0) ? 3 : 4;		
			break;

		case 3:	
			rx_buf[2 + rx_idx] = b;
			rx_chk ^= b;
			if(++rx_idx >= rx_buf[1]) rx_state = 4;
			break;

		case 4:	
			if(b == rx_chk) rx_state = 5;
			else            rx_state = 0;	
			break;

		case 5:	
			if(b == ETX && !Link_Packet_Ready)
			{
				Link_Cmd = rx_buf[0];
				Link_Len = rx_buf[1];
				for(int i = 0; i < Link_Len; i++)
					Link_Data[i] = rx_buf[2 + i];
				Link_Packet_Ready = 1;
			}
			rx_state = 0;
			break;
	}
}