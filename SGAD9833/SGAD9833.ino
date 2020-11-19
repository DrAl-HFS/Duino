// Duino/Common/AD9833.ino - Arduino IDE & AVR specific main code for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#include <math.h>

/***/

#define BAUDRATE  115200
#define FSR_1KHZ  0x29F1 // Default signal frequency 1kHz, F_TO_FSR(1E3) = 10737
// 0xFFFF -> 6.103kHz

/***/

#include "DA_ad9833HW.hpp"
#include "Common/DA_StrmCmd.hpp"
#include "Common/DA_Timing.hpp"
#ifndef AVR_FAST_TIMER // resource contention
#include "Common/DA_Counting.hpp"
#endif

StreamCmd gStreamCmd;
DA_AD9833Control gSigGen;
CClock gClock(15000,AVR_CLOCK_TRIM);

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

CCountExtBase gCount;

//SIGNAL(TIMER1_OVF_vect) { gCount.o++; }
ISR(TIMER1_COMPA_vect) { gCount.c++; }

#endif // DA_COUNTING_HPP

//#ifdef SAMD_CLOCK_TN
//#endif

char hackCh (char ch) { if ((0==ch) || (ch >= ' ')) return(ch); else return('?'); }

void sysLog (Stream& s, uint8_t events)
{
  uint8_t msBCD[3];
  char str[32];
  int8_t m=sizeof(str)-1, n=0;
  
  convMilliBCD(msBCD, 1, gClock.tick);
#if 0
  str[n++]= 'V';
  n+= hex2ChU8(str+n, events);
  str[n++]= ' ';
#endif
  n+= gClock.getStrHM(str+n, m-n, ':');
  n+= hex2ChU8(str+n, msBCD[0]); // seconds
#if 0
  str[n++]= '.';
  n+= hex2ChU8(str+n, msBCD[1]); // centi-sec
#endif
#if 1
  n+= snprintf(str+n, m-n, " S%u,B%u", gSigGen.reg.dbgTransClk, gSigGen.reg.dbgTransBytes);
  gSigGen.reg.dbgTransClk= gSigGen.reg.dbgTransBytes= 0;
#endif
  //n+= snprintf(str+n, m-n, " %uHz", gSigGen.getF());
#ifdef DA_COUNTING_H
  n+= snprintf(str+n, m-n, " C=%u ", gCount.c);
#else
  str[n++]= ' ';
  str[n]= 0;
#endif
  s.println(str);
  gSigGen.changeMon();
} // sysLog


void setup (void)
{
   noInterrupts();
   //const uint8_t cs[]={22,35};
   //gClock.setHM(cs);
   gClock.start();
  
   pinMode(LED_BUILTIN, OUTPUT);

   gSigGen.fsr= FSR_1KHZ;
   gSigGen.reg.setFSR(gSigGen.fsr, 0);
   gSigGen.reg.setFSR(gSigGen.fsr, 1);
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

    //if (ev & 0x40) { Serial.print("1)rwm="); Serial.println(gSigGen.rwm,HEX); }

    //if (lev) { ev|= 0x20; }
    if (gSigGen.resetPending()) { Serial.println("+RST"); } // Reset begins (completes next cycle) 
    else { gSigGen.sweepStep(ev&0xF); }

    //if (ev & 0x40) { Serial.print("2)rwm="); Serial.println(gSigGen.rwm,HEX); }
    
    if (ev & 0xF0)
    { 
      sysLog(Serial,ev);
      if (ev & 0x40)
      {
        gStreamCmd.respond(cmd,Serial);
        cmd.clean();
      }
    }
    gSigGen.commit(); // send whatever needs sent
    //lev^= ev & 0xF0;
  }
} // loop


