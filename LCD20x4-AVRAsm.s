
/*
 * LCD20x4_AVRAsm.s
 *
 * Created: 11/15/2015 1:57:35 PM
 *  Author: Sean Igo
 */

 // LCD20x4-AVRAsm.s
// assembly implementation of AVR LCD stuff.
// Assumes Tiny861, LCD data on top 4 pins of port.

#include <avr/io.h>

#include "LCD20x4-AVR.h"		//may change to LCD20x4-AVRAsm.h?

//TextLCD_DelayAsm takes a 16-bit number and counts down from that to
//So, hibyte in r24, lobyte in r25? Delay of 0 = delay of 65536.
//Trashes r24 and r25.
//Timing is about 4 clocks * passed-in number, so 250 would be about 1 millisecond
//CURRENTLY ASSUMING 1MHZ!!!!!!!!!!
//actually now 8MHz - so do 32 clocks * passed in number (buncha nops).
.global TextLCD_DelayAsm
TextLCD_DelayAsm:					//3 for call
/* uncomment for 8MHz
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
	nop								//1
*/
	sbiw	r24,1					//2
	brne	TextLCD_DelayAsm		//1/2
	ret								//4

//void TextLCD_SetDataPins(uint8_t data) data in r24.
//trashes r24
//assumes a 4-bit value in the lower 4 bits of parameter passed in. But masks, so if there's stuff
//the top 4 bits that's ok.
//current implementation 17 cycles incl call/ret. 18 cycles + busy if shift is 2 or 3. 16 if 0.
.global TextLCD_SetDataPinsAsm
TextLCD_SetDataPinsAsm:								//3 for call
	push	r18										//2

	//it looks like this is screwing up because when I read PORT and OR it with the data
	//then write it back, RS line is getting cleared - in short, non-data lines are getting
	//messed with. Should I be reading PIN instead?
	in		r18, _SFR_IO_ADDR(LCD_DATA_PORT)		//1 read PORT ... to preserve non-lcd pin vals.

	// THIS IS THE BIT WHERE DATA BIT POSITION IS ASSUMED!
	// at this point, PORT value assumed to be in r18.
	// could handle all the different possible values as #if blocks switched on
	// all possible shift values, 0..4, and just yode it. First get it working.
	andi	r18, ~LCD_DATA_MASK		//was 0x0F		//1 mask off data bits - define-driven

	// so with the masks, this is the only line that's position dependent...
	// not bad, worst case si 2 cycles!
	// TEST THIS STUFF!!!!!!!!!!!!!!!! conditional comp appears to work, verify running later.
#if	LCD_DATA_SHIFT == 0
	//don't need to do anything!
#elif	LCD_DATA_SHIFT == 1
	lsl		r24										//1 get data into bits 1..4
#elif	LCD_DATA_SHIFT == 2
	lsl		r24										//1
	lsl		r24										//1 get data into bits 2..5
#elif	LCD_DATA_SHIFT == 3
	swap	r24										//1 data in 4..7
	lsr		r24										//1 then shift to 3..6
#elif	LCD_DATA_SHIFT == 4
	swap	r24										//1 get data bits in 4..7
#endif

	andi	r24, LCD_DATA_MASK		//was 0xF0		//1 keep only data bits

	or		r18, r24								//1 construct new port value
	out		_SFR_IO_ADDR(LCD_DATA_PORT), r18		//1 and write it! - OOPS HAD R24 HERE BAD!!

	pop		r18										//2
	ret												//4


//TextLCD_PulseE just raises and lowers E line - put in a sub to allow for timing tweaks.
//current implementation 13 cycles.
TextLCD_PulseE:										//3 for call

	sbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2 raise E
	nop												//1 TIMING NOP - MIGHT MAKE A SPEED DEFINE
	nop												//1 TIMING NOP - MIGHT MAKE A SPEED DEFINE
	cbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2 lower E
	ret												//4

