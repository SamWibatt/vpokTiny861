/*
 * Tiny861Poker.c
 *
 * Created: 11/15/2015 1:49:31 PM
 * Author : Sean Igo
 */

#include <avr/io.h>


//AVRTextLCD.c - Test program for HD44780-based text LCD displays.
//started 7/20/08.

// PIN ASSIGNMENTS: So for LCD we need 7 pins, yes? RS, R/W, E, and 4 data bits.
// room enough for them all on either port. BUT! we need 8 buttons, easiest to put them
// on A, and use the 7 non-reset pins on port b as our LCD lines.

//  1 = PB0/MOSI/~OC1A - (output) ----- LCD data bit 4
//  2 = PB1/MISO/OC1A -- (output) ----- LCD data bit 5
//  3 = PB2/SCK/~OC1B -- (output) ----- LCD data bit 6
//  4 = PB3/OC1B ------- (output) ----- LCD data bit 7
//  5 = VCC ------------ (power) ------ vcc
//  6 = GND ------------ (power) ------ gnd
//  7 = PB4/~OC1D ------ (output) ----- LCD RS line
//  8 = PB5/OC1D ------- (output) ----- LCD R/W line
//  9 = PB6 ------------ (output) ----- LCD E line
// 10 = PB7/RESET ------ (input nopu) - don't use, pull up with e.g. 15K (I'll try 10 since it's handy)
// 11 = PA7 ------------ (input w/pu) - button 7 (deal/draw)
// 12 = PA6 ------------ (input w/pu) - button 6
// 13 = PA5 ------------ (input w/pu) - button 5
// 14 = PA4 ------------ (input w/pu) - button 4
// 15 = AVCC ----------- (power) ------ vcc
// 16 = AGND ----------- (power) ------ gnd
// 17 = PA3 ------------ (input w/pu) - button 3
// 18 = PA2 ------------ (input w/pu) - button 2
// 19 = PA1 ------------ (input w/pu) - button 1
// 20 = PA0 ------------ (input w/pu) - button 0

// LCD wiring (assuming backlit ones from ebay):
//  1 = Vss --- gnd
//  2 = Vcc --- +5
//  3 = Vlc --- wiper of pot b/t +5 and Gnd
//  4 = RS ---- pin 7 of mcu
//  5 = R/W --- pin 8 of mcu
//  6 = E ----- pin 9 of mcu
//  7 = D0 ---- nc
//  8 = D1 ---- nc
//  9 = D2 ---- nc
// 10 = D3 ---- nc
// 11 = D4 ---- pin 1 of mcu
// 12 = D5 ---- pin 2 of mcu
// 13 = D6 ---- pin 3 of mcu
// 14 = D7 ---- pin 4 of mcu
// 15 = VB+ --- nc for now
// 16 = VB- --- nc for now
//
// pins are at top of display, pin 1 nearest the corner.


// y'know, a tiny861 could probably run vpok - if I put the LCD on the 7 usable pins of port B
// and the buttons on the 8 pins of port A. Hm hm hm. That would be a nice little projjie to put
// on AVRFreaks! And of course on Uncle Sean page.
// Just out of curious - how nasty is the LCRNG? It's easy to code in C!


//INCLUDES --------------------------------------------------------------------------------------------------
#include <avr/io.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "LCD20x4-AVR.h"
#include "AVRVpok.h"

//TYPEDEFS --------------------------------------------------------------------------------------------------

//DEFINES ---------------------------------------------------------------------------------------------------



#define FOREVER (6477)



//PROTOTYPES ------------------------------------------------------------------------------------------------

void ReadButtons();

uint32_t rand();

//GLOBALS ---------------------------------------------------------------------------------------------------
//use static for pretty much everything, tho look into why. Or maybe not - doesn't seem to like
//static and extern.
//use volatile for variables accessed from interrupts.

//button vars
uint8_t buttonState;
uint8_t previousButtonState;
uint8_t buttonEdge;

//RNG var
uint32_t nextrand;

//hand variables
uint8_t hand[10];			//10 cards = 5 in hand (0-4) and 5 to draw from (5-9).
uint8_t rank[5];
uint8_t suit[5];
uint8_t held[5];			// a bit wasteful - 5 whole bytes for 5 flags? But that's modern times.

