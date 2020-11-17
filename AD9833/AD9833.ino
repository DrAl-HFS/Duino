// Duino/Common/AD9833.ino - Arduino IDE & AVR specific main code for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#include <math.h>

/***/

#define BAUDRATE  115200
#define FSR_1KHZ  10737 // Default signal frequency = F_TO_FSR(1E3) = 1kHz


/***/

#include "DA_ad9833HW.hpp"
#include "Common/DA_StrmCmd.hpp"
#include "Common/DA_Timing.hpp"
#ifndef AVR_FAST_TIMER // resource contention
#include "Common/DA_Counting.hpp"
#endif

StreamCmd gStreamCmd;
DA_AD9833Control gSigGen;
CClock gClock(3000,AVR_CLOCK_TRIM);

// Connect clock to timer interupt
// SIGNAL blocks other ISRs - check use of sleep()
#ifdef AVR_CLOCK_TIMER

#if (2 == AVR_CLOCK_TIMER)
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCB_vect) { gClock.nextIvl(); }
#endif // AVR_CLOCK_TIMER

#endif // AVR_CLOCK_TN

#ifdef DA_COUNTING_H

CCountExtBase gCount;

//SIGNAL(TIMER1_OVF_vect) { gCount.o++; }
ISR(TIMER1_COMPA_vect) { gCount.c++; }

#endif // DA_COUNTING_H

//#ifdef SAMD_CLOCK_TN
//#endif

/*
void setSigGen (uint32_t fsr)
{
   reg2.ctrl.u16=         (0x0<<14) | AD_F_B28|AD_F_RST;
   reg2.fpr[0].fr[0].u16= (0x1<<14) | (AD_FS_MASK & fsr);
   reg2.fpr[0].fr[1].u16= (0x1<<14) | (AD_FS_MASK & (fsr>>14));
   reg2.fpr[0].pr.u16=    (0x6<<13) | (AD_PS_MASK & psr);
   reg2.fpr[1].fr[0].u16= (0x2<<14) | (AD_FS_MASK & fsr);
   reg2.fpr[1].fr[1].u16= (0x2<<14) | (AD_FS_MASK & (fsr>>14));
   reg2.fpr[1].pr.u16=    (0x7<<13) | (AD_PS_MASK & psr);
   reg2.ctrl.u16^= AD_F_RST;
   while ((reg.b[r] == reg2.b[r]) && (r < 14)) { ++r; }
} // setSigGen
*/

void sysLog (Stream& s, uint8_t events)
{
  uint8_t msBCD[3];
  char str[32];
  int8_t m=sizeof(str)-1, n=0;
  
  convMilliBCD(msBCD, 1, gClock.tick);
/*
  { // DEPRECATE
static uint8_t msP=-1;
    if (((0x0F & msBCD[0]) == (0x0F & msP))) return;
    msP= msBCD[0];
  }
*/
  str[n++]= 'V';
  n+= hex2ChU8(str+n, events);
  str[n++]= ' ';
  n+= gClock.getStrHM(str+n, m-n, ':');
  n+= hex2ChU8(str+n, msBCD[0]); // seconds
#if 0
  str[n++]= '.';
  n+= hex2ChU8(str+n, msBCD[1]); // centi-sec
#endif
  n+= snprintf(str+n, m-n, " S%u,B%u,%04X", gSigGen.reg.dbgTransClk, gSigGen.reg.dbgTransBytes, gSigGen.reg.last);
  gSigGen.reg.dbgTransClk= gSigGen.reg.dbgTransBytes= 0;
  //n+= snprintf(str+n, m-n, " %uHz", gSigGen.getF());
#ifdef DA_COUNTING_H
  n+= snprintf(str+n, m-n, " C=%u ", gCount.c);
#else
  str[n++]= ' ';
  str[n]= 0;
#endif
  s.print(str);
  gSigGen.changeMon(true);
} // sysLog


void setup (void)
{
   noInterrupts();
   //const uint8_t cs[]={19,18};
   //gClock.setHM(cs);
   gClock.start();
  
   pinMode(LED_BUILTIN, OUTPUT);

   gSigGen.reg.setFSR(FSR_1KHZ, 0);
   gSigGen.reg.setFSR(FSR_1KHZ, 1);
   gSigGen.reg.write(-1);

   Serial.begin(BAUDRATE);
   Serial.println("SigGen " __DATE__ " " __TIME__);
   Serial.println("+RST"); // default flags include reset
   interrupts();
   gClock.intervalStart();
   sysLog(Serial,0);
} // setup

CmdSeg cmd; // Would be temp on stack but problems arise...

void loop (void)
{
//static uint8_t lev;//=0
  uint8_t ev= gClock.update();
  if (ev > 0)
  { // <=1KHz update rate
    if (gClock.intervalUpdate()) { ev|= 0x80; }
    if (gSigGen.resetClear()) { Serial.println("-RST"); } // Previously started reset completes
    if (gStreamCmd.read(cmd,Serial))
    {
      ev|= 0x40;
      gSigGen.apply(cmd);
    }
    //if (lev) { ev|= 0x20; }
    if (gSigGen.resetPending()) { Serial.println("+RST"); } // Reset begins (completes next cycle) 
    else { gSigGen.sweepStep(ev&0xF); }
    
    if (ev & 0xF0) { sysLog(Serial,ev); }
    gSigGen.commit(); // send whatever needs sent
    //lev^= ev & 0xF0;
  }
} // loop


