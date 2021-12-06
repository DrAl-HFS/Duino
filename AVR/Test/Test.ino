// Duino/AVR/Test/Test.ino - Arduino IDE & AVR test harness
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#include <math.h>
//include <avr.h>


/***/

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

//define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK
#define PIN_DA0 2
// 10kHz clock update - 100us (1600 coreClk) interrupt
// Allows up to ~5kHz bandwidth for PWM-DAC signal reconstruction
#define AVR_MILLI_TICKS 10

/***/

//#define DA_FAST_POLL_TIMER_HPP // resource contention
//#define AVR_CLOCK_TRIM 64
#include "Common/AVR/DA_Timing.hpp"
#include "Common/AVR/DA_StrmCmd.hpp"
//include "Common/AVR/DA_SPIMHW.hpp"
#include "Common/CAD779x.hpp"
//#include "Common/CADS1256.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Common/AVR/DA_RotEnc.hpp"
#include "Common/AVR/DA_Util.hpp"

#ifdef DA_SPIMHW_HPP
// Very basic SPI comm test
#define DA_HACKRF24

class HackRF24 : protected DA_SPIMHW
{
   uint8_t rb[4], wb[4];

public:
   HackRF24 (void) { ; }
   
   void test (Stream &s)
   {
      rb[0]= rb[1]= 0xFF; 
      wb[0]= 0x07; wb[1]= 0x00;
      int8_t i, nr= readWriteN(rb,wb,2);
      //rf.endTrans();
      for (i=0; i<nr-1; i++) { s.print(rb[i],HEX); s.print(','); }
      s.println(rb[i],HEX);
   }
}; // class HackRF24

DA_SPIMHW gSPIMHW;
uint8_t gM= SPI_MODE3;
/*    if (ev& 0x80)
    {
      uint8_t w[]={0x1F,0}, r[]={0xAA,0x55};
      gM &= SPI_MODE3;
      gSPIMHW.modeReadWriteN(r,w,2,gM);
      Serial.print("SPI - M");
      Serial.print(gM);
      Serial.print(':');
      Serial.print(r[0],HEX);
      Serial.print(',');
      Serial.println(r[1],HEX);
      gM+= SPI_MODE1;
    }*/
#endif // DA_SPIMHW_HPP


CClock gClock(1000);

#ifdef AVR_CLOCK_TIMER

#if (2 == AVR_CLOCK_TIMER)
// Connect clock to timer interrupt
// SIGNAL blocks other ISRs - check use of sleep()
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCB_vect) { gClock.nextIvl(); }
#endif // AVR_CLOCK_TIMER

#endif // AVR_CLOCK_TN


StreamCmd gStreamCmd;
CmdSeg cmd; // Would be temp on stack but problems arise...

#ifdef CAD779x_HPP
CAD779xDbg ad779x;
#endif
#ifdef CADS1256_HPP
CADS1256Dbg adc;
#endif

#define SEC_DIG 1
int8_t sysLog (Stream& s, uint8_t events)
{
static uint8_t nLog=0;
  uint8_t msBCD[SEC_DIG];
  char str[64];
  int8_t m=sizeof(str)-1, r=0, l=-1, n=0, x=0;
  
  n= gClock.getStrHMS(str,m);
#ifdef DA_ANALOGUE_HPP
  //s.print("DACt=");
  //s.println(gDAC.get());
  l= gADC.avail();
  if (l >= 0)
  {
    n+= snprintf(str+n, m-n, " A[%d]=", l);
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
        else { n+= snprintf(str+n, m-n, " T%d=%dC", r, convTherm(t)); }
      }
      else if (l < 0) { n+= gADC.dump(str+n, m-n); }
    } while ((r >= 0) && (n < 32));
    if (x & 0x01) { gADC.flush(); }
    if (0 == (++nLog & 0xF)) { gADC.next(); }
    //if (l >= 0) { gADC.set(l); }
  }
#endif
  s.println(str);
  return(r);
} // sysLog

