// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb - Mar 2021

//#include "Common/Morse/CMorse.hpp"
//#include "Common/AVR/DA_Analogue.hpp"
#include "Common/AVR/DA_Timing.hpp"
#include "Common/AVR/DA_Config.hpp"


/***/

#define DEBUG Serial
#define DEBUG_BAUD 115200 
#include "Common/Morse/CMorse.hpp" // DEBUG accessibility problems...

#define SIG_PIN 17  // Uno ADC3 / PC3
#define DET_PIN 14  // Uno ADC0 / PC0
//define TONE_PIN 3 // // NB tone conflicts with Clock (Timer2)


/***/

CClock gClock(5000); // 5.0 sec interval timer
CMorseDebug gS(5);
CDownTimer gMorseDT;
CDownTimer gStreamDT;

#ifdef DA_ANALOGUE_HPP
CAnalogue gADC;
ISR(ADC_vect) { gADC.event(); }
#endif


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

struct ReadState { uint16_t v[2], n[2], r; };

ReadState rs;
void receive (void)
{
  int8_t r;
#ifndef DA_ANALOGUE_HPP
  r= digitalRead(DET_PIN);
  rs.n[r]++;
#else
  int8_t n=0;
  uint16_t v; //[8];
  do
  {
    r= gADC.get(v);
    if (r >= 0)
    {
      if (v < rs.v[0]) { rs.v[0]= v; }
      if (v > rs.v[1]) { rs.v[1]= v; }
      int8_t t= v > ((rs.v[0]+rs.v[1])/2);
      rs.n[ t ]++;
      rs.r++;
    }
  } while (r >= 0);
#endif
} // receive

void dumpRS (Stream& s)
{
  s.print("RS:"); s.println(rs.r);
  s.print("v[0,1]:"); s.print(rs.v[0]); s.print(','); s.println(rs.v[1]);
  s.print("n[0,1]:"); s.print(rs.n[0]); s.print(','); s.println(rs.n[1]);
  rs.v[0]= -1; rs.v[1]= 0;
  rs.n[0]= rs.n[1]= rs.r= 0;
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
  gADC.init(); gADC.startAuto();
#else
  pinMode(DET_PIN, INPUT);
#endif
  
  interrupts();
  DEBUG.print(sid);
  DEBUG.print(" Morse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  gS.send("<SOS> What hath God wrought? <AR>");
  gClock.intervalStart();
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
} // setup

void loop (void)
{
  uint8_t ev= gClock.update();
  if (ev > 0)
  {
    receive(); // 1kHz
    if (gClock.intervalUpdate())
    {
      if (gS.complete())
      { 
        char s[12];
        gClock.getStrHMS(s,sizeof(s)-1);
        DEBUG.print(s); DEBUG.print(' ');
        dumpRS(DEBUG);
        gS.resend(); 
      }
    }
    if (gS.ready() && gMorseDT.update(ev))
    {
      if (gS.nextPulse()) // DEBUG))
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
  
  sleep_cpu();
} // loop
