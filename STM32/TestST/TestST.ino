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

Timer gTim;
CHackMFRC522 gRC522;

static int8_t n=1;

void setup (void)
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("TestST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  
  gTim.start();
  gRC522.init();
  DEBUG.println(gTim.poll());
} // setup

void loop (void)
{
  if (n > 0)
  {
    n-= gRC522.hack();
    DEBUG.println(gTim.poll());
  }
} // loop
