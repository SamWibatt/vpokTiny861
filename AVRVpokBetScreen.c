/*
 * AVRVpokBetScreen.c
 *
 * Created: 11/15/2015 2:01:27 PM
 *  Author: Sean
 */ 

//AVRVpokBetScreen - may get ripped out in favor of just sticking it in main,
//but on the PIC asm side betscreen looks pretty hairy.

#include <stdint.h>
#include <avr/pgmspace.h>
#include "AVRVpok.h"
#include "LCD20x4-AVR.h"

//protos
void betScreenDrawCoins();
void betScreenDrawBet();
uint8_t incBet();

void betScreen()
{
	uint8_t outerdone = 0;
	uint8_t innerdone = 0;
	uint8_t j;

	bet = 0;							//no coins bet yet!

	TextLCD_ClrAsm();					// initial clear screen
	// then draw fixed part of bet screen
	// "Coins: " now handled in DrawCoins
	//TextLCD_CmdAsm(lcdpos_line1 + 4);	// "Coins: " - draw on line 1, column 4
	//DrawString(str_bet0);
	TextLCD_CmdAsm(lcdpos_line2 + 7);	// "Bet:" - line 2, col 7
	DrawString(str_bet1);
	TextLCD_CmdAsm(lcdpos_line3 + 3);	// "Bet 1 or Max" - line 3, col 3
	DrawString(str_bet2);
	TextLCD_CmdAsm(lcdpos_line4)	;	// finally "bet max/deal to play" - line 4.
	DrawString(str_bet3);

	betScreenDrawCoins();				// draw player's bankroll - before payoff, will animate up
	betScreenDrawBet();					// and current bet, i.e. 0

	animstate = 1;						// set up for bet animation - state 1 means do it, 0 means don't

	while(!outerdone)					// This corresponds to pic's bscr_outerloop
	{
		animct = 5;						// animation rate = ca. 10 per sec, ??? ACTUALLY SHOULD SLOW OR SPEED BY HOW MANY LEFT?
		innerdone = 0;

		while(!innerdone)
		{
			DelayAndStir();

			ReadButtons();				// and read the buttons!
			// highest priorities: Bet Max first, then Deal, then Bet 1; others ignored.

			// so: let's look for a leading edge on the Bet Max button.
			if((buttonEdge & (1<<BUTTON_BETMAX)) && !(buttonState & (1<<BUTTON_BETMAX)))
			{
				for(j=0;j<MAX_BET;j++) 		// increment bet MAX_BET times. If bet already
				incBet();				// has coins in it, incbet is smart enough to clamp.
				prevcoins = coins;			// sync up coin amount so it draws properly
				betScreenDrawCoins(); 		// redraw new coin amount
				betScreenDrawBet();			// and new bet amount
				return;						// bet is now max which means play starts. Bail.
			}

			// if bet max wasn't pressed, then see if deal button pressed
			if((buttonEdge & (1<<BUTTON_DEAL)) && !(buttonState & (1<<BUTTON_DEAL)))
			{
				//if bet is zero, do nothing. Nonzero, return
				if(bet != 0) return;		// bet nonzero - animation would have stopped by now.
			}

			// no bet max or deal - how about bet 1?
			if((buttonEdge & (1<<BUTTON_BETONE)) && !(buttonState & (1<<BUTTON_BETONE)))
			{
				j = incBet();				//increment bet - if now max, j will = 1
				prevcoins = coins;			// sync up coin amount so it draws properly
				betScreenDrawCoins(); 		// redraw new coin amount
				betScreenDrawBet();			// and new bet amount

				if(j == 1) return;			//bet is now MAX_BET, so bail.
			}

			//end of loop stuff - do animating if necessary.
			if(animstate == 1)
			{
				//see if animation timer has ticked over.
				animct--;
				if(animct == 0)
				{
					innerdone = 1;		//inner loop condition = animation counter ticked over
					if(prevcoins != coins)
					{
						prevcoins++;	//animate coin count-up!
						betScreenDrawCoins();
					}
					else
					{
						animstate = 0;	//coins have counted all the way up, done animating.
					}
				}
			}
		}
	}
}

// incbet does the following:
// - if bet animation is happening, stop it.
// - if bet is MAX_BET, return 1.
// - otherwise, bet is assumed to be >= 0 and < 5. increment bet, decrement coins.
//   - if there are no more coins to bet, return 1. Not really max, but can bet no more.
// - if bet is NOW MAX_BET, return 1
// - otherwise return 0
//
// So, it has the effect of stopping animation if any, then incrementing the bet clamped to MAX_BET.
// If the bet is 5, either on entry or through the incrementing, return 1 so caller
// can tell.
uint8_t incBet()
{
	animstate = 0;						//stop animation, if any
	if(bet == MAX_BET) return 1;		//if bet = max, done.

	if(coins)							//can't inc bet if there are no more coins!
	{
		bet++;							//increment bet
		coins--;						//decrement coins
		if(bet == MAX_BET) return 1;	//if bet = max, done.
	}
	else
	{
		return 1;						//no more coins - effectively a max bet.
	}

	return 0;							// more bet incretion is still possible!
}

// drawcoins actually draws prevcoins, to accommodate animation while keeping coins having
// the proper value always. player's current coin amount -
// now also draws the "Coins: " label. Doesn't clear the line; assuming we don't need to,
// but be sure of that VERIFY!!!!!!!!!!!!!!!!!
// assumes 20-character-wide screen

void betScreenDrawCoins()
{
	uint8_t numlen = numberStrlen(prevcoins);		//find rendered length of prevcoins in characters
	uint8_t ctrpos = 10 - ((7 + numlen) >> 1);		//calculate centering: 10 = center (?),
	//7 is length of "Coins: " label

	TextLCD_CmdAsm(lcdpos_line1 + ctrpos);			//center - hopework
	DrawString(str_bet0);							//draw "Coins: " - includes space

	renderNumber(prevcoins);						//draw # coins with no leading zeroes.
}

//betScreenDrawBet draws player's current bet @ line 2, col 12
void betScreenDrawBet()
{
	TextLCD_CmdAsm(lcdpos_line2 + 12);
	TextLCD_CharDAsm(bet);
}

