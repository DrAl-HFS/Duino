// Duino/STM32/TestST/TestST.ino - STM32 test code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

#include "CMifare.hpp"
#include "Common/STM/ST_Timing.hpp"


/***/

#define LED PC13
#define DEBUG_BAUD 115200 


/***/

CTimer gT;
CHackMFRC522 gRC522;

uint32_t gLast= 0;
uint32_t gTick= 0;

void tickFunc (void) { gT.nextIvl(); }

void setup (void)
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("TestST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  
  gT.start(tickFunc);

  gRC522.init();
  gT.dbgPrint(DEBUG);
  DEBUG.print("tick"); DEBUG.println(gTick);//gTim.poll());
} // setup

void loop (void)
{
  if (gT.diff() >= 1000)
  {
    //gRC522.hack();
    gT.retire(1000);
    DEBUG.println(gT.tickVal()); //gTim.poll());
    //gT4.dbgPrint(DEBUG);
  }
} // loop