//general game variables
uint8_t bet;				//how many coins are bet currently
uint8_t score;				// save-off for current (pat or drawn) score.
uint8_t prevscore;			// previous score - for telling if we need to rerender score.
uint16_t coins;				// number of coins in player's bankroll
uint16_t prevcoins;			// number of coins that were in player's bankroll before the payoff
uint16_t payoff;			// 1-coin payoff for current hand
uint16_t totalpayoff;		// payoff * # coins bet

uint8_t carddelayflag; 		//flag: delay during card drawing?
uint8_t animstate;			//bet animation - state 1 means do it, 0 means don't
uint8_t animct;				//bet animation timing counter

//eeprom access! The "pointer" is an EEPROM address - I assume they start at 0 and go to
//EEPROM_SIZE-1. So, it itself is not in eeprom, it just POINTS there. See the eeprom routines below.
//eeprom will be treated as a ring buffer with eepromPtr pointing to the currently valid record.
uint16_t eepromPtr;
uint16_t eepromRecordNum;

#ifdef TESTSEEDS
uint16_t nextTestSeed;		// if doing the test-seeds RNG, this is the index of the next one to use.
#endif

//INTERRUPTS ------------------------------------------------------------------------------------------------

//None so far!

// DEALING & SCORING ----------------------------------------------------------------------------------------

void dealhand()
{
	uint8_t j,k,foundcard,nucad;

	for(j=0;j<10;j++)		//10 to generate 5 hand cards and 5 draw cards.
	{
		foundcard = 0;	//flag - have we picked a card that isn't already in the hand?

		//pick a card, then scan the hand to see if that card is already in there.
		do
		{
			//stupid simple dealing - pick a number, reject if it's not 0..51
			//use topmost bits because they're more random than lower ones.
			do { nucad = (rand() & 0xFC000000) >> 26; } while(nucad > 51);

			//see if the card is in the hand.
			for(k=0;k<j;k++) if(hand[k] == nucad) break;

			if(k==j)		//aha, it's good
			{
				hand[j] = nucad;
				foundcard = 1;
			}

		} while (!foundcard);
	}

	//and let's say HERE that no cards are currently held.
	for(j=0;j<5;j++) held[j] = 0;
}


uint8_t findhigh(uint8_t rank[5])
{
	uint8_t j, high = 0;

	for(j=0;j<5;j++) if(rank[j] > high) high = rank[j];

	return high;
}

uint8_t findlow(uint8_t rank[5])
{
	uint8_t j, low = 100;

	for(j=0;j<5;j++) if(rank[j] < low) low = rank[j];

	return low;
}

//scorehand scans global hand array and returns hand type:
//0 - worthless
//1 - Jacks or better
//2 - Two pair
//3 - Three of a kind
//4 - Straight
//5 - Flush
//6 - Full House
//7 - Four of a kind
//8 - Straight flush
//9 - Royal flush
//assumes that the hand is populated with legal cards.

