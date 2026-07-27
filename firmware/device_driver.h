#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

#define KEY_ROW_BASE   12        // PA0~PA3 = R1~R4
#define KEY_COL_BASE   3        // PB3~PB6 = C1~C4

// Uart.c
extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);

// Led.c

extern void LED_Init(void);
extern void LED_Set(int r, int y, int g);
extern void LED_Blink(int mask, int cnt, int ms);
extern void LED_Progress(int n);

// Clock.c

extern void Clock_Init(void);

// Key.c
extern void Keypad_ISR_Enable(int en);
extern void Keypad_Init(void);
extern char Keypad_Scan(int col);

// Timer.c

extern void TIM3_Servo_Init(void);
extern void TIM3_Servo_Set_Angle(int angle);
extern void TIM4_Delay(int ms);
extern void TIM2_IR_Start(void);
extern void TIM1_Out_Init(void);
extern void TIM1_Out_Freq_Generation(unsigned short freq);
extern void TIM1_Out_Stop(void);
extern void TIM5_Delay(int ms);

//lcd.c

extern void LCD_Init(void);
extern void LCD_Clear(void);
extern void LCD_Char(char c);
extern void LCD_Str(const char *s);
extern void LCD_Goto(int row, int col);

// IR.c
extern void IR_Init(void);
extern volatile unsigned int IR_Data;
extern volatile int IR_Ready;

// RFID.c
extern void RFID_Init(void);
extern int  RFID_Read_UID(unsigned char *uid);	

// uart_link.c
extern void Link_Init(int baud);
extern void Link_SendPacket(unsigned char cmd, const unsigned char *data, unsigned char len);
extern void Link_RX_ISR(void);
extern volatile unsigned char Link_Cmd;
extern volatile unsigned char Link_Data[8];
extern volatile int Link_Packet_Ready;
