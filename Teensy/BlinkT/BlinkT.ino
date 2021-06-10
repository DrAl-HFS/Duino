// Duino/Teensy/BlinkT.ino - Teensy 4.1 initial testing
// (using Arduino 1.8.13, required Teensyduino 1.54-beta7 for successful launch)
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Jan - Apr 2021

#include "Common/Teensy/TN_Timing.hpp"

#define DEBUG       Serial
#define DEBUG_BAUD  115200

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

class CMultiIntervalCounter : public CMultiIntervalTimer
{
public:
  uint32_t tock[IVL_COUNT_MAX];
  
  CMultiIntervalCounter (void) : CMultiIntervalTimer() { ; }
  
  uint8_t update (void)
  { 
    uint8_t m= CMultiIntervalTimer::update();
    for (int i=0; i<IVL_COUNT_MAX; i++)
    {
      if (m & (0x2<<i)) { tock[i]++; }
    } 
    return(m);
  }
}; // CMultiIntervalCounter

CMultiIntervalCounter gMIC;
void tickEvent (void) { gMIC.tickEvent(); }

void setup ()
{
  //noInterrupts();
  DEBUG.begin(DEBUG_BAUD);
  pinMode(LED_BUILTIN, OUTPUT);
  //while (!DEBUG) { ; } // forces wait for serial console 
  DEBUG.println("BlinkT " __DATE__ " " __TIME__);
  
  gMIC.init(tickEvent,100);
} // setup

void loop ()
{
  if (gMIC.update() > 0x1)
  {
    digitalWrite(LED_BUILTIN, gMIC.tock[0] & 0x1);
    if (0 == (gMIC.tock[0] & 0xF)) { DEBUG.print('.'); }
    if (0 == (gMIC.tock[0] & 0xFF)) { DEBUG.println(); }
  }
} // loop
