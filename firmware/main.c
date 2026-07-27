#include "device_driver.h"
#include <stdio.h>

#define CMD_ENTER		0x01
#define CMD_ADMIN		0x02
#define CMD_PWCHG		0x03
#define CMD_PW_OK		0x04
#define CMD_PW_FAIL		0x05
#define CMD_PW_INPUT	0x06
#define CMD_IDLE		0x07
#define CMD_LOCK		0x10
#define CMD_UNLOCK		0x11

#define PW_LEN	3
#ifdef BOARD_A
// 2026-09-12 publication copy: demonstration value, original secret omitted.
static char PASSWORD[PW_LEN] = {'0','0','0'};

#define CARD_CNT	2
// 2026-09-12 publication copy: example UIDs; enroll your own local cards.
static const unsigned char CARDS[CARD_CNT][4] = {
	{0xDE, 0xAD, 0xBE, 0xEF}, // administrator example
	{0x01, 0x23, 0x45, 0x67}  // user example
};
#define ADMIN_IDX	0
#endif

#define IR_KEY_1	0xF30CFF00
#define IR_KEY_2	0xE718FF00

extern volatile int Keypad_Event;
extern volatile int Keypad_Col;

static void Sys_Init(int baud)
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2);
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}

static void LED3_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 0);
	for(int i = 5; i <= 7; i++)
	{
		Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, i*2);
		Macro_Clear_Bit(GPIOA->OTYPER, i);
		Macro_Clear_Bit(GPIOA->ODR, i);
	}
}

#ifdef BOARD_A

enum key {C1,C1_,D1,D1_,E1,F1,F1_,G1,G1_,A1,A1_,B1,
          C2,C2_,D2,D2_,E2,F2,F2_,G2,G2_,A2,A2_,B2};

static void Buzzer_Beep(unsigned char tone, int duration)
{
	const static unsigned short tv[] =
		{261,277,293,311,329,349,369,391,415,440,466,493,
		 523,554,587,622,659,698,739,783,830,880,932,987};
	TIM1_Out_Freq_Generation(tv[tone]);
	TIM4_Delay(duration);
	TIM1_Out_Stop();
}
static void Buzzer_Key(void)   { Buzzer_Beep(C2, 40); }
static void Buzzer_OK(void)    { Buzzer_Beep(C2,100); Buzzer_Beep(E2,100); Buzzer_Beep(G2,200); }
static void Buzzer_Fail(void)  { Buzzer_Beep(G1,150); Buzzer_Beep(D1,300); }
static void Buzzer_Admin(void) { Buzzer_Beep(G2,80); Buzzer_Beep(C2,80); Buzzer_Beep(G2,80); }

static int Card_Check(unsigned char *uid)
{
	for(int c = 0; c < CARD_CNT; c++)
	{
		int m = 1;
		for(int i = 0; i < 4; i++) if(CARDS[c][i] != uid[i]) m = 0;
		if(m) return c;
	}
	return -1;
}

typedef enum { ST_IDLE, ST_ADMIN_NEW1, ST_ADMIN_NEW2 } State;
static State state = ST_IDLE;
static char input[PW_LEN];
static char newpw[PW_LEN];
static int  idx = 0;
static int  fail = 0;

static void Send_Input(int mode, int n)
{
	unsigned char d[2] = { (unsigned char)mode, (unsigned char)n };
	Link_SendPacket(CMD_PW_INPUT, d, 2);
}

static void Go_Idle(void)
{
	state = ST_IDLE; idx = 0;
	LED_Set(0,0,0);
	Link_SendPacket(CMD_IDLE, 0, 0);	
}

