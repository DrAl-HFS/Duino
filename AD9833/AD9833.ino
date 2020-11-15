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

#include "Common/DA_StrmCmd.hpp"
#include "Common/DA_ad9833.hpp"
#include "Common/DA_Timing.hpp"
#include "Common/DA_Counting.hpp"

StreamCmd gStreamCmd;
DA_AD9833Control gSigGen;
CClock gClock(AVR_CLOCK_TRIM);

// Connect clock to timer interupt
// SIGNAL blocks other ISRs - check use of sleep()
#ifdef AVR_CLOCK_TN

#ifdef AVR_CLOCK_OFLO // vs. OCMP

#if (2 == AVR_CLOCK_TN)
SIGNAL(TIMER2_OVF_vect) { gClock.nextIvl(); }
#endif // (0 == AVR_TN)
#if (0 == AVR_CLOCK_TN)
SIGNAL(TIMER0_OVF_vect) { gClock.nextIvl(); }
#endif // (0 == AVR_TN)

#if (1 == AVR_CLOCK_TN)
SIGNAL(TIMER1_OVF_vect) { gClock.nextIvl(); }
#endif // (1 == AVR_TN)

#else // OFLO vs. OCMP

#if (2 == AVR_CLOCK_TN)
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCB_vect) { gClock.nextIvl(); }
#endif

#endif // OFLO vs. OCMP

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

void setup (void)
{
   noInterrupts();
   //sei();

   //const uint8_t cs[]={19,18};
   //gClock.setHM(cs);
   gClock.start();
  
   pinMode(LED_BUILTIN, OUTPUT);

   gSigGen.reg.setFSR(FSR_1KHZ, 0);
   gSigGen.reg.setFSR(FSR_1KHZ, 1);
   gSigGen.reg.write();
   Serial.println("+RST"); // default flags include reset

   Serial.begin(BAUDRATE);
   Serial.println("SigGen " __DATE__ " " __TIME__);
   //Serial.println(r);
   interrupts();//SEI;
   //cli();
} // setup

void sysLog (uint8_t d)
{
static uint8_t msP=-1;
   uint8_t msBCD[3];
   int8_t n;
   char str[32];
  
   if (d > 0)
   {
      convMilliBCD(msBCD, 1, gClock.tick);

      if (((0xF0 & msBCD[0]) != (0xF0 & msP)))
      {
         msP= msBCD[0];
         n= gClock.getStrHM(str, sizeof(str)-1, ':');
         n+= hex2ChU8(str+n, msBCD[0]); // seconds
#if 0
         str[n++]= '.';
         n+= hex2ChU8(str+n, msBCD[1]); // centi-sec
#endif
         //n+= snprintf(str+n, sizeof(str)-n, " %uHz", gSigGen.getF());
#ifdef DA_COUNTING_H
         n+= snprintf(str+n, sizeof(str)-n, " C=%u ", gCount.c);
#else
         str[n++]= ' ';
         str[n]= 0;
#endif
         Serial.print(str);
         gSigGen.changeMon(true);
      }
   }
} // sysLog

CmdSeg cmd; // Would be temp on stack but problems arise...

void loop (void)
{
  uint8_t d= gClock.update();
  if (d > 0)
  { // <=1KHz update rate
    if (gSigGen.resetClear()) { Serial.println("-RST"); } // Previously started reset completes
    if (gStreamCmd.read(cmd,Serial))
    { 
      gSigGen.apply(cmd);
    }
    if (gSigGen.resetPending()) { Serial.println("+RST"); } // Reset begins (completes next cycle) 
    else { gSigGen.sweepStep(d); }
    
    sysLog(d);
    gSigGen.commit(); // send whatever needs sent
  }
} // loop


