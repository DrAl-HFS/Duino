// Duino/STM32/BlinkST/BlinkST.ino - STM32 "Blue Pill" first hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

#include "Common/Morse/CMorse.hpp"
#include "Common/STM/ST_Timing.hpp"


/***/

#define LED PB10
#define DEBUG_BAUD 115200 

uint16_t gSpeed=45;


/***/


/***/

CTimer gT;
void tickFunc (void) { gT.nextIvl(); }
uint16_t gDT; // initial delay

CMorseSSS gS;

void setup (void)
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("BlinkST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);

  gT.start(tickFunc);
  gT.dbgPrint(DEBUG);
  gS.send("What hath God wrought? <SOS> SOS");
  gDT= 1000;
} // setup

void loop (void)
{
  if (gT.diff() >= gDT)
  {
    gT.retire(gDT); // chop or stretch ?...
    if (gS.nextPulse())
    {
      digitalWrite(LED, gS.v);
      gDT= gS.t*gSpeed;
    }
    else
    {
      digitalWrite(LED, 0);
      DEBUG.print("\nnIvl="); DEBUG.println(gT.swTickVal());
      gS.resend();
      gDT= 1000;
    }
  }
  //delay(dT);
} // loop
