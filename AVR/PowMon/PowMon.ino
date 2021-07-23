// Duino/AVR/PowMon.ino - Power monitoring / charge controller
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors July 2021

#include <avr/sleep.h>


/***/

#define DEBUG      Serial
#define DEBUG_BAUD 115200

#define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK
#define PIN_DA0 2

#define AVR_INTR_SLOW // reduce interrupt rate for more efficient low-speed control


/***/

#include "Common/AVR/DA_Timing.hpp"
#include "Common/AVR/DA_StrmCmd.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Charge.hpp"


CCharge gCharge;

CClock gClock(1000);

#if (2 == AVR_CLOCK_TIMER)
// Connect clock to timer interrupt
// SIGNAL blocks other ISRs - check use of sleep()
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
#endif // AVR_CLOCK_TIMER

//CAnalogueDbg gADC;
ISR(ADC_vect) { gCharge.event(); }

StreamCmd gStreamCmd;
CmdSeg cmd; // Would be temp on stack but problems arise...


#define SEC_DIG 1
int8_t sysLog (Stream& s, uint8_t events)
{
static uint8_t nLog=0;
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
#if 1
  n+= snprintf(str+n, m-n, " T=%dC", gCharge.sysTC);
  for (int8_t i=0; i <= INST_PNLV; i++)
  {
    n+= snprintf(str+n, m-n, ",%dmV", gCharge.mV[i]);
  }
#else //def DA_ANALOGUE_HPP
  //s.print("DACt=");
  //s.println(gDAC.get());
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
        if (r < 3)
        {
          uint16_t mV= raw2mV(t); // (t + ((t*5)>>5) + 2)*10; // * 1.159 * 10
          n+= snprintf(str+n, m-n, " A%d=%u,(%umV)", r, t, mV);
        }
        else { n+= snprintf(str+n, m-n, " T%d=%dC", r, convTherm(t)); }
      }
      else if (l < 0) { n+= gADC.dump(str+n, m-n); }
    } while ((r >= 0) && (n < 32));
    if (x & 0x01) { gADC.flush(); }
    if (0 == (++nLog & 0x0)) { gADC.next(); }
    //if (l >= 0) { gADC.set(l); }
  }
#endif
  s.println(str);
  return(r);
} // sysLog

void setup (void)
{
  noInterrupts();
  
  //setID("Mega1" / "Nano1" / "UnoV3");
  gClock.setA(__TIME__);
  gClock.start();
  gCharge.init(0); gCharge.start();
  pinMode(PIN_PULSE, OUTPUT);
  pinMode(PIN_DA0, OUTPUT);

  DEBUG.begin(DEBUG_BAUD);
  char sid[8];
  if (getID(sid) > 0) { DEBUG.print(sid); }
  DEBUG.print(" Test " __DATE__ " ");
  DEBUG.println(__TIME__);
  
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  
  interrupts();
  gClock.intervalStart();
  sysLog(DEBUG,0);
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

uint8_t gLastCUD=0, gAV=0x01, gSI=0, gIS=0, gDIS= 2, gLastIS=0;

void loop (void)
{
  uint8_t ev=0;
  if (gClock.update() > 0)
  { // ~100Hz update rate
#if 0
    static uint8_t cycN= 0;
    ++cycN;
    switch (cycN & 0xF)
    {
      case 0 : gCharge.next(); gCharge.startAuto(); set_sleep_mode(SLEEP_MODE_ADC); break;
      case 1 : gCharge.stopAuto(); set_sleep_mode(SLEEP_MODE_IDLE); break;
    }
#endif
    if (gClock.intervalUpdate()) { ev|= 0x80; }
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
    }
    pulseHack();
    gCharge.update();
  }

  digitalWrite(PIN_DA0, (gClock.tick & 0x0400) > 0);
  sleep_cpu();
} // loop
