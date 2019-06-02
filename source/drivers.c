

//
// H8/3687 evaluation board hardware drivers
//

//-----------------Include Common Header Files--------------------------
#include <machine.h>
#include "iodefine.h"
#include "edk3687def.h"

#include "drivers.h"

#define LCD_RS 1
#define LCD_RW 2
#define LCD_E  3

// todo. use proper hardware timers

void delay5ms(void)
{
	long ulDelay;

 	  for(ulDelay=0; ulDelay<400000; ulDelay++);
}

void delay1us(void)
{
	int i;

	for(i=0;i<1000;i++);

}

//
// Wait for the LCD busy flag to be clear
//
void LCD_wait_BF(void)
{
	unsigned char x;

	P_PORT.PCR3.BYTE = 0xF;			// Data bus is input

	P_PORT.PDR3.BYTE = 1<<LCD_RW;	// Select R/W=1, RS = 0, E = 0

	delay1us();

	do 
	{

	P_PORT.PDR3.BYTE = (1<<LCD_RW)|(1<<LCD_E);

	delay1us();

	x=P_PORT.PDR3.BYTE ;

	P_PORT.PDR3.BYTE = 1<<LCD_RW;	// Select R/W=1, RS = 0, E = 0

	delay1us();

	} while(x & 128);

}

// write nibble

void LCD_write(unsigned char RS, unsigned char x)
{
	
	P_PORT.PCR3.BYTE = 0xFF;		// Data bus is output

	// R/W=0, load data, RS
	P_PORT.PDR3.BYTE = (RS<<LCD_RS) | ((x << 4) & 0xF0);
	 
	delay1us();

	P_PORT.PDR3.BYTE |= 1<<LCD_E;

	delay1us();

	P_PORT.PDR3.BYTE &= ~(1<<LCD_E);

	delay1us();
}

void LCD_write8(unsigned char RS,unsigned char x)
{

	LCD_write(RS,x >> 4);
	LCD_write(RS,x & 15);

	LCD_wait_BF();

}

void LCD_init(void)
{

	delay5ms();
	delay5ms();
	delay5ms();

	LCD_write(0,3);
	
	delay5ms();

	LCD_write(0,3);
	
	delay5ms();
	
	LCD_write(0, 3);
	
	LCD_write(0,2);

	LCD_wait_BF();

	LCD_write8(0, 32+8);	// 2 lines, 5x8 font

	LCD_write8(0, 8);		// display mode off

	LCD_write8(0, 1);		// clear

	LCD_write8(0, 4+2);		// entry mode advance
		
	LCD_write8(0, 8+4);		// display mode on
}

void LCD_at(char x, char y)
{
	LCD_write8(0, 128+(x&63)+((y&1)<<6));
}

void LCD_char(char x)
{
	LCD_write8(1, x);
}

void LCD_str(char *s)
{
	while(*s)
		LCD_char(*s++);
}

void LCD_clear(void)
{	
	LCD_write8(0, 1);
}

static const unsigned char mask[8] = { 1,2,4,8,16,32,64,128 };

//
// Control LEDs
//
void SetLED(unsigned char LED, unsigned char is_on)
{

	if(is_on)
		P_PORT.PDR6.BYTE |= mask[LED & 7];
	else
		P_PORT.PDR6.BYTE &= ~mask[LED & 7];

}

//
// Scan a button state. Returns true if pressed.
//
unsigned char ScanButton(unsigned char button)
{
	int i,x;

	x=0;

	for(i=0;i<255;i++)
	{
		if(P_PORT.PDR5.BYTE & mask[button & 15])
			x++;
	}

	if(x > 127)
		return 0;
	else
		return 1;
}


void InitDrivers(void)
{
	P_TMRZ.TFCR.BIT.CMD = 0;

	P_TMRZ.TOCR.BYTE = 0;

	P_TMRZ.TPMR.BYTE = 0;

	P_TMRZ.TOER.BYTE = 0xFF;

    P_PORT.PCR6.BYTE	= 0xFF;		/* Sets the bits in the LED Port DDR to O/P */
    P_PORT.PDR6.BYTE	= 0x00;		/* Sets the bits in the LED Port DR 0x00 */

	P_PORT.PMR5.BYTE = 0;			// Port5 is general purpose I/O
	P_PORT.PCR5.BYTE = 0xF0;		// Port5 lower nibble = input, upper nibble = output

	P_PORT.PUCR5.BYTE = 0xF;		// Enable pullups on switch inputs

	P_PORT.PCR3.BYTE = 0xF;			// LCD control lines are output, data is input
	P_PORT.PDR3.BYTE = 0x0;			// Deselect LCD


	// Chess board additions
	P_PORT.PCR1.BYTE = 0xFF;		// Port 1 set to output
	P_PORT.PDR1.BYTE = 0x00;


	LCD_init();

}

// Chess Board stuff
void Chess_Beep(void)
{
	unsigned long	i;
 
	P_PORT.PDR1.BYTE = CHESS_BUZZER;
	for (i=0; i<600000; i++);
	P_PORT.PDR1.BYTE = 0x00;
	for (i=0; i<600000; i++);
}

#define	LED_BRIGHTNESS	120

void Chess_LedX(char x)
{
	P_PORT.PCR5.BYTE = 0xFF;	
	P_PORT.PUCR5.BYTE = 0x00;
	P_PORT.PDR5.BYTE = x;
	P_PORT.PDR1.BYTE = CHESS_LEDX;
	for (x=0; x<LED_BRIGHTNESS; x++);
	P_PORT.PDR1.BYTE = 0x00;
	for (x=0; x<LED_BRIGHTNESS; x++);
}

void Chess_LedY(char y)
{
	P_PORT.PDR6.BYTE = y;
	P_PORT.PDR1.BYTE = CHESS_LEDY;
	for (y=0; y<LED_BRIGHTNESS; y++);
	P_PORT.PDR1.BYTE = 0x00;
	for (y=0; y<LED_BRIGHTNESS; y++);
}

void Chess_LedStat(char status)
{
	if (status!=-1)
		P_PORT.PDR6.BYTE = status;
	P_PORT.PDR1.BYTE = CHESS_LEDSTAT;
	for (status=0; status<LED_BRIGHTNESS; status++);
	P_PORT.PDR1.BYTE = 0x00;
	for (status=0; status<LED_BRIGHTNESS; status++);
}	

char Chess_ScanBoard(char position,char status)
{
	static	const	char	Mask[] = {1,2,4,8,16,32,64,128};
	char	x;
	char	row,column=1;

	for (;;)
	{
		if (position!=-1)
		{
			Chess_LedX(1<<RANK(position));
			Chess_LedY(1<<FILE(position));
		}
		Chess_LedStat(status);
		P_PORT.PCR6.BYTE = 0x00;
		P_PORT.PDR5.BYTE = column;
		for (x=0; x<LED_BRIGHTNESS; x++)
			row = P_PORT.PDR6.BYTE;
		P_PORT.PCR6.BYTE = 0xFF;
		if (row!=0)
		{
			Chess_Beep();
			break;
		}
		column *= 2;
		if (column==0)
			column = 1;
	}
	// Calculate Row Index
	for (x=0; x<8; x++)
	{
		if (row==Mask[x])
			break;
	}
	row = x;
	// Calculate Column Index
	for (x=0; x<8; x++)
	{
		if (column==Mask[x])
			break;
	}
	column = x;
	return	(8*row+column);
}

	
