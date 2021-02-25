// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#define DEBUG Serial

#include "Common/Morse/CMorse.hpp"
#include "Common/AVR/DA_Timing.hpp"


/***/

#define SIG_PIN 2
//define TONE_PIN 3 // // NB tone conflicts with Clock (Timer2)
#define DEBUG_BAUD 115200 


/***/



uint16_t gSpeed=45; // 11.1dps -> 26~27wpm

CClock gClock(15000); // 15.0 sec interval timer
CMorseSSS gS;
CDownTimer gMorseDT;
CDownTimer gStreamDT;

/***/

SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }


/***/

char procCmd (Stream& s)
{
  char ca[4];
  if (s.available() >= 2)
  {
    ca[0]= toupper(s.read());
    if ('K' == ca[0])
    {
      ca[1]= s.read();
      switch(ca[1])
      {
        case '+' : gClock.addM(1); break;
        case '0' : gClock.setS(0); break;
        case '-' : gClock.addM(-1); break;
        default : return(0);
      }
      ca[2]= ' '; ca[3]= 0;
      s.print(ca); s.println("OK");
    }
  }
  return(0);
} // procCmd

void setup (void)
{
  noInterrupts();

  DEBUG.begin(DEBUG_BAUD);

  pinMode(SIG_PIN, OUTPUT);
  digitalWrite(SIG_PIN, 0);

  gClock.setA(__TIME__,+1750); // Build time +1.75sec
  gClock.start();

  interrupts();
  
  DEBUG.print("\nMorse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  gS.send("What hath God wrought? <SOS> SOS");
  gClock.intervalStart();
  // DEBUG.print("MorsePG->"); DEBUG.println(sizeof(MorsePG));
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
} // setup

void loop (void)
{
  uint8_t ev= gClock.update();
  if (ev > 0)
  { 
    if (gClock.intervalUpdate())
    {
      if (gS.complete())
      { 
        char s[12];
        gClock.getStrHMS(s,sizeof(s)-1);
        DEBUG.print(s); DEBUG.print(' ');
        gS.resend(); 
      }
    }
    if (gS.ready() && gMorseDT.update(ev))
    {
      if (gS.nextPulse())
      {
        digitalWrite(SIG_PIN, gS.v);
        gMorseDT.add(gS.t*gSpeed);
      }
      else
      {
        digitalWrite(SIG_PIN, 0);
        gMorseDT.set(0);
        DEBUG.write('\n');
      }
    } // ready
  } // ev
  else { procCmd(DEBUG); }
  
  sleep_cpu();
} // loop
