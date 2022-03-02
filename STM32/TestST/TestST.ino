// Duino/STM32/TestST/TestST.ino - STM32 test code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

//#include "CMifare.hpp"
#include "Common/STM32/ST_Util.hpp"
#include "Common/STM32/ST_Timing.hpp"


/***/

#define PIN_LED    PC13
#define PIN_PULSE  PB12
#define DEBUG_BAUD 115200 


/***/

CClock gClock;
CTimer gT;
//CHackMFRC522 gRC522;

void tickFunc (void) { gT.nextIvl(); }

void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("TestST " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

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
} // setup

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
