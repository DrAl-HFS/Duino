// Duino/Common/AD9833.ino - Arduino IDE & AVR specific main code for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov-Dec 2020

#include <math.h>

/***/

#define BAUDRATE  115200
#define FSR_1KHZ  0x29F1 // Default signal frequency 1kHz, F_TO_FSR(1E3) = 10737
// 10kHz->0x1A36A, 100kHz->0x106224, 1MHz->0xA3D568, 10MHz->0x6665610
// max val in 14LSB: 0x3FFF -> 1.526kHz

/***/

#include "DA_ad9833HW.hpp"
#include "Common/DA_StrmCmd.hpp"
#include "Common/DA_Timing.hpp"
#ifndef DA_FAST_POLL_TIMER_HPP // resource contention
#include "Common/DA_Counting.hpp"
#endif
#include "Common/DA_Analogue.hpp"

#define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK

StreamCmd gStreamCmd;
DA_AD9833Control gSigGen;
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

#ifdef DA_COUNTING_HPP

CRateEst gRate;

ISR(TIMER1_OVF_vect)  { gRate.event(); }
//ISR(TIMER1_COMPA_vect) { gCount.event(); }

#endif // DA_COUNTING_HPP

#ifdef DA_ANALOGUE_HPP

CAnalogue gADC;

ISR(ADC_vect) { gADC.event(); }

#endif // DA_ANALOGUE_HPP

//#ifdef SAMD_CLOCK_TN
//#endif

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
#if 0 //def DA_COUNTING_HPP
  n+= snprintf(str+n, m-n, " Ref: %u, %u; [%d]", gRate.ref.c, gRate.ref.t, gRate.iRes);
  n+= snprintf(str+n, m-n, " R0: %u, %u", gRate.res[0].c, gRate.res[0].t);
  n+= snprintf(str+n, m-n, " R1: %u, %u", gRate.res[1].c, gRate.res[1].t);
/*
  if (events & 0x20)
  {
    const SRate& r= gRate.get();
    n+= snprintf(str+n, m-n, " %uHz\n", ((uint32_t)r.c * 1000) / r.t);
  }
*/
#endif
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
#ifdef DA_FAST_POLL_TIMER_HPP
  n+= snprintf(str+n, m-n, " S%u,B%u", gSigGen.reg.dbgTransClk, gSigGen.reg.dbgTransBytes);
  gSigGen.reg.dbgTransClk= gSigGen.reg.dbgTransBytes= 0;
#endif // DA_FAST_POLL_TIMER_HPP
  s.println(str);
  //n+= snprintf(str+n, m-n, " %uHz", gSigGen.getF());
  gSigGen.changeMon();
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

void setup (void)
{
   noInterrupts();
   //const uint8_t cs[]={22,18};
   //gClock.setHM(cs);
   gClock.start();
#ifdef DA_COUNTING_HPP
   gRate.start();
#endif
#ifdef DA_ANALOGUE_HPP
   gADC.init(); gADC.start();
#endif
   
   pinMode(PIN_PULSE, OUTPUT);

   gSigGen.fsr= FSR_1KHZ;
   gSigGen.reg.setFSR(gSigGen.fsr, 0);
   gSigGen.reg.setFSR(gSigGen.fsr, 1);
   gSigGen.reg.write(-1);

   Serial.begin(BAUDRATE);
   Serial.println("SigGen " __DATE__ " " __TIME__);
   sig(Serial);
   //dumpT0(Serial);
   
   interrupts();
   gClock.intervalStart();
   sysLog(Serial,0);
} // setup

CmdSeg cmd; // Would be temp on stack but problems arise...

void pulseHack (void)
{ // SPI CLK conflict
static uint16_t gHackCount= 0;
  if (((++gHackCount) & 0xFFF) >= 1000) { gHackCount-= 1000; }
  if (((gHackCount & 0xFFF) < 20) && (0 == gSigGen.rwm))
  {
    SPI.end(); gHackCount|= 1<<15;
    digitalWrite(PIN_PULSE, HIGH);
  }
  else
  {
    digitalWrite(PIN_PULSE, LOW);
    if ((0 != gSigGen.rwm) && (gHackCount & 1<<15)) { SPI.begin(); gHackCount &= 0xFFF; }
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
      gSigGen.apply(cmd);
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

    gSigGen.update(ev&0xF);
    if (gRate.update(gClock.tick) > 0) { ev|= 0x2; }

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
    gSigGen.commit(); // send whatever needs sent
  }
} // loop


