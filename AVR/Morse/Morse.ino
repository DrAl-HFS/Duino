// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb - Mar 2021

#include "Common/Morse/CMorse.hpp"
#include "Common/AVR/DA_Analogue.hpp"
#include "Common/AVR/DA_Timing.hpp"
#include "Common/AVR/DA_Config.hpp"


/***/

#define DEBUG Serial
#define DEBUG_BAUD 115200 
#ifndef CMORSE_HPP
#include "Common/Morse/CMorse.hpp" // DEBUG accessibility problems...
#endif

#define SIG_PIN 17  // Uno ADC3 / PC3
#define DET_PIN 14  // Uno ADC0 / PC0
//define TONE_PIN 3 // // NB tone conflicts with Clock (Timer2)

/***/


/***/

CClock gClock(5000); // 5.0 sec interval timer
CMorseDebug gS;
CDownTimer gMorseDT;
//CDownTimer gStreamDT;

CAnalogue gADC;
ISR(ADC_vect) { gADC.event(); }


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

#define BATCH 32
struct ReadState { uint16_t s[BATCH]; uint16_t i, n; };

ReadState rs;
void receive (void)
{
#ifdef DA_ANALOGUE_HPP
  if (gADC.avail())
  {
    uint16_t v, s=0;
    int8_t r;
    int8_t n=0, m=0;
    do
    {
      r= gADC.get(v);
      if (0x1 == r)// channel1, sensitivity ~1.07mV (1.1V ref vs 10bits)
      {
        s+= v;
        n++;
        m+= (v <= 0);
      }
    } while ((r >= 0) && (n < 10));
    if (n > m)
    {
      rs.s[rs.i & 0x1F]= s; ///(n-m);
      rs.n+= n;
      rs.i++;
    }
  }
#endif
} // receive

void dumpRS (Stream& s)
{
  s.print("RS:"); s.print("["); s.print(rs.n); s.print("]=");
  int8_t n= min(BATCH, rs.n);
  for (int8_t i=0; i<n; i++)
  {
    s.print(rs.s[i]); s.print(',');
  }
  s.println();
  rs.i= rs.n= 0;
} // dumpRS

void setup (void)
{
  char sid[8];
  noInterrupts();

  DEBUG.begin(DEBUG_BAUD);

  getID(sid);

  pinMode(SIG_PIN, OUTPUT);
  digitalWrite(SIG_PIN, 0);

  gClock.setA(__TIME__,+7500); // Build time +7.5sec
  gClock.start();

#ifdef DA_ANALOGUE_HPP
  gADC.init(); gADC.set(1); gADC.startAuto();
#else
  pinMode(DET_PIN, INPUT);
#endif
  
  interrupts();
  DEBUG.print(sid);
  DEBUG.print(" Morse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  gS.info(DEBUG);
  gS.send("<SOS> What hath God wrought? <AR>");
  gClock.intervalStart();
#ifndef DA_ANALOGUE_HPP
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
#endif
} // setup

uint8_t lastEV=0;

void loop (void)
{
  uint8_t ev= gClock.update();
  if (ev > 0)
  {
    //if (lastEV != ev) { lastEV= ev; DEBUG.print("ev"); DEBUG.print(ev); DEBUG.print(" t"); DEBUG.println(gClock.tick); }
    receive(); // 1kHz
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
    else if (gClock.intervalDiff() == -2500) { dumpRS(DEBUG); }
    if (gS.ready() && gMorseDT.update(ev))
    {
      if (gS.nextPulse(DEBUG))
      {
        digitalWrite(SIG_PIN, gS.pulseState());
        gMorseDT.add(gS.t);
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
  
#ifndef DA_ANALOGUE_HPP
  sleep_cpu();
#endif
} // loop