static void Handle_Key(char key)
{
	if(state == ST_IDLE)
	{
		if(key == '*') { idx = 0; Buzzer_Key(); Send_Input(0, 0); }
		else if(key == '#')
		{
			int ok = (idx == PW_LEN);
			for(int i=0;i<idx;i++) if(input[i]!=PASSWORD[i]) ok=0;

			if(ok)
			{
				printf("[A] PW OK\n");
				LED_Set(0,0,1); Buzzer_OK();
				TIM3_Servo_Set_Angle(90);
				Link_SendPacket(CMD_PW_OK, 0, 0);
				fail = 0;
				for(volatile int d=0;d<20000000;d++);
				TIM3_Servo_Set_Angle(0);
				Go_Idle();
			}
			else
			{
				fail++;
				printf("[A] Wrong (%d)\n", fail);
				Buzzer_Fail(); LED_Blink(0x1, 3, 200);
				unsigned char f = fail;
				Link_SendPacket(CMD_PW_FAIL, &f, 1);
				if(fail >= 3) fail = 0;
				Go_Idle();
			}
			idx = 0;
		}
		else if(idx < PW_LEN)
		{
			input[idx++] = key;
			Buzzer_Key(); LED_Progress(idx);
			Send_Input(0, idx);			
		}
		return;
	}

	if(state == ST_ADMIN_NEW1)
	{
		if(key == '*')
		{
			if(idx == 0) { Buzzer_Fail(); Go_Idle(); }
			else { idx = 0; Send_Input(1, 0); }
		}
		else if(key == '#')
		{
			if(idx == PW_LEN) { state = ST_ADMIN_NEW2; idx = 0; Buzzer_Key(); Send_Input(2, 0); }
			else Buzzer_Fail();
		}
		else if(idx < PW_LEN)
		{
			newpw[idx++] = key;
			Buzzer_Key(); LED_Progress(idx);
			Send_Input(1, idx);
		}
		return;
	}

	if(state == ST_ADMIN_NEW2)
	{
		if(key == '*') { idx = 0; Send_Input(2, 0); }
		else if(key == '#')
		{
			int match = (idx == PW_LEN);
			for(int i=0;i<idx;i++) if(input[i]!=newpw[i]) match=0;

			if(match)
			{
				for(int i=0;i<PW_LEN;i++) PASSWORD[i]=newpw[i];
				printf("[A] PW changed\n");
				LED_Set(0,0,1); Buzzer_OK();
				Link_SendPacket(CMD_PWCHG, (unsigned char*)PASSWORD, PW_LEN);
			}
			else { Buzzer_Fail(); }
			for(volatile int d=0;d<15000000;d++);
			Go_Idle();
		}
		else if(idx < PW_LEN)
		{
			input[idx++] = key;
			Buzzer_Key(); LED_Progress(idx);
			Send_Input(2, idx);
		}
		return;
	}
}

void Main(void)
{
	Sys_Init(115200);
	Keypad_Init();
	LED3_Init();
	TIM3_Servo_Init();
	TIM1_Out_Init();
	RFID_Init();
	Link_Init(9600);
	Keypad_ISR_Enable(1);

	printf("\n[A] DOOR \n");
	TIM3_Servo_Set_Angle(0);
	Go_Idle();

	EXTI->PR = (0xf << KEY_COL_BASE);
	Keypad_Event = 0; Keypad_Col = -1;

	int rfid_tick = 0;

	for(;;)
	{
		if(++rfid_tick >= 3000)
		{
			rfid_tick = 0;
			unsigned char uid[4];
			if(RFID_Read_UID(uid) && state == ST_IDLE)
			{
				int c = Card_Check(uid);
				if(c == ADMIN_IDX)
				{
					printf("[A] ADMIN card\n");
					state = ST_ADMIN_NEW1; idx = 0;
					LED_Set(0,1,0); Buzzer_Admin();
					Link_SendPacket(CMD_ADMIN, uid, 4);
				}
				else if(c >= 0)
				{
					printf("[A] card OK\n");
					LED_Set(0,0,1); Buzzer_OK();
					TIM3_Servo_Set_Angle(90);
					Link_SendPacket(CMD_ENTER, uid, 4);
					for(volatile int d=0;d<20000000;d++);
					TIM3_Servo_Set_Angle(0);
					Go_Idle();
				}
			}
		}

		if(Keypad_Event)
		{
			if(Keypad_Col >= 0 && Keypad_Col <= 3)
			{
				int col = Keypad_Col;
				char k = Keypad_Scan(col);
				if(k) Handle_Key(k);
				while(Macro_Check_Bit_Clear(GPIOB->IDR, KEY_COL_BASE + col));
			}
			EXTI->PR = (0xf << KEY_COL_BASE);
			Macro_Set_Area(EXTI->IMR, 0xf, KEY_COL_BASE);
			Keypad_Event = 0;
		}

		if(Link_Packet_Ready)
		{
			if(Link_Cmd == CMD_LOCK)   { TIM3_Servo_Set_Angle(0);  printf("[A] remote LOCK\n"); }
			if(Link_Cmd == CMD_UNLOCK) { TIM3_Servo_Set_Angle(90); printf("[A] remote UNLOCK\n"); }
			Link_Packet_Ready = 0;
		}
	}
}
#endif
#ifdef BOARD_B

