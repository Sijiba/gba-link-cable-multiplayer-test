
#include <gba.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"

void printRegisters()
{
    //Prepare display of specific registers
	vu16* playerAddr = 5;
	vu16* twoBytePtrs[] = {&REG_SIOCNT, &REG_RCNT, &REG_SIODATA8/*, &REG_HS_CTRL, &REG_SIOMLT_SEND, &REG_SIOMLT_RECV*/};
	const char * twoBytePtrNames[] = {"SioCnt", "RCnt", "SioData8"/*, "HsCtrl", "SioMultSend", "SioMultRecv"*/};
	const int twoBytePtrCount = sizeof(twoBytePtrs)/sizeof(twoBytePtrs[0]);
	const int ptrRowOffset = 14;
	const int ptrColOffset = 1;
    
	iprintf("\x1b[%d;1HF              0",ptrRowOffset - 1);
    for (int p = 0; p < twoBytePtrCount; p++)
    {
        iprintf("\x1b[%d;%dH", ptrRowOffset + p, ptrColOffset);
        printBits(sizeof(*twoBytePtrs[0]), twoBytePtrs[p]);
        iprintf(" %s", twoBytePtrNames[p]);
    }
}