/*
 * AVRVpok.h
 *
 * Created: 11/15/2015 1:53:38 PM
 *  Author: Sean
 */ 


#ifndef AVRVPOK_H_
#define AVRVPOK_H_


//DEFINES ================================================================================================

//conditional-compilation defines: for "release" version, all should be commented out.
//#define DEBUG							// for debugging in the simulator -
//#define NO_STIR						// if defined, RNG doesn't get stirred by wait-for-deal fn
//#define TESTSEEDS						// if defined, RNG uses the seed array in AVRVpokTestSeeds.c
// in order, starting with the first one on powerup. When doing
// this, you need to define NO_STIR as well.


#define BUTTON_DEAL		(7)				// bit # of deal button
#define BUTTON_BETMAX	(6)				// bit # of bet-max button
#define BUTTON_BETONE	(5)				// bit # of bet-one button. Note bet is always drawn as 1 digit!

#define MAX_BET			(5)				// max # coins you can bet on one hand
#define MAX_COINS		(50000)			// max # coins you can have in your bankroll
// if max coins + highest possible payoff > 65535, there's the
// potential that the number of coins could wrap around!
#define START_COINS		(500)			// # coins you start out with

#define START_SEED		(0xFACE7105)	// initial seed for RNG

#define EEPROM_SIZE		(512)			// Tiny861 has 512 bytes of eeprom - change to suit chip
// THIS MUST BE A MULTIPLE OF 8!
#define EEPROM_RECORDS	(EEPROM_SIZE >> 3)

#define MAGIC_NUMBER	(0x1337)		// 2-byte number found at the beginning of a valid eeprom record.

//GLOBALS ================================================================================================

//button vars
extern uint8_t buttonState;
extern uint8_t previousButtonState;
extern uint8_t buttonEdge;

//RNG var
extern uint32_t nextrand;

//hand variables
extern uint8_t hand[10];		//10 cards = 5 in hand (0-4) and 5 to draw from (5-9).
extern uint8_t rank[5];
extern uint8_t suit[5];
extern uint8_t held[5];			// a bit wasteful - 5 whole bytes for 5 flags? But that's modern times.

//general game variables
extern uint8_t bet;				//how many coins are bet currently
extern uint8_t score;			// save-off for current (pat or drawn) score.
extern uint8_t prevscore;		// previous score - for telling if we need to rerender score.
extern uint16_t coins;			// number of coins in player's bankroll
extern uint16_t prevcoins;		// number of coins that were in player's bankroll before the payoff
extern uint16_t payoff;			// 1-coin payoff for current hand
extern uint16_t totalpayoff;	// payoff * # coins bet

extern uint8_t carddelayflag; 	//flag: delay during card drawing?
extern uint8_t animstate;		//bet animation - state 1 means do it, 0 means don't
extern uint8_t animct;			//bet animation timing counter

//from AVRVpokStrings.c
extern const char str_splash0[] PROGMEM;
extern const char str_splash1[] PROGMEM;
extern const char str_splash2[] PROGMEM;

extern const char str_score0[] PROGMEM;
extern const char str_score1[] PROGMEM;
extern const char str_score2[] PROGMEM;
extern const char str_score3[] PROGMEM;
extern const char str_score4[] PROGMEM;
extern const char str_score5[] PROGMEM;
extern const char str_score6[] PROGMEM;
extern const char str_score7[] PROGMEM;
extern const char str_score8[] PROGMEM;
extern const char str_score9[] PROGMEM;

extern const char str_bet0[] PROGMEM;
extern const char str_bet1[] PROGMEM;
extern const char str_bet2[] PROGMEM;
extern const char str_bet3[] PROGMEM;

extern const char str_gamescr0[] PROGMEM;
extern const char str_gamescr1[] PROGMEM;
extern const char str_gamescr2[] PROGMEM;

//from AVRVpokTables.c
extern const uint8_t scoreCenterColumn[] PROGMEM;
//this dunt work extern PGM_P scorestrs[10] PROGMEM;
extern const uint8_t charData[64] PROGMEM;
extern const uint8_t rankChars[13] PROGMEM;
extern const int16_t payoffAmount[10] PROGMEM;

//what if I use some ram for the little table of score strings?
extern PGM_P scorestrs[10];

//from AVRVpokTestSeeds.c
#ifdef TESTSEEDS
extern const uint32_t testSeeds[] PROGMEM;
#endif

//PROTOS =================================================================================================

//from AVRVpokMain.c
void ReadButtons();
void DelayAndStir();
void DrawString(PGM_P str);
void renderNumber(uint16_t num);
uint8_t numberStrlen(uint16_t num);

//from AVRVpokBetScreen.c
void betScreen();





#endif /* AVRVPOK_H_ */