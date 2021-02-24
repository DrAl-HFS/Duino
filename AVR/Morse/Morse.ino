// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#define DEBUG Serial

#include "Common/Morse/CMorse.hpp"
#include "Common/AVR/DA_Timing.hpp"


/***/

#define SIG_PIN 2
#define TONE_PIN 3
#define DEBUG_BAUD 115200 


/***/

// NB tone conflicts with Clock (Timer2)

uint16_t gSpeed=45; // 11.1dps -> 26~27wpm

CClock gClock(15000);
CMorseSSS gS;
CDownTimer gDT;

/***/

SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }


/***/

void setup()
{
  noInterrupts();
  gClock.setA(__TIME__,5000); // Build time +5.0sec
  gClock.start();

  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("\nMorse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(SIG_PIN, OUTPUT);
  pinMode(TONE_PIN, OUTPUT);
  digitalWrite(SIG_PIN, 0);
#ifdef CMORSE_HPP
  gS.set("What hath God wrought?"); //SOS");
#endif // CMORSE_HPP
  interrupts();
  gClock.intervalStart();
}

void loop()
{
  uint8_t ev= gClock.update();
  if (ev > 0)
  { 
    if (gClock.intervalUpdate())
    {
      char s[12];
      gClock.getStrHMS(s,sizeof(s)-1);
      DEBUG.print(s); DEBUG.print(' ');
      if (gS.complete()) { gS.reset(); }
    }
    if (gS.ready() && gDT.update(ev))
    {
      if (gS.nextPulse())
      {
        digitalWrite(SIG_PIN, gS.v);
        gDT.add(gS.t*gSpeed);
      }
      else
      {
        digitalWrite(SIG_PIN, 0);
        DEBUG.write('\n');
      }
    } // ready
  } // ev
} // loop
