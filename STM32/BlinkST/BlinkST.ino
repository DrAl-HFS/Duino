// Duino/STM32/BlinkST/BlinkST.ino - STM32 "Blue Pill" timing test hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1 // PA9/10 (F1xx & F4xx compatible)

#include "Common/Morse/CMorse.hpp"
#include "Common/STM32/ST_Timing.hpp"
#include "Common/STM32/ST_Util.hpp"


/***/

#ifdef ARDUINO_ARCH_STM32F4
//define DEBUG Serial // = instance of USBSerial, same old CDC sync problems

#ifdef VARIANT_blackpill_f401
#define PIN_LED     PC13  // = LED_BUILTIN
#define PIN_BTN     PA0   // ? BTN_BUILTIN ?
#else
#define PIN_LED     PE0
#define PIN_BTN     PD15
#endif // f401

#else
#define PIN_LED PC13
#endif // STM32F4

#define PIN_SIG PB10


/***/


/***/

CClock gClock;
CTimer gT;
void tickFunc (void) { gT.nextIvl(); }
uint16_t gDT; // initial delay

CMorseDebug gS;

/***/


void bootMsg (Stream& s)
{
  s.print("BlinkST " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

void setup (void)
{
  noInterrupts();
  
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);
  pinMode(PIN_SIG, OUTPUT);
  digitalWrite(PIN_SIG, 0);

  gClock.setA(__DATE__,__TIME__);

  interrupts();

  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  
  //gClock.begin();
  gT.start(tickFunc);
  //gT.dbgPrint(DEBUG);
  gS.send("SOS <SOS>" "What hath God wrought?");
  gDT= 1000;
  gClock.print(DEBUG,0x82);
} // setup


void loop (void)
{
  if (gT.diff() >= gDT)
  {
    gT.retire(gDT); // chop or stretch ?...
    if (gS.nextPulse(DEBUG))
    {
      uint8_t v= gS.pulseState();
      digitalWrite(PIN_LED, v);
      digitalWrite(PIN_SIG, v);
      gDT= gS.t;
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
