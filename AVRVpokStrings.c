/*
 * AVRVpokStrings.c
 *
 * Created: 11/15/2015 1:59:44 PM
 *  Author: Sean
 */ 
//AVRVPokStrings.c - strings that are meant to be stored in program memory.
//see avr-libc-user-manual/FAQ.html#faq_rom_array

#include <stdint.h>
#include <avr/pgmspace.h>
#include "AVRVpok.h"

//const char str_splash0[] PROGMEM = "**** Welcome to ****";
const char str_splash0[] PROGMEM = "     Welcome to     ";
const char str_splash1[] PROGMEM = "One Chip Video Poker";
const char str_splash2[] PROGMEM = "-Press Deal to Play-";

const char str_score0[] PROGMEM = "Worthless";
const char str_score1[] PROGMEM = "Jacks or Better";
const char str_score2[] PROGMEM = "Two Pair";
const char str_score3[] PROGMEM = "Three of a Kind";
const char str_score4[] PROGMEM = "Straight";
const char str_score5[] PROGMEM = "Flush";
const char str_score6[] PROGMEM = "Full House";
const char str_score7[] PROGMEM = "Four of a Kind";
const char str_score8[] PROGMEM = "Straight Flush";
const char str_score9[] PROGMEM = "Royal Flush";



// bet screen stuff
// 01234567890123456789
// --------------------
//|    Coins: 00000    |
//|       Bet: 0       |
//|    Bet 1 or max    |
//|Bet Max/Deal to Play|
// --------------------


const char str_bet0[] PROGMEM = "Coins: ";
const char str_bet1[] PROGMEM = "Bet:";
const char str_bet2[] PROGMEM = "Bet 1 or Max";
const char str_bet3[] PROGMEM = "Bet Max/Deal to Play";


// game screen stuff
//  --------------------
// |(centered scorename)|
// | RS  RS  RS  RS  RS | <- cards
// | HE  HE  HE  HE  HE | <- held flags
// | Press Deal to Draw | <- for first screen; alternate b/t
// |Choose Cards to Hold|    THIS and press deal to draw, once a second or so
// |-Press Deal to Play-| <- THIS last line after cards redealt
//  --------------------
//       Win: 0000        Centered winnings line alternates with scorename on line 1
//  01234567890123456789

const char str_gamescr0[] PROGMEM = " Press Deal to Draw";		// render in column 0
const char str_gamescr1[] PROGMEM = "Choose Cards to Hold";		// render in column 0
const char str_gamescr2[] PROGMEM = "Win: ";					// render in center - see main.c

// the -Press Deal to Play- is the same as str_splash2.
