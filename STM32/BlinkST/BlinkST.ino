// Duino/STM32/BlinkST/BlinkST.ino - STM32 "Blue Pill" first hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#define DEBUG Serial1

#include "CMorse.hpp"
#ifndef CMORSE_HPP
#include "morsePattern.h"
#endif // CMORSE_HPP


/***/

#define LED PC13
#define DEBUG_BAUD 115200 

uint16_t gSpeed=100;


/***/

bool send (uint8_t v)
{
static const char mch[]={'.','-','|',' '};
  if (v <= 3)
  {
    DEBUG.write(mch[v]); DEBUG.flush();
    if (v <= 1)
    {
      digitalWrite(LED, 0x1);
      delay(gTimeRelIMC[v]*gSpeed);
      digitalWrite(LED, 0);
      v= 0;
    }
    delay(gTimeRelIMC[v]*gSpeed);
    return(true);
  }
  return(false);
} // send


/***/

const uint16_t msgb=0b000111000;
int8_t i=3, j=3;

void oldHack (void)
{
  if (--j < 0)
  { 
    j= 3;
    if (--i < 0) { i= 3; send(3); }
    else { send(2); }
  } //DEBUG.println(); delay(gTimeRelIMC[3]*TSTEP); }
  else
  {
    int8_t k= i*3+j;
    send((msgb >> k) & 0x1);
  }
} // oldHack


/***/

#ifdef CMORSE_HPP
CMorseSSS gS;
#endif // CMORSE_HPP

void setup()
{
  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("BlinkST " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
#ifdef CMORSE_HPP
  gS.set("SOK");
#endif // CMORSE_HPP
}

void loop()
{
#if 0
  oldHack();
#else
  //if (!send(gS.next())) { gS.reset(); DEBUG.write('\n'); delay(1000); }
  
  if (gS.nextPulse())
  {
    digitalWrite(LED, gS.v);
    delay(gS.t*gSpeed);
  }
  else
  {
    digitalWrite(LED, 0);
    DEBUG.write('\n');
    gS.reset();
    delay(1000);
  }
#endif
}