uint8_t scorehand()
{
	uint8_t j,k;
	uint8_t flushflag = 0;
	uint8_t straightflag = 0;
	uint8_t numpairs = 0;
	uint8_t highcard = 0,lowcard = 100;
	uint8_t pairrank = 255;

	//boil down to rank & suit.
	for(j=0;j<5;j++)
	{
		rank[j] = hand[j] >> 2;
		suit[j] = hand[j] & 3;
	}

	//then, time to check for primitives!

	//flush checker - see if all the suits are the sam.
	/*
	if(suit[0] == suit[1] &&
		suit[0] == suit[2] &&
		suit[0] == suit[3] &&
		suit[0] == suit[4])
		flushflag = 1;
	*/
	//loopy way seems to take 2 fewer bytes!
	for(j=1;j<5;j++) if(suit[0] != suit[j]) break;
	if(j==5) flushflag = 1;

	//short circuit: if there are any flushes, there can't be any pairs.
	if(!flushflag)
	{
		//pair detector - check ab ac ad ae bc bd be cd ce de - unroll
		//aside from detecting pairs, assign pair rank. Note that pairrank
		//will get trampled if there are multiple pairs, but since it's for
		//spotting jacks-or-better it's meaningless unless there is exactly
		//one pair. QED.
		/* unrolled way
		if(rank[0] == rank[1]) { pairrank = rank[0]; numpairs++; }
		if(rank[0] == rank[2]) { pairrank = rank[0]; numpairs++; }
		if(rank[0] == rank[3]) { pairrank = rank[0]; numpairs++; }
		if(rank[0] == rank[4]) { pairrank = rank[0]; numpairs++; }
		if(rank[1] == rank[2]) { pairrank = rank[1]; numpairs++; }
		if(rank[1] == rank[3]) { pairrank = rank[1]; numpairs++; }
		if(rank[1] == rank[4]) { pairrank = rank[1]; numpairs++; }
		if(rank[2] == rank[3]) { pairrank = rank[2]; numpairs++; }
		if(rank[2] == rank[4]) { pairrank = rank[2]; numpairs++; }
		if(rank[3] == rank[4]) { pairrank = rank[3]; numpairs++; }
		*/
		for(j=0;j<4;j++)
		{
			for(k=j+1;k<5;k++)
			{
				if(rank[j] == rank[k]) { pairrank = rank[j]; numpairs++; }
			}
		}
	}

	//straight checker: find highcard and lowcard.
	//short circuit: if there are any pairs, there can be no straight.
	if(!numpairs)
	{
		highcard = findhigh(rank);
		lowcard = findlow(rank);
		if((highcard - lowcard) == 4)
		{
			straightflag = 1;
		}
		else //special ace case: if !straight && lowcard was ace(0), trample any aces with
		{	 //13 and re-find hi & low.
			if(lowcard == 0)
			{
				for(j=0;j<5;j++) if(rank[j] == 0) rank[j] = 13;
			}
			highcard = findhigh(rank);
			lowcard = findlow(rank);
			if((highcard - lowcard) == 4)
			{
				straightflag = 1;
			}
		}
	}


	//ok - time for scoring!
	if(flushflag && straightflag && lowcard == 9) return 9;		//royal flush
	if(flushflag && straightflag) return 8;						//straight flush
	if(numpairs == 6) return 7;									//four da kine
	if(numpairs == 4) return 6;									//full house
	if(flushflag) return 5;										//flush
	if(straightflag) return 4;									//straight
	if(numpairs == 3) return 3;									//3 da kine
	if(numpairs == 2) return 2;									//2 pair

	//jacks or better spotting - will work even if any aces have been
	//trampled by straight finder. Checks for 0 (ace) or anything
	//> 9 (9="10", so >9 = jack, queen, king, straightfinder-massaged ace).
	if(numpairs == 1 &&
		(pairrank == 0 || pairrank > 9)) return 1;				//jaxor better

	return 0;													//WORTHLESS!
}


void drawCards()
{
	uint8_t j,nextDrawCard;

	nextDrawCard = 5;			// next card to be drawn is the first one after the hand.

	for(j=0;j<5;j++)
	{
		if(!held[j])
		{
			hand[j] = hand[nextDrawCard];		// replace non-held card with next draw card
			nextDrawCard++;						// and advance to next drawable card
		}
	}
}

//SUBROUTINES -----------------------------------------------------------------------------------------------

void InitPorts()
{
	//For this project, the lcd port init will handle everything we need for port B.
	TextLCD_PortInitAsm();

	//init non-LCD stuff - in this project's case, just 8 buttons; port A all inputs w/pullup
	DDRA = 0x00;
	PORTA = 0xFF;

	//init button variables - start with all-ones (h/t Dr. Bronner) because buttons are active low.
	buttonState = 0xFF;
	previousButtonState = 0xFF;
	buttonEdge = 0;
}

void InitCustomChars()
{
	uint8_t j;

	TextLCD_CmdAsm(0x40);				//Set CGRAM addr to $40 (cursor may hop to 2nd line)

										//then just unwind data, 8 bytes per char * 8 chars = 64 bytes.
	for(j=0;j<64;j++)					//read from program memory then write as char data.
		TextLCD_CharAsm(pgm_read_byte_near(charData+j));

	TextLCD_CmdAsm(0x80);				//set Display Addr (e.g. cmd 0x80) to get back out of define-char
}

// RNG ------------------------------------------------------------------------------------------------------

//this implementation of 32-bit lcrng is only 104 bytes with no optis! Very impressive. 82 bytes
//in -Os.
uint32_t rand()
{
	nextrand = (1664525 * nextrand) + 1013904223;
	return nextrand;
}

// DRAWING ROUTINES -----------------------------------------------------------------------------------------

void DrawString(PGM_P str)
{
	uint8_t j,chr = 0xff;

	//assume cursor is in position. Here just keep drawing characters until a zero comes up.
	j=0;
	do
	{
		chr = pgm_read_byte_near(str+j);
		if(chr != 0) TextLCD_CharAsm(chr);
		j++;
	} while (chr != 0);
}

