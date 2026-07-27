#include "device_driver.h"
#include <stdio.h>

#define RC_SS		2		
#define RC_RST		1		
#define RC_IRQ		12		

#define CommandReg		0x01
#define ComIEnReg		0x02
#define DivIEnReg		0x03
#define ComIrqReg		0x04
#define ErrorReg		0x06
#define FIFODataReg		0x09
#define FIFOLevelReg	0x0A
#define ControlReg		0x0C
#define BitFramingReg	0x0D
#define ModeReg			0x11
#define TxControlReg	0x14
#define TxASKReg		0x15
#define TModeReg		0x2A
#define TPrescalerReg	0x2B
#define TReloadRegH		0x2C
#define TReloadRegL		0x2D
#define VersionReg		0x37

#define PCD_IDLE		0x00
#define PCD_TRANSCEIVE	0x0C
#define PCD_RESETPHASE	0x0F
#define PICC_REQIDL		0x26
#define PICC_ANTICOLL	0x93

volatile int RFID_Irq = 0;
static int  RC_Comm(unsigned char*, unsigned char, unsigned char*, unsigned int*);

static void SPI2_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 1);	
	Macro_Set_Bit(RCC->APB1ENR, 14);	

	for(int p = 13; p <= 15; p++)	
	{
		Macro_Write_Block(GPIOB->MODER,  0x3, 0x2, p*2);
		Macro_Write_Block(GPIOB->AFR[1], 0xf, 0x5, (p-8)*4);
	}

	Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, RC_SS*2);
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, RC_RST*2);
	Macro_Set_Bit(GPIOB->ODR, RC_SS);
	Macro_Set_Bit(GPIOB->ODR, RC_RST);

	SPI2->CR1 = (1<<9)|(1<<8)|(1<<2)|(0x4<<3);
	Macro_Set_Bit(SPI2->CR1, 6);
}

static unsigned char SPI2_Byte(unsigned char d)
{
	while(Macro_Check_Bit_Clear(SPI2->SR, 1));
	SPI2->DR = d;
	while(Macro_Check_Bit_Clear(SPI2->SR, 0));
	return (unsigned char)SPI2->DR;
}

static void RC_Write(unsigned char addr, unsigned char val)
{
	Macro_Clear_Bit(GPIOB->ODR, RC_SS);
	SPI2_Byte((addr << 1) & 0x7E);
	SPI2_Byte(val);
	Macro_Set_Bit(GPIOB->ODR, RC_SS);
}

static unsigned char RC_Read(unsigned char addr)
{
	unsigned char v;
	Macro_Clear_Bit(GPIOB->ODR, RC_SS);
	SPI2_Byte(((addr << 1) & 0x7E) | 0x80);
	v = SPI2_Byte(0x00);
	Macro_Set_Bit(GPIOB->ODR, RC_SS);
	return v;
}

static void RC_SetBit(unsigned char reg, unsigned char mask)
{
	RC_Write(reg, RC_Read(reg) | mask);
}

static void RC_ClearBit(unsigned char reg, unsigned char mask)
{
	RC_Write(reg, RC_Read(reg) & (~mask));
}

void RFID_Init(void)
{
	SPI2_Init();

	Macro_Clear_Bit(GPIOB->ODR, RC_RST);
	TIM5_Delay(10);
	Macro_Set_Bit(GPIOB->ODR, RC_RST);
	TIM5_Delay(50);

	RC_Write(CommandReg, PCD_RESETPHASE);
	TIM5_Delay(50);

	RC_Write(TModeReg,      0x8D);
	RC_Write(TPrescalerReg, 0x3E);
	RC_Write(TReloadRegL,   30);
	RC_Write(TReloadRegH,   0);
	RC_Write(TxASKReg,      0x40);
	RC_Write(ModeReg,       0x3D);

	if(!(RC_Read(TxControlReg) & 0x03))
		RC_SetBit(TxControlReg, 0x03);

	printf("RC522 Ver = 0x%02X\n", RC_Read(VersionReg));
}

static int RC_Comm(unsigned char *sendBuf, unsigned char sendLen,
                   unsigned char *backBuf, unsigned int *backBits)
{
	unsigned char n, lastBits;
	unsigned int  i = 2000;
	int status = 0;

	RC_Write(ComIEnReg, 0x77);
	RC_ClearBit(ComIrqReg, 0x80);
	RC_SetBit(FIFOLevelReg, 0x80);
	RC_Write(CommandReg, PCD_IDLE);

	for(int k = 0; k < sendLen; k++) RC_Write(FIFODataReg, sendBuf[k]);

	RC_Write(CommandReg, PCD_TRANSCEIVE);
	RC_SetBit(BitFramingReg, 0x80);

	do {
		n = RC_Read(ComIrqReg);
		i--;
	} while(i && !(n & 0x01) && !(n & 0x30));

	RC_ClearBit(BitFramingReg, 0x80);

	if(i == 0) return 0;
	if(RC_Read(ErrorReg) & 0x1B) return 0;

	if(n & 0x30)
	{
		n = RC_Read(FIFOLevelReg);
		lastBits = RC_Read(ControlReg) & 0x07;
		*backBits = lastBits ? (n-1)*8 + lastBits : n*8;
		if(n == 0) n = 1;
		if(n > 16) n = 16;
		for(int k = 0; k < n; k++) backBuf[k] = RC_Read(FIFODataReg);
		status = 1;
	}
	return status;
}

int RFID_Read_UID(unsigned char *uid)
{
	unsigned char buf[16];
	unsigned int  bits;

	RC_Write(BitFramingReg, 0x07);
	buf[0] = PICC_REQIDL;
	if(!RC_Comm(buf, 1, buf, &bits)) return 0;
	if(bits != 0x10) return 0;

	RC_Write(BitFramingReg, 0x00);
	buf[0] = PICC_ANTICOLL;
	buf[1] = 0x20;
	if(!RC_Comm(buf, 2, buf, &bits)) return 0;

	unsigned char chk = 0;
	for(int i = 0; i < 4; i++) { uid[i] = buf[i]; chk ^= buf[i]; }
	if(chk != buf[4]) return 0;

	return 1;
}