//void dumpT0 (Stream& s) { s.print("TCNT0="); s.println(TCNT0); }

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { DEBUG.print(sid); }
  DEBUG.print(" Test " __DATE__ " ");
  DEBUG.println(__TIME__);
} // bootMsg

void setup (void)
{
  noInterrupts();
  
  //setID("ProM1");//"Mega1" / "Nano1" / "UnoV3");
  gClock.setA(__TIME__);
  gClock.start();
#ifdef DA_ANALOGUE_HPP
  gADC.init(1); gADC.start();
  gDAC.init(1); gDAC.set(0);
#endif
  //pinMode(PIN_PULSE, OUTPUT);
  //pinMode(PIN_DA0, OUTPUT);

  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  
  interrupts();
  gClock.intervalStart();
  sysLog(DEBUG,0);
#ifdef DA_ANALOGUE_HPP
  gDS.set(evenTempScale[0]);
  DEBUG.print("gDS.r=");
  DEBUG.println(gDS.rQ8,HEX);
#endif

#ifdef DA_HACKRF24
  HackRF24 hack; hack.test(DEBUG);
#endif
#ifdef CADS1256_HPP
  adc.init(DEBUG);
#endif
#ifdef CAD779x_HPP
  ad779x.reset();
  ad779x.test(DEBUG);
#endif
} // setup

#ifdef PIN_PULSE
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
#endif

uint8_t gCyc=0, gLastCUD=0, gAV=0x01, gSI=0, gIS=0, gDIS= 2, gLastIS=0;

void loop (void)
{
  uint8_t ev=0, cud= gClock.update();
  if (cud > 0) //AVR_MILLI_TICKS)
  { // <=1KHz update rate
    // Pre-collect multiple ADC samples, for pending sysLog()
#ifdef CAD779x_HPP
    ad779x.sample();
#endif
#ifdef DA_ANALOGUE
    if (gClock.intervalDiff() >= -1) { gADC.startAuto(); } else { gADC.stop(); }
#endif
    if (gClock.intervalUpdate())
    { 
      ev|= 0x80;
#ifdef DA_ANALOGUE
      gIS+= majorSS[gSI];
      if (++gSI > 7) { gSI= 0; gIS= 0; }; 
      //gIS+= gDIS;
      //if (gIS > 14) { gIS= 0; gDIS= 16 - gDIS; }
#endif
    }
    if (gStreamCmd.read(cmd,DEBUG))
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
    if (ev)
    {
      sysLog(DEBUG,ev);
      if (ev & 0x40)
      {
        gStreamCmd.respond(cmd,DEBUG);
        cmd.clean();
      }
      gClock.intervalStart();
#ifdef CADS1256_HPP
      adc.logSR(DEBUG);
#endif
#ifdef CAD779x_HPP
      if (ad779x.ready()) { ad779x.report(DEBUG); } else { ad779x.test(DEBUG); }
      ad779x.debugCycle( gCyc++ );
#endif
    }
    //pulseHack();
  }
#ifdef DA_ANALOGUE
#ifdef ARDUINO_AVR_MEGA2560
  if (cud != gLastCUD)
  { // synthesise analogue voltage ramp
    if (gIS != gLastIS)
    { // set tone
      uint16_t fq8= evenTempScale[gIS];
      gDS.set(fq8);
      gLastIS= gIS;
      DEBUG.print("f="); DEBUG.print(fq8/0x100);
      DEBUG.print(" gDS.r="); DEBUG.println(gDS.rQ8,HEX);
    }
    gDS.step();
    gDAC.set(gSin.filterSampleU8(gDS.iQ8), triangle(gAV), gAV<<2);
    //analogWrite(PIN_DA0,gAV); // pin2 = timer3 PWM0
    gAV++;
    gLastCUD= cud;
  }
  //gAV++; 
#else
  digitalWrite(PIN_DA0, (gClock.tick & 0x0400) > 0);
#endif
#endif // DA_ANALOGUE
} // loop
