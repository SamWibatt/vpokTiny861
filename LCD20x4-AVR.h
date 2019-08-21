/*
 * LCD20x4_AVR.h
 * - header file for HD44780 text display routines *
 * Created: 11/15/2015 1:55:21 PM
 *  Author: Sean
 */ 


#ifndef LCD20X4_AVR_H_
#define LCD20X4_AVR_H_


//***********************************************************************************************
//***********************************************************************************************
//***********************************************************************************************
//DEBUG DEFINE - uncomment to debug LCD stuff in simulator
//#define DEBUG_LCD
//***********************************************************************************************
//***********************************************************************************************
//***********************************************************************************************

// RS, R/W, and E lines can be on any port and any bit.
// NOTE: later when I have a unidirectional compile flag, the R/W pin will not be used.

// *********** I SHOULD TRY A FEW DIFFERENT SHIFT VALUES TO BE SURE EVERYTHING WORKS AS EXPECTED.

#define LCD_DATA_PORT	PORTB
#define LCD_DATA_DIR	DDRB
#define LCD_DATA_PIN	PINB
#define LCD_DATA_SHIFT	(0)
#define LCD_DATA_MASK	(0x0F << LCD_DATA_SHIFT)
#define LCD_BUSY_BIT	(3 + LCD_DATA_SHIFT)	// LCD bit 7 as read; see TextLCD_BusyAsm

// function pin setup currently like this.
// 17 = PA3 ------------ (output) ----- LCD E line
// 18 = PA2 ------------ (output) ----- LCD RS line
// 19 = PA1 ------------ (output) ----- LCD R/W line

#define LCD_RS_PORT	PORTB
#define LCD_RS_DIR	DDRB
#define LCD_RS_PIN	PINB
#define LCD_RS_BIT	(4)

#define LCD_RW_PORT	PORTB
#define LCD_RW_DIR	DDRB
#define LCD_RW_PIN	PINB
#define LCD_RW_BIT	(5)

#define LCD_E_PORT	PORTB
#define LCD_E_DIR	DDRB
#define LCD_E_PIN	PINB
#define LCD_E_BIT	(6)

// lcd cursor positions INCL command flag, so just load one up & call lcd_cmd to set position.
// Currently they're for a 4x20 LCD, can change for other layouts.
#define lcdpos_line1	0x80
#define lcdpos_line2	0xC0
#define lcdpos_line3	0x94
#define lcdpos_line4	0xD4

#ifdef __ASSEMBLER__

#else	//__ASSEMBLER__

//assembly versions!
void TextLCD_PortInitAsm();			//sets up I/O pins for LCD
void TextLCD_InitAsm();				//initializes the LCD controller itself
void TextLCD_CmdAsm(uint8_t cmd);
void TextLCD_CharDAsm(uint8_t num);	//convenience for printing number-characters, e.g. send 0, prints '0'
void TextLCD_CharAsm(uint8_t c);
void TextLCD_ClrAsm();				//Clear display
void TextLCD_DelayAsm(uint16_t i);	//delay, currently ca. 4 clocks * given number

#endif	//__ASSEMBLER__

#endif /* LCD20X4-AVR_H_ */