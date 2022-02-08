// Duino/AVR/Morse/Morse.ino - Morse code generation
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Feb - Mar 2021

#include "Common/Morse/CMorse.hpp"
#include "Common/AVR/DA_Analogue.hpp"
#include "Common/AVR/DA_Config.hpp"

#define CLOCK_INTERVAL 5000
#include "Common/AVR/DA_ClockInstance.hpp"

/***/

#define DEBUG Serial
#define DEBUG_BAUD 115200 
#ifndef CMORSE_HPP
#include "Common/Morse/CMorse.hpp" // DEBUG accessibility problems...
#endif


#define SIG_PIN 17  // Uno ADC3 / PC3
#define DET_PIN 14  // Uno ADC0 / PC0
#define SQW_PIN 6   // Uno OC0A / PD6
//define TONE_PIN 3 // // NB tone conflicts with Clock (Timer2)


class CSqWGen
{
public:
  CSqWGen (void) { ; }
  
  void init (void) // Uno Timer0 (beware ARDUINO API conflict)
  {
    DDRD|= 1<<6; // OC0A 
    //pinMode(SQW_PIN, OUTPUT);
    TCCR0A= 0x43; // Fast PWM -> OC0A
    //TCCR0B= 0x03; // 16MHz/64 = 256kHz - It seems Arduino boot relies on this value
    // 0x0A = 16/8= 2MHz  OCR0A= 9  flip 5us -> 10us period -> 100kHz
    TCCR0B= 0x0B; 
    OCR0A=  1; // 256kHz / (1+n) -> 128kHz
  }
}; // CSqWGen

/***/


/***/

//CClock gClock(5000); // 5.0 sec interval timer
//SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
CMorseDebug gS;
CDownTimer gMorseDT;
//CDownTimer gStreamDT;
CSqWGen gSqW;

CAnalogue gADC;
ISR(ADC_vect) { gADC.event(); gADC.start(); } // deferred restart reduces rate slightly




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

struct ReadState { int8_t s[32], mm[2], t; uint8_t iS, iW; uint16_t tn; uint32_t w[8]; };

int16_t acf (int8_t w0[], int8_t w1[], int8_t n)
{
  //if (n <= 0) { return(0); }
  int16_t s= w0[0] * w1[0];
  for (int8_t i=1; i<n; i++) { s+= w0[i] * w1[i]; }
  return(s);
} // acf

ReadState rs;
void receive (void)
{
  if (gADC.avail())
  {
const int16_t calOffs= 210;
    uint16_t v;
    int8_t w[10], r, n=0;
    do
    {
      r= gADC.get(v);
      if (0x1 == r)// channel1, sensitivity ~1.07mV (A0/1.1V ref)
      {
        w[n]= (v-calOffs);
        n++;
      }
    } while ((r >= 0) && (n < 10));
    if (n >= 8)
    {
      rs.s[rs.iS]= acf(w,w+4,4) / 4;
      rs.mm[0]= min(rs.mm[0], rs.s[rs.iS]);
      rs.mm[1]= max(rs.mm[1], rs.s[rs.iS]);
      rs.tn+= n;
      if (++(rs.iS) >= 32)
      {
        int8_t t= (rs.mm[0] + rs.mm[1]) / 2;
        rs.t= max(rs.t, t);
        uint32_t w= 0;
        for (int8_t i=0; i<32; i++) { w= (w << 1) | (rs.s[i] > rs.t); }
        rs.iW= (rs.iW + 1) & 0x7;
        rs.w[rs.iW]= w;
        rs.iS&= 0x1F;
      }
    }
  }
} // receive

void dumpRS (Stream& s)
{
  s.print("RS:"); s.print("["); s.print(rs.tn); s.print("] mm={"); 
  s.print(rs.mm[0]); s.print(','); s.print(rs.mm[1]); s.print("} ");
  for (int8_t i=0; i<8; i++) { s.print(rs.w[i],HEX); }
  rs.iS= rs.iW= rs.tn= 0;
  rs.mm[0]= 0x7F; rs.mm[1]= 0x80;
} // dumpRS

void setup (void)
{
  char sid[8];
  noInterrupts();

  DEBUG.begin(DEBUG_BAUD);

  getID(sid);

  pinMode(SIG_PIN, OUTPUT);
  digitalWrite(SIG_PIN, 0);

  gSqW.init();
  
  gClock.setA(__TIME__,+7500); // Build time +7.5sec
  gClock.start();

#ifdef DA_ANALOGUE_HPP
  gADC.init(); gADC.set(1); gADC.start(); //Auto();
#else
  pinMode(DET_PIN, INPUT);
#endif
  
  interrupts();
  DEBUG.print(sid);
  DEBUG.print(" Morse " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  DEBUG.print("T0:"); DEBUG.print(TCCR0A,HEX); DEBUG.print(','); DEBUG.print(TCCR0B,HEX); DEBUG.print(','); DEBUG.println(OCR0A,HEX); 
  
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
        gClock.printHMS(DEBUG,' ');
        gS.resend(); 
      }
    }
    //else if (gClock.intervalDiff() == -500) { dumpRS(DEBUG); }
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
