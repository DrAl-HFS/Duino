// Duino/Teensy/BlinkT/BlinkT.ino - Teensy 4.1 initial testing
// (using Arduino 1.8.13, required Teensyduino 1.54-beta7 for successful launch)
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Jan - Apr 2021

#define SERIAL_TYPE usb_serial_class  // Teensy
#include "Common/DN_Util.hpp"
#include "Common/Teensy/TN_Timing.hpp"

#define DEBUG       Serial
//define DEBUG_BAUD  115200

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

CMultiIntervalCounter gMIC;
void tickEvent (void) { gMIC.tickEvent(); }

void bootMsg (Stream& s) { s.println("BlinkT " __DATE__ " " __TIME__); }

void setup ()
{
  //noInterrupts();
  pinMode(LED_BUILTIN, OUTPUT);
  if (beginSync(DEBUG)) { bootMsg(DEBUG); } //.begin(DEBUG_BAUD);
  //inter
  
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
