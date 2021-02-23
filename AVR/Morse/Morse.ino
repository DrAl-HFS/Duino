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

uint16_t gSpeed=45;

CClock gClock(15000);
CMorseSSS gS;


/***/

SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }


/***/

void setup()
{
  gClock.setA(__TIME__); // Use build time as default
  gClock.start();

  DEBUG.begin(DEBUG_BAUD);
  
  DEBUG.print("Morse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  pinMode(SIG_PIN, OUTPUT);
  pinMode(TONE_PIN, OUTPUT);
  digitalWrite(SIG_PIN, 0);
#ifdef CMORSE_HPP
  gS.set("What hath God wrought?"); //SOS");
#endif // CMORSE_HPP
}

int16_t gDelayHack=0;
void loop()
{
  uint8_t ev= gClock.update();
  if (ev > 0)
  { 
    if (gClock.intervalUpdate())
    {
      char s[12];
      const int8_t m= sizeof(s)-1;
      int8_t n;
      n= gClock.getStrHM(s,m,':');
      n+= gClock.getStrS(s+n, m-n);
      DEBUG.println(s);
      if (gS.complete()) { gS.reset(); }
    }
    if (gS.ready())
    {
      gDelayHack-= ev;
      if (gDelayHack <= 0)
      {
        if (gS.nextPulse())
        {
          //tone(TONE_PIN, 4000, gS.t*gSpeed - 10);
          digitalWrite(SIG_PIN, gS.v);
          gDelayHack+= gS.t*gSpeed;
        }
        else
        {
          digitalWrite(SIG_PIN, 0);
          DEBUG.write('\n');
          gDelayHack= 0;
        }
      }
    } // ready
  } // ev
} // loop
