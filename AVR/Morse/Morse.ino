// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#define DEBUG Serial

#include "Common/Morse/CMorse.hpp"
#ifndef CMORSE_HPP
#include "Common/Morse/morsePattern.h"
#endif // CMORSE_HPP


/***/

#define SIGNAL 2
#define TONE 3
#define DEBUG_BAUD 115200 


/***/

uint16_t gSpeed=45;


#ifdef CMORSE_HPP
CMorseSSS gS;
#endif // CMORSE_HPP

/***/


void setup()
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("Morse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(SIGNAL, OUTPUT);
  pinMode(TONE, OUTPUT);
  digitalWrite(SIGNAL, 0);
#ifdef CMORSE_HPP
  gS.set("What hath God wrought?"); //SOS");
#endif // CMORSE_HPP
}

void loop()
{
  if (gS.nextPulse())
  {
    tone(TONE, 4000, gS.t*gSpeed - 10);
    digitalWrite(SIGNAL, gS.v);
    delay(gS.t*gSpeed);
  }
  else
  {
    digitalWrite(SIGNAL, 0);
    DEBUG.write('\n');
    gS.reset();
    delay(2000);
  }
}
