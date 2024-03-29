// Duino/SGAD9833/SGAD9833.ino - Arduino IDE & AVR specific main code for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020 - Mar 2021

#include <math.h>

/***/

#define BAUDRATE   115200
#define FSR_1KHZ   0x29F1 // Default signal frequency 1kHz, F_TO_FSR(1E3) = 10737
#define FSR_10KHZ  0x1A36A // F_TO_FSR(1E4)
#define FSR_100KHZ 0x106224 // F_TO_FSR(1E5)
// 1MHz->0xA3D568, 10MHz->0x6665610
// max val in 14LSB: 0x3FFF -> 1.526kHz

/***/
#define CLOCK_INTERVAL 3000

#include "DA_ad9833Mgt.hpp"
#include "Common/AVR/DA_StrmCmd.hpp"
#include "Common/AVR/DA_ClockInstance.hpp"
#ifndef DA_FAST_POLL_TIMER_HPP // resource contention
#include "Common/AVR/DA_Counting.hpp"
#endif
#include "Common/AVR/DA_Analogue.hpp"
#include "Common/AVR/DA_RotEnc.hpp"

#define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK


CRotEncDFR gRotEnc;

StreamCmd gStreamCmd;
DA_AD9833Control gSigGen;
DA_AD9833Chirp gChirp;

#ifdef DA_COUNTING_HPP

CRateEst gRate;

ISR(TIMER1_OVF_vect)  { gRate.event(); }
//ISR(TIMER1_COMPA_vect) { gCount.event(); }

#endif // DA_COUNTING_HPP

#ifdef DA_ANALOGUE_HPP

CAnalogueDbg gADC;

ISR(ADC_vect) { gADC.event(); }

#endif // DA_ANALOGUE_HPP

//#ifdef SAMD_CLOCK_TN
//#endif

char hackCh (char ch) { if ((0==ch) || (ch >= ' ')) return(ch); else return('?'); }

#define SEC_BCD_BYTES 1
int8_t sysLog (Stream& s, uint8_t events)
{
  char str[64];
  int8_t m=sizeof(str)-1, r=0, l=-1, n=0, x=0;
  
#if 0
  str[n++]= 'V';
  n+= hex2ChU8(str+n, events);
  str[n++]= ' ';
#endif
  n+= gClock.getStrHMS(str+n, m-n, ' ');

#ifdef DA_COUNTING_HPP
  n+= snprintf(str+n, m-n, " rate: %u, %u; [%d]\n", gRate.ref.c, gRate.ref.t, gRate.iRes);
  s.println(str);
  n= 0;
  //n+= snprintf(str+n, m-n, " R0: %u, %u", gRate.res[0].c, gRate.res[0].t);
  //n+= snprintf(str+n, m-n, " R1: %u, %u", gRate.res[1].c, gRate.res[1].t);
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

void setup (void)
{
   noInterrupts();
   gClock.setA(__TIME__,5000); // Use build time as default +5.000 sec
   gClock.start();
#ifdef DA_COUNTING_HPP
   gRate.start();
#endif
#ifdef DA_ANALOGUE_HPP
   gADC.init(); gADC.start();
#endif
   gChirp.begin(FSR_100KHZ);  gChirp.end(); // ???

   gRotEnc.init();
   pinMode(PIN_PULSE, OUTPUT);

   gSigGen.fsr= FSR_1KHZ;
   gSigGen.reg.setFSR(gSigGen.fsr, 0);
   gSigGen.reg.setFSR(gSigGen.fsr, 1);
   gSigGen.reg.write(-1);

   Serial.begin(BAUDRATE);
   Serial.println("SGAD9833 " __DATE__ " " __TIME__);
   //sig(Serial);
   //dumpT0(Serial);
   
   interrupts();
   gClock.intervalStart();
   sysLog(Serial,0);
   gSigGen.setGain(gRotEnc.qCount);
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
  uint8_t ev= gClock.update();
  if (ev > 0)
  { // <=1KHz update rate
    if (gClock.intervalDiff() >= -1) { gADC.startAuto(); } else { gADC.stop(); } // mutiple samples, prior to routine sysLog()
    if (gClock.intervalUpdate()) { ev|= 0x80; } //
    if (gRotEnc.update() & 0x30)
    { 
      gRotEnc.dump(gClock.tick,Serial);
      gSigGen.setGain(gRotEnc.qCount);
    }
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
      if (cmd.cmdF[0] & 0x20) // debug
      { cmd.cmdR[0]|= 0x20; }
    }
    if (gSigGen.iFN >= 3)
    {
      gChirp.chirp();
    }
    else
    {
      gSigGen.update(ev&0xF);
    }
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
    //if (0 == gGSC) { pulseHack(); }
    gSigGen.commit(); // send whatever needs sent
  }
} // loop