void DrawTitleScreen()
{
	/* This doesn't seem to work - but I shouldn't need the clear anyway. Let's swh if I rip it out.
	It works! So I will delete this. Later maybe.
	TextLCD_ClrAsm();
	TextLCD_DelayAsm(5000);					//TEST THING - for some reason the line1 isn't working
											//unless I single-step through. Seems to help.
	//let's try losing that and lengthening the delay in clrasm itself. ...no help.
	*/
	TextLCD_CmdAsm(lcdpos_line1);			//should position cursor at beginning of line 1.
	DrawString(str_splash0);
	//quick custom-char test - drop suit chars in upper row where asteriches were
	TextLCD_CmdAsm(lcdpos_line1);
	TextLCD_CharAsm(0);
	TextLCD_CmdAsm(lcdpos_line1+19);
	TextLCD_CharAsm(0);
	TextLCD_CmdAsm(lcdpos_line1+1);
	TextLCD_CharAsm(1);
	TextLCD_CmdAsm(lcdpos_line1+18);
	TextLCD_CharAsm(1);
	TextLCD_CmdAsm(lcdpos_line1+2);
	TextLCD_CharAsm(2);
	TextLCD_CmdAsm(lcdpos_line1+17);
	TextLCD_CharAsm(2);
	TextLCD_CmdAsm(lcdpos_line1+3);
	TextLCD_CharAsm(3);
	TextLCD_CmdAsm(lcdpos_line1+16);
	TextLCD_CharAsm(3);

	TextLCD_CmdAsm(lcdpos_line2);			//should position cursor at beginning of line 2.
	DrawString(str_splash1);
	TextLCD_CmdAsm(lcdpos_line4);			//should position cursor at beginning of line 4.
	DrawString(str_splash2);
}

//ClearLine, given 1,2,3, or 4, moves the cursor to the beginning of that line, then draws 20 spaces
//to clear it.
void ClearLine(uint8_t line)
{
	uint8_t j;

	switch(line)
	{
	case 1: TextLCD_CmdAsm(lcdpos_line1); break;
	case 2: TextLCD_CmdAsm(lcdpos_line2); break;
	case 3: TextLCD_CmdAsm(lcdpos_line3); break;
	case 4: TextLCD_CmdAsm(lcdpos_line4); break;
	default: return;
	}

	for(j=0;j<20;j++) TextLCD_CharAsm((uint8_t)(' '));
}

// rendercardsDelay calls the delay-n-stir routine some number of times for appropriate card
// delay. Let's try ca. 1/4s = 12 calls to delay-n-stir at about 1/50s each.
// if carddelayflag is 0, returns immediately.
// return value - 0 if the delay went off uneventfully (even if short-circuited by carddelayflag
// being 0), 1 if something happened.
// CURRENTLY ALWAYS RETURNS 0. I think the original intention was to have a button-press
// cause the rest of the hand to be drawn without delays, but it isn't currently implemented
// in the PIC version, and people seem happy with that.
uint8_t rendercardsDelay()
{
	uint8_t j;

	if(!carddelayflag) return 0;

	for(j=0;j<12;j++)					//hardcoded 12 iterations of 1/50 sec = about 1/4 sec
	{
		DelayAndStir();
		ReadButtons();
	}

	return 0;
}

// renderCards does the whole animated drawing of the hand - first, draws "card backs"
// of two checkerboard characters for any cards whose held[] flag is 0, then draws the
// front sides. The effect is a dealing-out then turning-over of the cards. Kind of budjo,
// but what do you expect on a text LCD?

void renderCards()
{
	uint8_t j, cardval;

	carddelayflag = 1;

	//so: draw card backs for any card whose held-flag is 0. I think that's all I have to do.
	for(j=0;j<5;j++)
	{
		if(!held[j])
		{
			TextLCD_CmdAsm(lcdpos_line2 + 1 + (j << 2));	//position is 1 + (4 * card index)

			if(rendercardsDelay()) carddelayflag = 0;		//pause; if cancelled, no further pauses

			TextLCD_CharAsm(0x07);							//draw card back = 2 char 7s
			TextLCD_CharAsm(0x07);
		}
	}

	//then a very similar loop, only this time, draw the FRONTS of the non-held cards.
	for(j=0;j<5;j++)
	{
		if(!held[j])
		{
			TextLCD_CmdAsm(lcdpos_line2 + 1 + (j << 2));	//position is 1 + (4 * card index)

			if(rendercardsDelay()) carddelayflag = 0;		//pause; if cancelled, no further pauses

			//draw rank (look up character corresponding to rank which = card value >> 2)
			cardval = hand[j];

			TextLCD_CharAsm(pgm_read_byte_near(rankChars + (cardval >> 2)));

			//draw suit; suit character value is just card value & 0x03
			TextLCD_CharAsm(cardval & 0x03);
		}
	}

	rendercardsDelay();			//one final delay so the rest of the screen doesn't show up right
								//when the last card is drawn.
}