static void LCD_Show(const char *l1, const char *l2)
{
	LCD_Clear();
	LCD_Goto(0,0); LCD_Str(l1);
	LCD_Goto(1,0); LCD_Str(l2);
}

static char IR_To_Key(unsigned int code)
{
	if(code == IR_KEY_1) return '1';
	if(code == IR_KEY_2) return '2';
	return 0;
}

static void Show_Input(int mode, int n)
{
	const char *l1;
	const char *lb;
	if(mode == 0)      { l1 = "* DOOR LOCK *"; lb = "PW  : "; }
	else if(mode == 1) { l1 = "ADMIN: SET PW"; lb = "New : "; }
	else               { l1 = "ADMIN:CONFIRM"; lb = "Again: "; }

	LCD_Clear();
	LCD_Goto(0,0); LCD_Str(l1);
	LCD_Goto(1,0); LCD_Str(lb);
	for(int i=0;i<PW_LEN;i++) LCD_Char(i<n ? '*' : '_');
}

void Main(void)
{
	Sys_Init(115200);
	LED3_Init();
	LCD_Init();
	IR_Init();
	Link_Init(9600);

	printf("\n[B] CONSOLE (admin)\n");
	LED_Set(0,0,1);
	LCD_Show("* DOOR LOCK *", "PW Please...");

	for(;;)
	{
		if(Link_Packet_Ready)
		{
			unsigned char cmd = Link_Cmd;

			if(cmd == CMD_PW_INPUT)
			{
				Show_Input(Link_Data[0], Link_Data[1]);
			}
			else if(cmd == CMD_IDLE)
			{
				LED_Set(0,0,1);
				LCD_Show("* DOOR LOCK *", "PW Please...");
			}
			else if(cmd == CMD_ENTER)
			{
				printf("[B] ENTER %02X%02X%02X%02X\n",
				       Link_Data[0],Link_Data[1],Link_Data[2],Link_Data[3]);
				LED_Set(0,1,0);
				LCD_Show("  ENTRY OK", " Card User");
			}
			else if(cmd == CMD_ADMIN)
			{
				printf("[B] ADMIN card\n");
				LCD_Show(" ADMIN CARD", "Set new PW...");
			}
			else if(cmd == CMD_PWCHG)
			{
				printf("[B] PW->%c%c%c\n", Link_Data[0],Link_Data[1],Link_Data[2]);
				LCD_Show(" PW CHANGED!", " ");
			}
			else if(cmd == CMD_PW_OK)
			{
				printf("[B] PW OK\n");
				LED_Set(0,1,0);
				LCD_Show("  ENTRY OK", " Password");
			}
			else if(cmd == CMD_PW_FAIL)
			{
				printf("[B] PW FAIL %d\n", Link_Data[0]);
				LCD_Show("  WRONG PW", "  check PW ");
				for(volatile int d=0;d<12000000;d++);
				LCD_Show("* DOOR LOCK *", "PW Please...");
				if(Link_Data[0] >= 3) LED_Set(0,0,0);
			}

			Link_Packet_Ready = 0;
		}

		if(IR_Ready)
		{
			char k = IR_To_Key(IR_Data);
			if(k == '1') { Link_SendPacket(CMD_UNLOCK, 0, 0); LCD_Show("REMOTE","UNLOCK"); }
			if(k == '2') { Link_SendPacket(CMD_LOCK,   0, 0); LCD_Show("REMOTE","LOCK"); }
			IR_Ready = 0;
		}
	}
}
#endif
