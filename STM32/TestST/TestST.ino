// Duino/STM32/TestST/TestST.ino - STM32 test code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

#if 0
//#include "Common/MBD/mbdDef.h" // UU16/32
#else
typedef union { uint32_t u32; uint16_t u16[2]; uint8_t u8[4]; } UU32;
#endif

#include "Common/STM32/ST_Util.hpp"
#include "Common/STM32/ST_Timing.hpp"
#include <SPI.h>
#include "Common/CW25Q.hpp"
#include "Common/MFDHacks.hpp"


/***/

#define PIN_LED    PC13
#define PIN_PULSE  PB12
#define DEBUG_BAUD 115200 


/***/

CClock gClock;
CTimer gT;
void tickFunc (void) { gT.nextIvl(); }

CW25QDbg gW25QDbg(21);
#ifdef TEST_CRC
HWCRC crc;
#endif


void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("TestST " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

void dump (Stream& s)
{ // check packing
  s.print("CFC header size:");
  s.print(sizeof(CFCHdrJ0)); s.print(", ");
  s.print(sizeof(CFCHdrD0)); s.print(", ");
  s.print(sizeof(CFCHdrR0)); s.println();
}

void setup (void)
{
  noInterrupts();
  
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);

  pinMode(PIN_PULSE, OUTPUT);
  digitalWrite(PIN_PULSE, 0);

  gClock.setA(__DATE__,__TIME__);
  gT.start(tickFunc);

  interrupts();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  
  //gRC522.init();
  gT.dbgPrint(DEBUG);
  
  gW25QDbg.init(DEBUG);
  gW25QDbg.dataEraseDirty(0xD000,0x4000); // 1MB
  
  dump(DEBUG);
} // setup

W25Q::PageScan scan(0xD000);
uint16_t last=0;
uint8_t c=0;
void loop (void)
{
  const uint16_t d= gT.diff();
  if (d >= 1000)
  {
    digitalWrite(PIN_LED, 1);
    gClock.print(DEBUG);
    //gRC522.hack(DEBUG);
    gT.retire(1000);
    c= 100;
    scan.next(DEBUG,gW25QDbg);
  }
  if (d != last)
  {
    c-= (c>0);
    digitalWrite(PIN_LED, c > 0);
    last= d;
#if 1
    uint16_t t[2], j;
    for (int i=0; i<20; i++)
    {
      j= i & 0x1;
      digitalWrite(PIN_PULSE, j);
      //t[j]= hwTickVal();
      //delayMicroseconds(1);
    }
    digitalWrite(PIN_PULSE, 0);
#else
    digitalWrite(PIN_PULSE, gT.swTickVal() & 0x1 );
#endif
  }
} // loop
