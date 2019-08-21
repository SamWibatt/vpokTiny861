/*
 * AVRVpokTables.c
 *
 * Created: 11/15/2015 2:02:43 PM
 *  Author: Sean
 */ 

//AVRVpokTables.c
// OCP-tables.asm - central gathering point for all data kept in tables.
// By Sean Igo for the One Chip Poker project
// Copyright 2002-2005, 2008 Sean Igo.

#include <stdint.h>
#include <avr/pgmspace.h>
#include "AVRVpok.h"

// Score centering values! Indexed by score as returned by scorehand() in main.

const uint8_t scoreCenterColumn[] PROGMEM =
{ 			// 01234567890123456789
	5, 		//|     Worthless      | col 5
	2, 		//|  Jacks or Better   | col 2
	6,		//|      Two Pair      | col 6
	2,		//|  Three of a Kind   | col 2
	6,		//|      Straight      | col 6
	7,		//|       Flush        | col 7
	5,		//|     Full House     | col 5
	3,		//|   Four of a Kind   | col 3
	3,		//|   Straight Flush   | col 3
	4		//|    Royal Flush     | col 4
};


// score strings - see file:///C:/WinAVR/doc/avr-libc/avr-libc-user-manual/FAQ.html#faq_rom_array
// Indexed by score as returned by scorehand() in main.
//was PGM_P 
//that seemed to work in studio 4, but now (11/19/15) we're in studio 7.
// see http://www.atmel.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_rom_array.html
//... seems I did stuff right.
//this dunt work PGM_P scorestrs[10] PROGMEM =

//try just putting it in ram
PGM_P scorestrs[10] =
{
	str_score0,
	str_score1,
	str_score2,
	str_score3,
	str_score4,
	str_score5,
	str_score6,
	str_score7,
	str_score8,
	str_score9
};



//custom character data
const uint8_t charData[64] PROGMEM =
{
	//heart char
	//'---*****'				//only get to use lowest order 5 bits
	0x0A,	//'00001010'		// @ @
	0x15,	//'00010101'		//@ @ @
	0x11,	//'00010001'		//@   @
	0x11,	//'00010001'		//@   @
	0x11,	//'00010001'		//@   @
	0x0A,	//'00001010'		// @ @
	0x04,	//'00000100'		//  @
	0x00,	//'00000000'

	//diamond
	0x04,	//'00000100'		//  @
	0x0A,	//'00001010'		// @ @
	0x11,	//'00010001'		//@   @
	0x11,	//'00010001'		//@   @
	0x11,	//'00010001'		//@   @
	0x0A,	//'00001010'		// @ @
	0x04,	//'00000100'		//  @
	0x00,	//'00000000'

	//club
	0x04,	//'00000100'		//  @
	0x0E,	//'00001110'		// @@@
	0x15,	//'00010101'		//@ @ @
	0x1B,	//'00011011'		//@@ @@
	0x15,	//'00010101'		//@ @ @
	0x04,	//'00000100'		//  @
	0x0E,	//'00001110'		// @@@
	0x00,	//'00000000'

	//spade
	0x04,	//'00000100'		//  @
	0x0E,	//'00001110'		// @@@
	0x1F,	//'00011111'		//@@@@@
	0x1F,	//'00011111'		//@@@@@
	0x1F,	//'00011111'		//@@@@@
	0x04,	//'00000100'		//  @
	0x0E,	//'00001110'		// @@@
	0x00,	//'00000000'
	
	//ten
	0x17,	//'00010111'		//@ @@@
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x17,	//'00010111'		//@ @@@
	0x00,	//'00000000'

	//"HE"
	0x17,	//'00010111'		//@ @@@
	0x16,	//'00010110'		//@ @@
	0x16,	//'00010110'		//@ @@
	0x1F,	//'00011111'		//@@@@@
	0x16,	//'00010110'		//@ @@
	0x16,	//'00010110'		//@ @@
	0x17,	//'00010111'		//@ @@@
	0x00,	//'00000000'

	//"LD"
	0x16,	//'00010110'		//@ @@
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x15,	//'00010101'		//@ @ @
	0x1E,	//'00011110'		//@@@@
	0x00,	//'00000000'

	//Card back - checkerboard which might be a standard character, but let's not risk it.
	0x15,	//'00010101'		//@ @ @
	0x0A,	//'00001010'		// @ @
	0x15,	//'00010101'		//@ @ @
	0x0A,	//'00001010'		// @ @
	0x15,	//'00010101'		//@ @ @
	0x0A,	//'00001010'		// @ @
	0x15,	//'00010101'		//@ @ @
	0x00,	//'00000000'
};


// rankchars is the table that returns the character to send to the LCD for a given rank.

const uint8_t rankChars[13] PROGMEM =
{
	'A',	// 0 - Ace - 'A'
	'2',	// 1 - 2
	'3',	// 2 - 3
	'4',	// 3 - 4
	'5',	// 4 - 5
	'6',	// 5 - 6
	'7',	// 6 - 7
	'8',	// 7 - 8
	'9',	// 8 - 9
	4,		// 9 - 10 - custom char 4 is the one-character 10.
	'J',	// 10 - Jack - 'J'
	'Q',	// 11 - Queen - 'Q'
	'K'		// 12 - King - 'K'
};


//payoffAmount Indexed by score as returned by scorehand() in main.

const int16_t payoffAmount[10] PROGMEM =
{
	0,			//	Nothing	 		0
	1,			//	Pair 			1
	2,			//	Two pair 		2
	3,			//	Three of a kind 3
	4,			//	Straight 		4
	6,			//	Flush 			6
	9,			//	Full house 		9
	25,			//	Four of a kind 	25
	50,			//	Straight flush 	50
	800			//	Royal flush 	800 (0x320)
};

