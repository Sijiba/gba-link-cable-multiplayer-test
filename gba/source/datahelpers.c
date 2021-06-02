
#include <gba.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"

void printRegisters()
{
    //Prepare display for specific registers
	vu16* playerAddr = 5;
	vu16* twoBytePtrs[] = {&REG_SIOCNT, &REG_RCNT, &REG_HS_CTRL, &REG_SIODATA8, &REG_SIOMLT_SEND, &REG_SIOMLT_RECV};
	const char * twoBytePtrNames[] = {"SioCnt", "RCnt", "HsCtrl", "SioData8", "SioMultSend", "SioMultRecv"};
	const int twoBytePtrCount = sizeof(twoBytePtrs)/sizeof(twoBytePtrs[0]);
	iprintf("\x1b[10;1HF              0");
	const int ptrRowOffset = 11;
	const int ptrColOffset = 1;
    //Print choice registers
    //twoBytePtrs[twoBytePtrCount-1] = &playerKeys[0];

    for (int p = 0; p < twoBytePtrCount; p++)
    {
        iprintf("\x1b[%d;%dH", ptrRowOffset + p, ptrColOffset);
        printBits(sizeof(*twoBytePtrs[0]), twoBytePtrs[p]);
        iprintf(" %s", twoBytePtrNames[p]);
    }
}