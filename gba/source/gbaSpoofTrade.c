#include <gba.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"
#include "datahelpers.h"


#define LINK_NODATA 0xFE
#define LINK_PREAMBLE 0xFD

#define LINK_TOLEADER 0x1C
#define LINK_TOPARTNER 0x24

#define setmask(reg, mask) (reg = reg | mask)
#define unsetmask(reg, mask) (reg = reg & -mask)
#define flipmask(reg, mask) (reg = reg ^ mask)

#define isbitset(reg, mask) (reg & mask)

int linkProgress = 0;
int isLeader = 0;
int playerNum = -1;

typedef enum LINK_MODE {
    Normal8 = 0,
    Normal32 = 1,
    Multiplay16 = 2,
    UART = 3,
    GeneralPurpose = 4,
    JoyBus = 5
};

int getLinkType()
 {
    if ((REG_RCNT & R_GPIO))
    {
        if (REG_RCNT & 0x4000)
            return JoyBus;
        else
            return GeneralPurpose;
    }
    else if (REG_SIOCNT & SIO_32BIT)
    {
        if (REG_SIOCNT & SIO_MULTI)
            return UART;
        else
            return Normal32;
    }
    else if (REG_SIOCNT & SIO_MULTI)
        return Multiplay16;
    return Normal8;
}

void setLinkType(int mode)
{
    switch (mode) {
        case Normal8:
        {
            unsetmask(REG_RCNT, 0x8000);
            unsetmask(REG_RCNT, 0x4000);
            unsetmask(REG_RCNT, 0xC000);
            unsetmask(REG_SIOCNT,SIO_32BIT);
            unsetmask(REG_SIOCNT,SIO_MULTI);
            break;
        }
        case Normal32:
        {
            unsetmask(REG_RCNT, 0xC000);
            unsetmask(REG_SIOCNT,SIO_MULTI);
            setmask(REG_SIOCNT,SIO_32BIT);
            break;
        }
        case Multiplay16:
        {
            REG_RCNT = REG_RCNT & 0x7FFF;
            REG_SIOCNT = REG_SIOCNT & 0xEFFF | SIO_MULTI;
            break;
        }
        case UART:
        {
            REG_RCNT = REG_RCNT & 0x7FFF;
            REG_SIOCNT = REG_SIOCNT | SIO_UART;
            break;
        }
        case GeneralPurpose:
        {
            REG_RCNT = REG_RCNT | R_GPIO & 0xBFFF;
            break;
        }
        case JoyBus:
        {
            REG_RCNT = REG_RCNT | R_JOYBUS;
            break;
        }
    }
}

int exchangeData(int byte)
{
    switch(getLinkType())
    {
        case Normal8:
        {
            if (isLeader)
            {

            }
            else
            {

            }
            REG_SIODATA8 = byte;
            setmask(REG_SIOCNT,SIO_START);
            IntrWait(0,IRQ_SERIAL);
            setmask(REG_SIOCNT,SIO_SO_HIGH);
            return REG_SIODATA8;
            //break;
        }
        default:
        {
            //Unimplemented
        }
    }
    return LINK_NODATA;
}

int setupCommunication()
{
    REG_SIODATA8 = LINK_NODATA;
    REG_SIODATA32 = LINK_NODATA;
    unsetmask(REG_SIOCNT, SIO_CLK_INT);
    setmask(REG_SIOCNT, SIO_IRQ);
    iprintf("\x1b[3;1HComm setup...    \n");
    switch(getLinkType())
    {
        case Normal8:
        {
            break;
        }
        default:
        {
            //Unimplemented
            iprintf("\x1b[3;1HUnimplemented...    \n");
            return 0;
        }
    }

    return 1;
}