//renderheld draws the current hold state.
void renderHeld()
{
	uint8_t j;

	for(j=0;j<5;j++)
	{
		//get new cursor position - put cursor at line 3, column 1 + (4*card#).
		TextLCD_CmdAsm(lcdpos_line3 + 1 + (j<<2));

		if(held[j])
		{
			TextLCD_CharAsm(5);		//draw "HE" character (character 5)
			TextLCD_CharAsm(6);		//draw "LD" character (character 6)
		}
		else
		{
			TextLCD_CharAsm((uint8_t)(' '));	//draw 2 spaces
			TextLCD_CharAsm((uint8_t)(' '));
		}
	}
}

// renderscore renders the string for the hand's score, up at the top of the screen, centered.
// DOES NOT CLEAR THE TOP LINE - caller will have to do that for the drawn hand score.
// assumes global var "score" contains a legal and correct score.

void renderScore()
{
	uint8_t column = pgm_read_byte_near(scoreCenterColumn + score);
	TextLCD_CmdAsm(lcdpos_line1 + column);
	//DrawString(scorestrs[score]);
	uint16_t straddr = pgm_read_word_near(scorestrs+score);
	DrawString((PGM_P)straddr);
}

//renderNumber is a routine to draw the given number with no leading zeroes.
//assumes cursor is in position where the number should be drawn.
void renderNumber(uint16_t num)
{
	uint8_t digit;
	uint8_t foundnonzero = 0;

	digit = (uint8_t)(num / 10000);
	if(foundnonzero || digit)
	{
		TextLCD_CharDAsm(digit);			//draw 10K digit, if nonzero
		foundnonzero = 1;
	}
	digit = (uint8_t)((num / 1000) % 10);
	if(foundnonzero || digit)
	{
		TextLCD_CharDAsm(digit);			//draw thousands digit, if nonzero
		foundnonzero = 1;
	}
	digit = (uint8_t)((num / 100) % 10);
	if(foundnonzero || digit)
	{
		TextLCD_CharDAsm(digit);			//draw hundreds digit, if nonzero
		foundnonzero = 1;
	}
	digit = (uint8_t)((num / 10) % 10);
	if(foundnonzero || digit) TextLCD_CharDAsm(digit);			//draw tens digit, if nonzero
											//- no need to set nonzero flag
											//since ones are always drawn.
	digit = (uint8_t)(num % 10);
	TextLCD_CharDAsm(digit);					//draw ones digit, whether zero or not.
}

//numberStrlen returns the number of characters that will be drawn when renderNumber is called
//for that same number - useful for calculating centering.
uint8_t numberStrlen(uint16_t num)
{
	uint8_t digit;
	//uint8_t foundnonzero = 0;	//flag: have we found any nonzero digits yet?
	uint8_t len = 0;		//ones digit always drawn, so just count it here.

	digit = (uint8_t)(num / 10000);
	if(digit) len++;							//count 10K digit, if nonzero
	digit = (uint8_t)((num / 1000) % 10);
	if(len || digit) len++;						//count thousands digit, if nonzero or following nonzero
	digit = (uint8_t)((num / 100) % 10);
	if(len || digit) len++;						//count hundreds digit, if nonzero or following
	digit = (uint8_t)((num / 10) % 10);
	if(len || digit) len++;						//count tens digit, if nonzero or following
	len++;										//always count ones digit

	return len;
}

// renderWin draws "Win: " and winnings. Assumes top line is cleared.
void renderWin()
{
	uint8_t numlen = numberStrlen(totalpayoff);		//find rendered length of totalpayoff in characters
	uint8_t ctrpos = 10 - ((5 + numlen) >> 1);		//calculate centering: 10 = center (?),
													//5 is length of "Win: " label

	TextLCD_CmdAsm(lcdpos_line1 + ctrpos);			//center - hopework
	DrawString(str_gamescr2);						//draw "Win: " - includes space

	renderNumber(totalpayoff);						//draw win amount with no leading zeroes.
}

// BUTTON ROUTINES ------------------------------------------------------------------------------------------