//TextLCD_BusyAsm - will later have unidirectional version which just delays, I imagine
//trashes r24
//ASSUMES DATA BITS ARE TOP 4 IN PORT!!!!!!! Fixed with defines.
//about 60 cycles in best case.
.global TextLCD_BusyAsm
TextLCD_BusyAsm:									//3 for call
#ifdef DEBUG_LCD
	ret
#endif
	push	r18										//2

	clr		r24										//1
	rcall	TextLCD_SetDataPinsAsm					//17 (currently) prep pins for input, no pullup
	in		r18, _SFR_IO_ADDR(LCD_DATA_DIR)			//1 get current direction settings

	andi	r18, ~LCD_DATA_MASK						//1 construct new data direction (top 4 now = input)
	out		_SFR_IO_ADDR(LCD_DATA_DIR), r18			//1 and make pins inputs

	cbi 	_SFR_IO_ADDR(LCD_RS_PORT), LCD_RS_BIT	//2 lower RS line (command)
	nop
	sbi 	_SFR_IO_ADDR(LCD_RW_PORT), LCD_RW_BIT	//2 raise RW line (read)

TextLCD_BusyAsmLoop:
	nop												//1 timing nop - LATER MAKE SPEED DEFINE?
	nop												//1 timing nop - LATER MAKE SPEED DEFINE?

	sbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2 raise E line

	//FOLLOWING ADDED AS EXPERIMENT
	nop												//1 timing nop - LATER MAKE SPEED DEFINE?

	in		r18, _SFR_IO_ADDR(LCD_DATA_PIN)			//1 read upper nybble (whole port into r18)
	nop												//1 timing nop - LATER MAKE SPEED DEFINE?
	cbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2 lower E line

	nop												//1 timing nop - LATER MAKE SPEED DEFINE?
	nop												//1 timing nop - LATER MAKE SPEED DEFINE?

	sbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2 raise E line
	nop												//1 timing nop - LATER MAKE SPEED DEFINE? dummy read
	nop												//1 timing nop - LATER MAKE SPEED DEFINE?
	cbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2 lower E line

	sbrc	r18, LCD_BUSY_BIT						//1/2/3 check for busy bit - if clear, done!
	rjmp	TextLCD_BusyAsmLoop						//2

	cbi 	_SFR_IO_ADDR(LCD_RW_PORT), LCD_RW_BIT	//2 lower RW line (read)

	in		r18, _SFR_IO_ADDR(LCD_DATA_DIR)			//1 get current direction settings
	ori		r18, LCD_DATA_MASK						//1 construct new data direction (top 4 now = output)
	out		_SFR_IO_ADDR(LCD_DATA_DIR), r18			//1 and make pins outputs

	pop		r18									//2
	ret											//4

//void TextLCD_Cmd(uint8_t cmd) - argument will be in r24, yes?
//trashes r24
//currently 76 cycles + busy call
.global TextLCD_CmdAsm
TextLCD_CmdAsm:										//3 for call
	push	r18										//2

	mov		r18,r24									//1 save off copy of data
	swap	r24										//1 get upper nybble in lower 4
	rcall	TextLCD_SetDataPinsAsm					//17 (currently).
	cbi 	_SFR_IO_ADDR(LCD_RS_PORT), LCD_RS_BIT	//2 lower RS line (command)
	rcall	TextLCD_PulseE							//13 (currently)

	mov		r24,r18									//1 restore original data
	rcall	TextLCD_SetDataPinsAsm					//17 (currently). Send low nybble
	rcall	TextLCD_PulseE							//13 (currently)

	rcall	TextLCD_BusyAsm							//varies

	pop		r18										//2
	ret												//4

//void TextLCD_Char(uint8_t cmd) - argument will be in r24, yes?
//trashes r24 (charD 25 as well)
//currently Char = 76 cycles + busy call, CharD = 78 + busy call
.global TextLCD_CharDAsm
TextLCD_CharDAsm:									//3 for call
	ldi		r25,0x30								//1 add 0x30 to given byte = ascii numeral char
	add		r24,r25									//1