int checkInitialHandshake() {
    int waitcycles = 5;
    int i;
    
    if (!isLeader)
    {
        //Init data
        //  Send 0x02, seek 0x01
        REG_SIODATA8 = LINK_TOLEADER;
        //Try to be partner.
        //init mode/clock
        unsetmask(REG_SIOCNT, SIO_CLK_INT);
        iprintf("\x1b[3;1HAre we partner?       \n");

        
        //Almost ready
        unsetmask(REG_SIOCNT, SIO_START);
        unsetmask(REG_SIOCNT,SIO_SO_HIGH);

        //Indicate ready
        setmask(REG_SIOCNT, SIO_START);
        //setmask(REG_SIOCNT, SIO_SO_HIGH);
        
        //wait for master
        for (i=0; i<waitcycles; i++)
        {
            VBlankIntrWait();
        }
        //If Start was unset process data
        if (!isbitset(REG_SIOCNT, SIO_START))
        {
            setmask(REG_SIOCNT, SIO_SO_HIGH);
            
            if (REG_SIODATA8 == LINK_TOPARTNER)
            {
                iprintf("\x1b[3;1HBecame partner!      \n");
                playerNum = 2;
            }
            else
            {
                iprintf("\x1b[3;1HWeird data from leader!      \n");
            }
            setmask(REG_IF, IRQ_SERIAL);
        }
        else
        {
            iprintf("\x1b[3;1HNo link!        \n");
        }
        if (playerNum < 0)
            isLeader = true;
    }
    else
    {
        //Try to be leader. Send 1, get 2

        //Init data
        REG_SIODATA8 = LINK_TOPARTNER;
        //Init clock/transfer rate
        unsetmask(REG_SIOCNT, SIO_START);
        //TODO why isn't internal clock setting consistently?
        setmask(REG_SIOCNT, SIO_CLK_INT);
        printRegisters();

        iprintf("\x1b[3;1HAre we leader?          \n");
        
        //Wait for partner w/timeout
        if (!isbitset(REG_SIOCNT, SIO_RDY))
        {
            //set ready
            setmask(REG_SIOCNT, SIO_START);
            
            for (i=0; i<waitcycles; i++)
            {
                VBlankIntrWait();
            }
            //*/

            //IntrWait(0,IRQ_SERIAL);
            
            if (!isbitset(REG_SIOCNT, SIO_START))
            {
                if (REG_SIODATA8 == LINK_TOLEADER)
                {
                    iprintf("\x1b[3;1HBecame leader!          \n");
                    //exchangeData(0x01);
                    playerNum = 1;
                }
                else
                    iprintf("\x1b[3;1HWeird data from partner!      \n");
            }
            else
                iprintf("\x1b[3;1HNo data from partner!         \n");
        }
        else
            iprintf("\x1b[3;1HPartner not ready!    \n");
        
    }
    
    //unsetmask(REG_SIOCNT, SIO_START);

    if (playerNum >= 0)
        return 1;
    else
    {
        for (i=0; i<waitcycles; i++)
        {
            VBlankIntrWait();
        }
        return 0;
    }
}

int checkMenuing() {
    //Send 4, wait for partner to agree
    
    iprintf("\x1b[3;1HAwaiting P%d decision!   \n", playerNum);
    if (REG_SIODATA8 == 4)
    {
	    iprintf("\x1b[3;1HEntering trade!   \n");
        return 1;
    }
    return 0;
}


void sendData() {
    // Initialize Data
    
	iprintf("\x1b[3;1HSending data...      \n");
    int helloBin[] = {
        0x3E, 0x63, 0xF5, 0x21, 0xA0, 0xC3, 0x01, 0x88, 0x01, 0xCD, 0xE0, 0x36, 0x11, 0xFF, 0xC5, 0x21,
        0x57, 0xC4, 0xCD, 0x55, 0x19, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0xF1, 0x3C, 0x0E, 0x6C,
        0xB9, 0xC2, 0xD8, 0xC5, 0x3E, 0x63, 0xC3, 0xD8, 0xC5, 0x7F, 0x87, 0x84, 0x8B, 0x8B, 0x8E, 0x7F,
        0x96, 0x8E, 0x91, 0x8B, 0x83, 0xE7, 0x7F, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
}

int checkTradeReady() {
    return 0;
}

void attemptLink() {
    switch(linkProgress) {
        case 0:
        {
            linkProgress += setupCommunication();
            break;
        }
        case 1:
        {
            linkProgress += checkInitialHandshake();
            break;
        }
        case 2:
        {
            linkProgress += checkMenuing();
            break;
        }
        case 3:
        {
            linkProgress += checkTradeReady();
            break;
        }
        default:
        {}
    }
    iprintf("\x1b[2;1HLinkProgress: %d", linkProgress);
    return;
}


void resetLink()
{
    setLinkType(0);
    linkProgress = 0;
    isLeader = 0;
    playerNum = -1;
    iprintf("\x1b[2;1HReset Link              \n");
    iprintf("\x1b[3;1HLink Reset              \n");
    return;
}