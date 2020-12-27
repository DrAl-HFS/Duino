// Duino/Test.ino - Arduino IDE & AVR test harness
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020

#include <math.h>

/***/

#define BAUDRATE   115200


/***/

#define DA_FAST_POLL_TIMER_HPP // resource contention
#include "Common/DA_Timing.hpp"
#include "Common/DA_Analogue.hpp"
#include "Common/DA_StrmCmd.hpp"
#include "Common/DA_SPIMHW.hpp"

#define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK

CClock gClock(3000);

#ifdef AVR_CLOCK_TIMER

#if (2 == AVR_CLOCK_TIMER)
// Connect clock to timer interrupt
// SIGNAL blocks other ISRs - check use of sleep()
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCB_vect) { gClock.nextIvl(); }
#endif // AVR_CLOCK_TIMER

#endif // AVR_CLOCK_TN

#ifdef DA_ANALOGUE_HPP

CAnalogue gADC;

ISR(ADC_vect) { gADC.event(); }

#endif // DA_ANALOGUE_HPP

StreamCmd gStreamCmd;
CmdSeg cmd; // Would be temp on stack but problems arise...


char hackCh (char ch) { if ((0==ch) || (ch >= ' ')) return(ch); else return('?'); }

#define SEC_DIG 1
int8_t sysLog (Stream& s, uint8_t events)
{
  uint8_t msBCD[SEC_DIG];
  char str[64];
  int8_t m=sizeof(str)-1, r=0, l=-1, n=0, x=0;
  
  convMilliBCD(msBCD, SEC_DIG, gClock.tick);
#if 0
  str[n++]= 'V';
  n+= hex2ChU8(str+n, events);
  str[n++]= ' ';
#endif
  n+= gClock.getStrHM(str+n, m-n, ':');
  n+= hex2ChU8(str+n, msBCD[0]); // seconds
#if SEC_DIG > 1
  str[n++]= '.';
  n+= hex2ChU8(str+n, msBCD[1]); // centi-sec
#endif
  str[n]= 0;
#ifdef DA_ANALOGUE_HPP
  l= gADC.avail();
  if (l >= 0)
  {
    n+= snprintf(str+n, m-n, "[%d]", l);
    if (l > ANLG_VQ_MAX) { x|= 0x01; }
    l= -1;
    do
    {
      uint16_t t;
      r= gADC.get(t);
      if (r >= 0)
      {
        l= r+1;
        if (r < 3) { n+= snprintf(str+n, m-n, " A%d=%u", r, t); }
        else
        { 
          int tC;
#if 1     // Device signature calibration: TSOFFSET=FF, TSGAIN=4F ???
          // Looks like available docs are totally wrong on this.
          tC= (int)t - ((int)273+80);
          //tC= (tC * 128) / 0x4F;
          tC= (tC * 207) / 128; // (reciprocated scale - reduce division)
          tC+= 25;
#else
          tC= 25+(((int)t-((int)273+80))*13)/16;
#endif
          n+= snprintf(str+n, m-n, " T%d=%dC", r, tC);
        }
      }
      else if (l < 0) { n+= gADC.dump(str+n, m-n); }
    } while ((r >= 0) && (n < 32));
    if (x & 0x01) { gADC.flush(); }
    if (l >= 0) { gADC.set(l); }
  }
#endif
  s.println(str);
  return(r);
} // sysLog

//void dumpT0 (Stream& s) { s.print("TCNT0="); s.println(TCNT0); }

#include <avr/boot.h>
void sig (Stream& s)  // looks like garbage...
{ // avrdude: Device signature = 0x1e950f (probably m328p)
  char lot[7];
  s.print("sig:");
  for (uint8_t i= 0x0E; i < (6+0x0E); i++)
  { // NB - bizarre endian reversal
    lot[(i-0x0E)^0x01]= boot_signature_byte_get(i);
  }
  lot[6]= 0;
  s.print("lot:");
  s.print(lot);
  s.print(" #");
  s.print(boot_signature_byte_get(0x15)); // W
  s.print("(");
  s.print(boot_signature_byte_get(0x17)); // X
  s.print(",");
  s.print(boot_signature_byte_get(0x16)); // Y
  s.println(")");
  s.flush();
  for (uint8_t i=0; i<0xE; i++)
  { // still doesn't appear to capture everything...
    uint8_t b= boot_signature_byte_get(i);
    if (0xFF == b) { s.println(b,HEX); }
    else { s.print(b,HEX); }
    s.flush();
  }
  // Cal. @ 0x01,03,05 -> 9A FF 4F, (RCOSC, TSOFFSET, TSGAIN)
  // 1E 9A 95 FF FC 4F F2 6F 
  // F9 FF 17 FF FF 59 36 31
  // 34 38 37 FF 13 1A 13 17 
  // 11 28 13 8F FF F
} // sig

DA_SPIMHW rf;
uint8_t rb[4], wb[4];

void setup (void)
{
  noInterrupts();
  gClock.start();
#ifdef DA_ANALOGUE_HPP
  gADC.init(); gADC.start();
#endif
  pinMode(PIN_PULSE, OUTPUT);

  Serial.begin(BAUDRATE);
  Serial.println("Test " __DATE__ " " __TIME__);
  //sig(Serial);
  //dumpT0(Serial);
   
  interrupts();
  gClock.intervalStart();
  sysLog(Serial,0);

  // NRF24 test hack
  rb[0]= rb[1]= 0xFF; 
  wb[0]= 0x07; wb[1]= 0x00;
  int8_t i, nr= rf.readWriteN(rb,wb,2);
  //rf.endTrans();
  for (i=0; i<nr-1; i++) { Serial.print(rb[i],HEX); Serial.print(','); }
  Serial.println(rb[i],HEX);
} // setup

void pulseHack (void)
{ // Caveat: SPI CLK conflict
static uint16_t gHackCount= 0;
  if (((++gHackCount) & 0xFFF) >= 1000) { gHackCount-= 1000; }
  if ((gHackCount & 0xFFF) < 20)// && (0 == gSigGen.rwm))
  {
    //SPI.end(); 
    gHackCount|= 1<<15;
    digitalWrite(PIN_PULSE, HIGH);
  }
  else
  {
    digitalWrite(PIN_PULSE, LOW);
    //if ((0 != gSigGen.rwm) && (gHackCount & 1<<15)) { SPI.begin(); gHackCount &= 0xFFF; }
  }
} // pulseHack

void loop (void)
{
//static uint16_t n=0;  if (0 == n++)  { dumpT0(Serial); }
  uint8_t ev= gClock.update();
  if (ev > 0)
  { // <=1KHz update rate
    if (gClock.intervalDiff() >= -1) { gADC.startAuto(); } else { gADC.stop(); } // mutiple samples, prior to routine sysLog()
    if (gClock.intervalUpdate()) { ev|= 0x80; } // 
    if (gStreamCmd.read(cmd,Serial))
    {
      ev|= 0x40;
      if (cmd.cmdF[0] & 0x10) // clock
      {
        uint8_t hms[3], d;
        d= cmd.v[SCI_VAL_MAX-1].extractHMS(hms,3);
        if (d >= 2)
        { 
          gClock.setHM(hms);
          if (d >= 3) { gClock.setS(hms[2]); }
          cmd.cmdR[0]|= 0x10;
        }
      }
    }

    if (ev & 0xF0)
    {
      sysLog(Serial,ev);
      if (ev & 0x40)
      {
        gStreamCmd.respond(cmd,Serial);
        cmd.clean();
      }
      gClock.intervalStart();
    }
    pulseHack();
  }
} // loop
