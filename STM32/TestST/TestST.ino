// Duino/STM32/TestST/TestST.ino - STM32 test code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

#include "CMifare.hpp"
#include "CTimersST.hpp"


/***/

#define LED PC13
#define DEBUG_BAUD 115200 




/***/

CTimer gT4(4);
CHackMFRC522 gRC522;

uint32_t gLast= 0;
uint32_t gTick= 0;

void tickFunc (void) { ++gTick; }

void setup (void)
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("TestST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  
  gT4.start(tickFunc);

  gRC522.init();
  DEBUG.println(gTick);//gTim.poll());
} // setup

void loop (void)
{
  int32_t d= gTick - gLast;
  if (d >= 1000)
  {
    //gRC522.hack();
    gLast= gTick;
    DEBUG.println(gTick); //gTim.poll());
  }
} // loop