void ReadButtons()
{
	//so, read port A, which is a big bunch of buttons.
	//xor with previous button state to show where the edges are.
	previousButtonState = buttonState;
	buttonState = PINA;
	buttonEdge = previousButtonState ^ buttonState;
}



//WaitForDealButtonWhileStirring() waits for a leading edge on the deal/draw button (currently
//button 7). While it's waiting, it "stirs" the RNG. Uses a roughly 1/50s debounce interval.
void WaitForDealButtonWhileStirring()
{
#ifdef DEBUG
	return;
#endif

	uint8_t done = 0;


	while(!done)
	{
		DelayAndStir();
		ReadButtons();
		if((buttonEdge & (1<<BUTTON_DEAL)) && !(buttonState & (1<<BUTTON_DEAL)))
		{
			//aha, buttonEdge shows deal button bit has changed, and buttonState
			//shows that it is now low (on). So we're done!
			done = 1;
		}
	}
}

void DelayAndStir()
{
	TextLCD_DelayAsm(5000);			// I think this should be about 20 milliseconds - verify!
#ifndef NO_STIR
	rand();							// call rng to stir it
#endif
}

void getHolds()
{
	uint8_t outerloopdone = 0;
	uint8_t j;
	uint8_t mask;

	animstate = 0;					// here, animation state 0 means choose is showing, 1 means press deal.

	TextLCD_CmdAsm(lcdpos_line4);	// start off w/prompt = choose cards to hold..no need to clear line 4
	DrawString(str_gamescr1);

	while(!outerloopdone)
	{
		animct = 100;				// rate for changing prompt, in debounce delays

		while(animct)
		{
			DelayAndStir();					// wait one debounce delay / stir RNG
			ReadButtons();

			if((buttonEdge & (1<<BUTTON_DEAL)) && !(buttonState & (1<<BUTTON_DEAL)))
			{
				//aha, buttonEdge shows deal button bit has changed, and buttonState
				//shows that it is now low (on). So we're done!
				ClearLine(4);			//erase prompt on exit.
				outerloopdone = 1;
				break;
			}

			//now to loop through and check the individual card hold buttons.
			//ASSUMES BIT N = HOLD BUTTON FOR CARD N IN THE PORT!!!!!!!!!!!
			for(j=0, mask = 0x01; j<5; j++, mask <<= 1)
			{
				if((buttonEdge & mask) && !(buttonState & mask))
				{
					//aha, button j has a leading edge.
					held[j] ^= 1;		//toggle corresponding card's held-flag
					renderHeld();		//redraw the hold-flags.
				}
			}

			animct--;
		}

		if(!animct)		//only do all this stuff if deal button wasn't just hit.
		{
			//switch to other message
			animstate ^= 1;

			ClearLine(4);								//clear bottom line
			TextLCD_CmdAsm(lcdpos_line4);

			if(animstate) DrawString(str_gamescr0);		//if anim state now = 1, draw press-deal
			else DrawString(str_gamescr1);				//else draw choose cards to hold msg.
		}

	}
}

//EEPROM STUFF ----------------------------------------------------------------------------------------------

//I have this idea that I should create a 8-byte record that saves off coins, seed, and a Magic Number.
//it will look like this:
//byte	name		desc.
// 0-1	magic		MAGIC_NUMBER signals valid record
// 2-3	coins		# coins last saved off
// 4-5	rand2-3		msword of random seed
// 6-7	rand0-1		lsword of random seed
// why not make a struct? Well, hm. Could read the whole thing probably fast enough,
// but I never need to read or write more than 4 bytes at once, really.
//
// to try to even out the burden on the eeprom, treat it as a ring buffer of EEPROM_SIZE bytes.
// I'm not sure it works this way, but if the number of write cycles is per byte, this should
// spread it out - the datasheet claims Endurance: 100,000 Write/Erase Cycles for the eeprom,
// so if there's a ring buffer of 512/8 = 64 record spaces, might get 6,400,000 record writes in
// before the eeprom wears out. If it works like that - but I guess the magic number would be
// written twice per hand - in any case, it should lengthen the eeprom life. Unless you only
// get 100K writes *anywhere* in it, in which case it's all bad anyway.
// so - at any time, only one record should be active, meaning its magic number is MAGIC_NUMBER.
// On startup, scan through the eeprom until we find a magic number on a record boundary.
// if we never find one anywhere,
// http://teslabs.com/openplayer/docs/docs/prognotes/EEPROM%20Tutorial.pdf looks informative.
// saved in avrappnotes.


// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void RestoreCoinsAndSeed()
{
	uint16_t dumword = 0;

	for(eepromRecordNum = 0; eepromRecordNum < EEPROM_RECORDS; eepromRecordNum++)
	{
		eepromPtr = eepromRecordNum << 3;					//8-byte records.
		dumword = eeprom_read_word((uint16_t*)eepromPtr);		//hopework
		if(dumword == MAGIC_NUMBER) break;		//aha! found a magic number. Stop.
	}

	if(eepromPtr == EEPROM_SIZE)
	{
		//magic number was not found anywhere in the eeprom. so, let's start a new
		//ring buffer somewhere.
		eepromPtr = EEPROM_SIZE-8;				//this way, when it's advanced, it'll go to zero.
		eepromRecordNum = EEPROM_RECORDS-1;

		//KLUD would be nice to have a warning screen!!!!!!!!!!!!!!!!!!

		//assign default values to random seed and # coins.
		nextrand = START_SEED;
		coins = START_COINS;
	}
	else
	{
		//aha, found a valid record. read seed and coins from it, and trample its magic
		//number to invalidate it... actually don't need to do that here - ?
		//eeprom_write_word((uint16_t *)eepromPtr,0);			//zero-out magic number
		eepromPtr += 2;										//advance to coins
		coins = eeprom_read_word((uint16_t*)eepromPtr);		//read coins - hopework
		eepromPtr += 2;										//advance to seed hi-word
		dumword = eeprom_read_word((uint16_t*)eepromPtr);	//read msword of seed - hopework
		nextrand = ((uint32_t)dumword) << 16;				//install msword of seed
		eepromPtr += 2;										//advance to seed lo-word
		dumword = eeprom_read_word((uint16_t*)eepromPtr);	//read lsword of seed - hopework
		nextrand |= (uint32_t)dumword;						//install lsword of seed

		//sanity check # coins - if illegal, reset to start
		//since it's unsigned, it can only be > legal value if illegal.
		//there are no illegal values for RNG.
		if(coins > MAX_COINS)
		{
			//KLUD would be nice to have an error screen!
			coins = START_COINS;
		}
	}

	prevcoins = coins;						//sync up prevcoins / coins so bet screen doesn't animate

}

// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// VERIFY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void SaveCoinsAndSeed()
{
	//This is the last thing to happen at the end of a hand, yes? So
	//invalidate write the # coins and magic number for the record, advance pointer.
	//invalidate current record -
	eepromPtr = eepromRecordNum << 3;					//8-byte records
	eeprom_write_word((uint16_t *)eepromPtr,0);			//zero-out magic number

	//advance eepromPtr! THIS ASSUMES ZERO-RELATIVE EEPROM ADDRESSES!
	//AND THAT EEPROM_SIZE IS A POWER OF TWO / MULTIPLE OF EIGHT!
	eepromRecordNum = (eepromRecordNum + 1) & (EEPROM_RECORDS - 1);
	eepromPtr = eepromRecordNum << 3;					//8-byte records.

	// write new record! Y'know, might be best to handle this as a block - see how it goes.
	eeprom_write_word((uint16_t *)eepromPtr,MAGIC_NUMBER);	//install magic number
	eepromPtr += 2;
	eeprom_write_word((uint16_t *)eepromPtr,coins);			//write coins
	eepromPtr += 2;
	eeprom_write_word((uint16_t *)eepromPtr,(uint16_t)(nextrand >> 16));	//write seed msword
	eepromPtr += 2;
	eeprom_write_word((uint16_t *)eepromPtr,(uint16_t)(nextrand & 0x0000FFFF));	//write seed lsword
}


