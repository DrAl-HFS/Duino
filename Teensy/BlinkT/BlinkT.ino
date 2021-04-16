// Duino/Teensy/BlinkT.ino - Teensy 4.1 initial testing
// (required Arduino 1.8.13 + Teensyduino 1.54-beta7 for successful launch)
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Jan - Apr 2021

#include "Common/Teensy/TN_Timing.hpp"

#define DEBUG       Serial
#define DEBUG_BAUD  115200

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

class CIntervalCounter : public CIntervalTimer
{
public:
  uint32_t tock;
  
  CIntervalCounter (void) : CIntervalTimer() { ; }
  
  bool update (void) { bool r= CIntervalTimer::update(); tock+= r; return(r); }
}; // CIntervalCounter

CIntervalCounter gIC;
void tickEvent (void) { gIC.tickEvent(); }

void setup ()
{
  //noInterrupts();
  DEBUG.begin(DEBUG_BAUD);
  pinMode(LED_BUILTIN, OUTPUT);
  while (!DEBUG) { ; } // forces wait for serial console 
  DEBUG.println("BlinkT " __DATE__ " " __TIME__);
  
  gIC.init(tickEvent,100);
} // setup

void loop ()
{
  if (gIC.update())
  {
    digitalWrite(LED_BUILTIN, gIC.tock & 0x1);
    if (0 == (gIC.tock & 0xF)) { DEBUG.print('.'); }
    if (0 == (gIC.tock & 0xFF)) { DEBUG.println(); }
  }
}
