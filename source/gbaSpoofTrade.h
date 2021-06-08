
#ifndef _gba_spoof_h_
#define _gba_spoof_h_
//---------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------------------

int attemptFullLink();
void resetLink();

/*
typedef enum LINK_MODE {
    Normal8 = 0,
    Normal32 = 1,
    Multiplay16 = 2,
    UART = 3,
    GeneralPurpose = 4,
    JoyBus = 5
};
*/
int getLinkType();
void setLinkType(int mode);

void handleSerialInterrupt(void);

//Exchange data. (Set link type first please)
int exchangeData(int byte);
int exchangeDataWithTimeout(int byte, int framesTimeout);

//---------------------------------------------------------------------------------
#ifdef __cplusplus
}	   // extern "C"
#endif
//---------------------------------------------------------------------------------
#endif