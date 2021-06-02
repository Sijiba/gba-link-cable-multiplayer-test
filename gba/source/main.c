/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gba.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"
#include "gbaSpoofTrade.h"
#include "datahelpers.h"

#define	REG_WAITCNT *(vu16 *)(REG_BASE + 0x204)
#define JOY_WRITE 2
#define JOY_READ 4
#define JOY_RW 6

u8 save_data[0x20000] __attribute__ ((section (".sbss")));

s32 getGameSize(void)
{
	if(*(vu32*)(0x08000004) != 0x51AEFF24)
		return -1;
	s32 i;
	for(i = (1<<20); i < (1<<25); i<<=1)
	{
		vu16 *rompos = (vu16*)(0x08000000+i);
		int j;
		bool romend = true;
		for(j = 0; j < 0x1000; j++)
		{
			if(rompos[j] != j)
			{
				romend = false;
				break;
			}
		}
		if(romend) break;
	}
	return i;
}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            iprintf("%u", byte);
        }
    }
}

//---------------------------------------------------------------------------------
// Program entry point
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	// the vblank interrupt must be enabled for VBlankIntrWait() to work
	// since the default dispatcher handles the bios flags no vblank handler
	// is required
	irqInit();
	irqEnable(IRQ_VBLANK);

	consoleDemoInit();
	REG_JOYTR = 0;
	// ansi escape sequence to set print co-ordinates
	// /x1b[line;columnH
	iprintf("\x1b[1;1HGBA<->GBA/GBC Link Test\n");
	// disable this, needs power
	SNDSTAT = 0;
	SNDBIAS = 0;
	// Set up waitstates for EEPROM access etc. 
	REG_WAITCNT = 0x0317;
	//clear out previous messages
	REG_HS_CTRL |= JOY_RW;
	

	//Prepare display for held keys
	iprintf("\x1b[4;1HMy Keys:");
	iprintf("\x1b[7;1HVS Keys:");
	const int keys[] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_L, KEY_R, KEY_A, KEY_B, KEY_START, KEY_SELECT};
	const char * keyNames[] = {"u", "d", "l", "r", "L", "R", "A", "B", "S", "s"};
	const int keysCount = sizeof(keys)/sizeof(keys[0]);
	const int playerCount = 2;
	u16 playerKeys[2] = {0,0};
	u16 previousKeys[2] = {0,0};
	const int playerRowOffset = 4;
	const int playerColOffset = 10;

	int initiatedLink = 0;
	irqEnable(IRQ_SERIAL);
	irqSet(IRQ_SERIAL, NULL);
    setLinkType(0);

	while(1)
	{
		for (int i = 0; i < playerCount; i++)
			previousKeys[i] = playerKeys[i];
		
		scanKeys();
		u16 playerKeys[2] = {0,0};
		playerKeys[0] = keysHeld();
		playerKeys[1] = 0xFFFF;

		//Display held buttons for all players
		
		for (int p = 0; p < playerCount; p++)
		{
			int xPos = playerRowOffset + (3 * p);
			iprintf("\x1b[%d;%dH", xPos, playerColOffset);
			printBits(sizeof(playerKeys[0]), &playerKeys[p]);
			iprintf("\x1b[%d;%dH", xPos + 1, playerColOffset);
			for (int k = 0; k < keysCount; k++)
			{
				if (playerKeys[p] & keys[k])
					iprintf(keyNames[k]);
				else
					iprintf(" ");
			}
			//*/
		}

		printRegisters();

		int type = getLinkType();
		iprintf("\x1b[18;1HMode: %d", type);
		
		if (((!previousKeys[0]) & KEY_A) && (playerKeys[0] & KEY_A))
		{
			initiatedLink = 1;
		}	

		if (playerKeys[0] & KEY_B)
		{
			resetLink();								
			initiatedLink = 0;
		}
 
		if (initiatedLink)
		{
			attemptLink();
		}

		
	}
}


