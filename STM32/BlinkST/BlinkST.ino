// Duino/STM32/BlinkST/BlinkST.ino - STM32 "Blue Pill" first hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

#include "Common/Morse/CMorse.hpp"


/***/

#define LED PC13
#define DEBUG_BAUD 115200 

uint16_t gSpeed=45;


/***/


/***/

CMorseSSS gS;

void setup (void)
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("BlinkST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  gS.send("SOS");
} // setup

void loop (void)
{
  if (gS.nextPulse())
  {
    digitalWrite(LED, gS.v);
    delay(gS.t*gSpeed);
  }
  else
  {
    digitalWrite(LED, 0);
    DEBUG.write('\n');
    gS.resend();
    delay(1000);
  }
} // loop
