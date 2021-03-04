// Duino/STM32/BlinkST/BlinkST.ino - STM32 "Blue Pill" timing test hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1

#include "Common/Morse/CMorse.hpp"
#include "Common/STM32/ST_Timing.hpp"

/***/

#define LED PC13
#define SIGNAL PB10
#define DEBUG_BAUD 115200 

uint16_t gSpeed=45;


/***/


/***/

CClock gClock;
CTimer gT;
void tickFunc (void) { gT.nextIvl(); }
uint16_t gDT; // initial delay

CMorseSSS gS;

void setup (void)
{
  noInterrupts();
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("\n--------\n");
  DEBUG.print("BlinkST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  pinMode(SIGNAL, OUTPUT);
  digitalWrite(SIGNAL, 0);

  gClock.setA(__DATE__,__TIME__);

  interrupts();
  //gClock.begin();
  gT.start(tickFunc);
  gT.dbgPrint(DEBUG);
  gS.send("What hath God wrought? <SOS> SOS");
  gDT= 1000;
  gClock.print(DEBUG,0x82);
} // setup

void loop (void)
{
  if (gT.diff() >= gDT)
  {
    gT.retire(gDT); // chop or stretch ?...
    if (gS.nextPulse())
    {
      digitalWrite(LED, gS.v);
      digitalWrite(SIGNAL, gS.v);
      gDT= gS.t*gSpeed;
    }
    else
    {
      //digitalWrite(LED, 0);
      DEBUG.print("\nnIvl="); DEBUG.println(gT.swTickVal());
      gClock.print(DEBUG,0x82);
      gS.resend();
      gDT= 1000;
    }
  }
  //delay(dT);
} // loop