//MAIN ------------------------------------------------------------------------------------------------------
int main(void)
{
	uint8_t done;

	//Get I/O rolling and initialize the LCD.
	InitPorts();
	TextLCD_InitAsm();
	InitCustomChars();

	//Restore coin count and random seed
	//LATER HERE WE WILL READ SEED/COINS FROM EEPROM!

	//RestoreCoinsAndSeed Needs Implemented!!!!!!!!!!!!!!!!!!!!
	RestoreCoinsAndSeed();

//	nextrand = 0x00003DC4; //was 1337d00d; - 00404990 should be a pat straight flush. Wasn't.
							//00003DC4 should be one. Oops, forgot about stirring!
//	coins = 5000;			//LATER get from eprom too.

#ifdef TESTSEEDS
	//if we're doing the predefined-seeds test, start at the first one on powerup.
	nextTestSeed = 0;
#endif

	//other initializations - may move or get packaged-up
	carddelayflag = 1;

	//Show title screen
	DrawTitleScreen();

	//Stir RNG and wait for deal button to be pressed.
	WaitForDealButtonWhileStirring();

	/*
	//let's swh if we try to deal and render a hand -
	ClearLine(1);
	ClearLine(2);
	ClearLine(4);

	//temp - undo the stirring by forcing seed to be a pat straight flush here - WORKY!
	nextrand = 0x00003DC4;
	dealhand();
	renderCards();
	*/

	// NOW FOR THE REAL MAIN GAME LOOP.
	// - get bet
	//   - bet screen should show animated coin countup?
	// - deal, score, & render pat hand
	//   - save seed and stash immediately after hand dealt, before scored & rendered
	// - get held flags
	// - draw cards, score, render drawn hand
	// - award payoff if any
	//   - add payoff to stash and save stash immediately, set up to animate payoff
	// - repeat!
	while(FOREVER)
	{
		//call	betscreen
		betScreen();						// on return from here, bet and coins will be ready to go.

#ifdef TESTSEEDS
		//if we're doing the predetermined-seeds test, grab the next one here.
		nextrand = pgm_read_dword_near(testSeeds + nextTestSeed);
		nextTestSeed++;
#endif

		dealhand();							// the Big Deal! now pat hand and draw pool are settled.
											// remember random seed in case of power-off.

		//...should I even bother with this? or just write both coins and seed at the same time
		//at the end of the hand? Let's try that.
		//SaveSeed();							// but not coins: if they power off here, they lose the bet coins
		//									// but never get to play the hand.

		score = scorehand();				// get the score for the hand in score.

		TextLCD_ClrAsm();
		renderCards();						// it's ON! draw the initial hand.
		renderScore();						// and we already have the hand's score in score, so draw that too.

		getHolds();							// Get player input for which cards to hold.

		drawCards();						// so now that we know which cards to hold - draw!

		renderCards();						// and draw the drawn hand.
		prevscore = score;					// save off pat score
		score = scorehand();				// and SCORE new hand!

		if(score != prevscore)				// if score changed, draw the new one.
		{
			ClearLine(1);
			TextLCD_CmdAsm(lcdpos_line1);
			renderScore();
		}

		prevcoins = coins;					// memorize pre-payoff coin total for animation
		payoff = pgm_read_word_near(payoffAmount+score);		//is this arithmetic right? VERIFY!
		totalpayoff = payoff * bet;			// asm does appear to be doubling it, or something.
		coins += totalpayoff;				// award coins (clamp to MAX_COINS).
		if(coins > MAX_COINS) coins = MAX_COINS;

											// HAND IS DONE!!!!!!!!!!!!!!!!!!!!!!!!

		if(coins == 0)						// if coins go to 0, restore.
		{									// might want to do something fancier in C version.
			coins = START_COINS;			// KLUD like a chiding screen!
		}

		SaveCoinsAndSeed();					// AFTER PAYOFF IS MADE, save coins / seed. This then saves
											// the deduction for the bet and also the payoff.

		TextLCD_CmdAsm(lcdpos_line4);		// show "Press Deal to Play"
		DrawString(str_splash2);

		//here, we should bop back and forth on line 1 b/t score and winnings. ?
		//while waiting for the deal button to be pressed.

		animstate = 0;						// anim state 0 means score is showing, 1 means winnings.
		done = 0;

		while(!done)
		{
			animct = 50;					// # of debounce delays to wait between switching msgs

			while(animct)
			{
				DelayAndStir();					// wait one debounce delay / stir RNG
				ReadButtons();

				if((buttonEdge & (1<<BUTTON_DEAL)) && !(buttonState & (1<<BUTTON_DEAL)))
				{
					//aha, buttonEdge shows deal button bit has changed, and buttonState
					//shows that it is now low (on). So we're done!
					done = 1;
					break;
				}

				animct--;
			}

			if(animct == 0)			//don't change message if deal button broke out of previous.
			{
				//switch to other message
				animstate ^= 1;

				ClearLine(1);								//clear top line

				if(animstate) renderWin();					//if anim state now = 1, draw winnings.
				else renderScore();
			}

		}
	}
}


/*
int main(void)
{
    // Replace with your application code
    while (1)
    {
    }
}
*/