.global TextLCD_CharAsm
TextLCD_CharAsm:									//3 for call
	push	r18										//2

	mov		r18,r24									//1 save off copy of data
	swap	r24										//1 get upper nybble in lower 4
	rcall	TextLCD_SetDataPinsAsm					//17 (currently).
	sbi 	_SFR_IO_ADDR(LCD_RS_PORT), LCD_RS_BIT	//2 raise RS line (data)
	rcall	TextLCD_PulseE							//13 (currently)

	mov		r24,r18									//1 restore original data
	rcall	TextLCD_SetDataPinsAsm					//17 (currently). Send low nybble
	rcall	TextLCD_PulseE							//13 (currently)

	rcall	TextLCD_BusyAsm							//varies

	pop		r18										//2
	ret												//4

//void TextLCD_PortInitAsm();			//sets up I/O pins for LCD - trashes r24
//uses LCD_DATA_MASK, so should be compatible with any setup for that.
.global TextLCD_PortInitAsm
TextLCD_PortInitAsm:
	//so: all LCD-related pins should start out as outputs.
	//might as well zero them while we're there.
	//actually I think I'll do all the port-sets-to-0 first, then the directions.
	clr		r24										//1
	rcall	TextLCD_SetDataPinsAsm					//17 (currently)
	in		r18, _SFR_IO_ADDR(LCD_DATA_DIR)			//1 get current direction settings
	ori		r18, LCD_DATA_MASK						//1 construct new data direction (top 4 now = output)
	out		_SFR_IO_ADDR(LCD_DATA_DIR), r18			//1 and make pins outputs

	//then set the other pins individually.
	cbi 	_SFR_IO_ADDR(LCD_RS_PORT), LCD_RS_BIT	//2
	sbi 	_SFR_IO_ADDR(LCD_RS_DIR), LCD_RS_BIT	//2

	cbi 	_SFR_IO_ADDR(LCD_RW_PORT), LCD_RW_BIT	//2
	sbi 	_SFR_IO_ADDR(LCD_RW_DIR), LCD_RW_BIT	//2

	cbi 	_SFR_IO_ADDR(LCD_E_PORT), LCD_E_BIT		//2
	sbi 	_SFR_IO_ADDR(LCD_E_DIR), LCD_E_BIT		//2
	ret												//4


//void TextLCD_ClrAsm();				//clears LCD display - trashes r24
.global TextLCD_ClrAsm
TextLCD_ClrAsm:
	ldi		r24, 0x01
	rcall	TextLCD_CmdAsm
	ret

//void TextLCD_InitAsm();				//initializes the LCD controller itself
//trashes r24,r25
.global TextLCD_InitAsm
TextLCD_InitAsm:

	rcall	TextLCD_BusyAsm				//wait for LCD to settle

	ldi		r24,0x20					//set 4-bit mode
	rcall	TextLCD_CmdAsm
	ldi		r24,0x20					//experiment - do it a few times to make sure it sticks.
	rcall	TextLCD_CmdAsm				//recommended by Ilyett article.
	ldi		r24,0x20
	rcall	TextLCD_CmdAsm

	ldi		r24,0x08					//shut off display
	rcall	TextLCD_CmdAsm

	ldi		r24,0x28					//Set display shift
	rcall	TextLCD_CmdAsm

	ldi		r24,0x06					//Set display character mode
	rcall	TextLCD_CmdAsm

	ldi		r24,0x0C					//Set display on/off and cursor command - cursor off
	rcall	TextLCD_CmdAsm

	rcall	TextLCD_ClrAsm				//clear display

										//just in case, might help...delay of 16000 as currently imped
										//= about 64 millis, which is as long as a clear is supposed
										//to take in the worst case. 16000 = 0x3e80
										// actually that seems to be too short - not sure. Let's
										// try 25000 (61A8) - then swh without any delay.
	//ldi		r25,0x61					//hibyte
	//ldi		r24,0xA8					//lobyte
	//rcall	TextLCD_DelayAsm

	ret
