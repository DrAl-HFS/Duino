// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb - Mar 2021

//#include "Common/Morse/CMorse.hpp"
#include "Common/AVR/DA_Analogue.hpp"
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

class CMorseDebug : public CMorseTime
{
public :
   CMorseDebug (uint8_t np=25, uint8_t tu=0x20) : CMorseTime(np,tu) { ; }

   bool nextPulse (void) // (Stream& log)
   {
      bool r= CMorseTime::nextPulse();
      static const char dbg[8]={ '\n', 0, '|', ' ' , '0', '.', '-', '3' };
      DEBUG.write( dbg[tc&0x7] ); DEBUG.flush();
      //if (dbgFlag & 0x0C) { DEBUG.print("f:"); DEBUG.println(dbgFlag,HEX); }
      return(r);
   } // nextPulse

   void info (void) // (Stream& s) not working due to inherited CMorseBuff::Stream interface???
   {
     DEBUG.print("msPulse="); DEBUG.println(msPulse);
   } // info

}; // CMorseDebug

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

struct ReadState { uint16_t mm[2], n[4]; };

ReadState rs;
void receive (void)
{
#ifdef DA_ANALOGUE_HPP
  if (gADC.avail())
  {
    int8_t r;
    int8_t n=0;
    uint16_t v, mm[2]={-1,0}, s=0;
    do
    {
      r= gADC.get(v);
      if (0 == r)
      {
        s+= v; ++n;
        if (v < mm[0]) { mm[0]= v; }
        if (v > mm[1]) { mm[1]= v; }
      }
    } while (r >= 0);
    r= 0;
    if (mm[0] < rs.mm[0]) { rs.mm[0]= mm[0]; r|= 0x1; }
    if (mm[1] > rs.mm[1]) { rs.mm[1]= mm[1]; r|= 0x2; }
    //uint16_t pm= ((rs.v[0]+rs.v[1])/2);
    rs.n[ r ]++;
  }
#endif
} // receive

void dumpRS (Stream& s)
{
  s.print("RS:");
  s.print("mm[]:"); s.print(rs.mm[0]); s.print(','); s.println(rs.mm[1]);
  s.print("n[]:"); s.print(rs.n[0]); s.print(','); s.print(rs.n[1]); s.print(','); s.print(rs.n[2]); s.print(','); s.println(rs.n[3]);
  rs.mm[0]= -1; rs.mm[1]= 0;
  rs.n[0]= rs.n[1]= rs.n[2]= rs.n[3]= 0;
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
  
  gS.info(); //DEBUG);
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
  
#ifndef DA_ANALOGUE_HPP
  sleep_cpu();
#endif
} // loop
