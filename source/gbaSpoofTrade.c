#include <gba.h>
#include <stdio.h>
#include <stdlib.h>
#include "libSave.h"
#include "datahelpers.h"


#define LINK_NODATA 0xFE
#define LINK_PREAMBLE 0xFD

#define LINK_TOLEADER 0x02
#define LINK_TOPARTNER 0x01
#define LINK_ACK 0x00
#define LINK_TRADECUE 0xD4

#define setmask(reg, mask) (reg = reg | mask)
#define unsetmask(reg, mask) (reg = reg & -mask)
#define flipmask(reg, mask) (reg = reg ^ mask)

#define isbitset(reg, mask) (reg & mask)

int linkProgress = 0;
int isLeader = 0;
int playerNum = -1;
int isWaitingOnData = false;

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

void handleSerialInterrupt(void);

void setLinkType(int mode)
{
    switch (mode) {
        case Normal8:
        {
            REG_RCNT = R_NORMAL;
            REG_SIOCNT = SIO_8BIT;
            break;
        }
        case Normal32:
        {
            REG_RCNT = R_NORMAL;
            REG_SIOCNT = SIO_32BIT;
            break;
        }
        case Multiplay16:
        {
            REG_RCNT = R_MULTI;
            REG_SIOCNT = SIO_MULTI;
            break;
        }
        case UART:
        {
            REG_RCNT = R_UART;
            REG_SIOCNT = SIO_UART;
            break;
        }
        case GeneralPurpose:
        {
            REG_RCNT = R_GPIO;
            break;
        }
        case JoyBus:
        {
            REG_RCNT = R_JOYBUS;
            break;
        }
    }
}

int exchangeData(int byte)
{
    //Unimplemented
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

int exchangeDataWithTimeout(int byte, int framesTimeout)
{
    int i = 0;
    switch (getLinkType())
    {
        case Normal8:
        {
            REG_SIODATA8 = byte;
            if (isLeader)
            {
                //Leader, Normal8
                unsetmask(REG_SIOCNT, SIO_START);
                setmask(REG_SIOCNT,SIO_CLK_INT);
                i=0;
                while (isbitset(REG_SIOCNT, SIO_RDY))
                {
                    VBlankIntrWait();
                    i++;
                    if (i >= framesTimeout)
                        break;
                }
                if (isbitset(REG_SIOCNT,SIO_RDY) && (i >= framesTimeout))
                {
                    iprintf("\x1b[3;1HPartner not ready      \n");
                    REG_SIODATA8 = LINK_NODATA;
                    return LINK_NODATA;
                }

                setmask(REG_SIOCNT, SIO_START);
                IntrWait(0,IRQ_SERIAL);
                return REG_SIODATA8;
            }
            else
            {
                //Partner, Normal8
                setmask(REG_SIOCNT, SIO_START);
                unsetmask(REG_SIOCNT, SIO_SO_HIGH);

                if (isbitset(REG_SIOCNT,SIO_START))
                {
                    for (i=0; i<framesTimeout; i++)
                    {
                        VBlankIntrWait();
                        if (!isbitset(REG_SIOCNT,SIO_START))
                            break;
                    }
                }
                if (!isbitset(REG_SIOCNT,SIO_START))
                {
                    return REG_SIODATA8;
                }
                else
                {
                    iprintf("\x1b[3;1HNobody's here...    \n");
                    REG_SIODATA8 = LINK_NODATA;
                    return LINK_NODATA;
                }

            }
            break;
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
    iprintf("\x1b[3;1HComm setup...       \n");
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

int doWholeHandshake() {
    int waitcycles = 5;
    int i;
    
    //Try to be partner.
    isLeader = false;
    if (exchangeDataWithTimeout(LINK_TOLEADER, 2) == LINK_TOPARTNER)
    {
        while (exchangeDataWithTimeout(LINK_ACK, 10) != LINK_ACK);

        playerNum = 2;
        iprintf("\x1b[3;1HBecame partner!         \n");
    }
    
    if (playerNum < 0)
    {
        isLeader = true;
        //Try to be leader. Send 1, get 2
        while (exchangeDataWithTimeout(LINK_TOPARTNER, 10) != LINK_TOLEADER);
        while (exchangeDataWithTimeout(LINK_ACK, 10) != LINK_ACK);
        
        playerNum = 1;
        iprintf("\x1b[3;1HBecame leader!          \n");
    }


	iprintf("\x1b[18;12HPlayer: %d", playerNum);
    if (playerNum >= 0)
        return 1;
    else
        return 0;
}

int checkMenuing() {
    //Send 4, wait for partner to agree
    
    iprintf("\x1b[3;1HI'm P%d! Pick an option.   \n", playerNum);
    int val = LINK_NODATA;
    int i = 0;
    while ((0xD4 > val) || (val > 0xD6))
    {
        //i++;
        val = exchangeDataWithTimeout(LINK_TRADECUE, 10);
        //iprintf("\x1b[2;1HOther one has %x        \n", val);
    }

    if (val >= LINK_TRADECUE)
    {
        if (val == LINK_TRADECUE)
        {
            iprintf("\x1b[3;1HGot trade!             \n");
            return 1;
        }
        else
        {
            iprintf("\x1b[3;1HOther one picked %x     \n", val);
            return 0;
        }
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

int attemptFullLink() {
    if (!setupCommunication())
    {
        iprintf("\x1b[2;1HCouldn't setup.     ");
        return 0;
    }
    if (!doWholeHandshake())
    {
        iprintf("\x1b[2;1HCouldn't handshake.    ");
        return 0;
    }
    if (!checkMenuing())
    {
        iprintf("\x1b[2;1HCouldn't menu.       ");
        return 0;
    }
    //if (!checkTradeReady())
    if (false)
    {
        iprintf("\x1b[2;1HCouldn't trade.      ");
        return 0;
    }
    return 1;
}

void resetLink()
{
    setLinkType(0);
    linkProgress = 0;
    isLeader = 0;
    playerNum = -1;
    isWaitingOnData = false;
    iprintf("\x1b[2;1HReset Link              \n");
    iprintf("\x1b[3;1HLink Reset              \n");
    return;
}