
#define	CHESS_BUZZER	0x01
#define	CHESS_LEDX		0x10
#define	CHESS_LEDY		0x04
#define	CHESS_LEDSTAT	0x40
#define	CHESS_KEYPAD	0x20

#define	CHESS_WHITE		0x01
#define	CHESS_BLACK		0x02
#define	CHESS_CHECK		0x04
#define	CHESS_MATE		0x08

#define	RANK(X)			(X%8)
#define	FILE(X)			(X/8)

//
// H8 evaluation board hardware drivers
//

void InitDrivers(void);

//
// Control LEDs
//
void SetLED(unsigned char LED, unsigned char is_on);

//
// Scan buttons
//
unsigned char ScanButton(unsigned char Button_No);

//
// 16x2 LCD interface
//

void LCD_at(char x, char y);

void LCD_char(char c);

void LCD_str(char *s);

void LCD_clear(void);

// Chess Board defines
void Chess_Beep(void);
void Chess_LedX(char x);
void Chess_LedY(char y);
void Chess_LedStat(char status);
char Chess_ScanBoard(char position,char status);	
void ChessMain(void);

