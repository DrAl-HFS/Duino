// Duino/Test.ino - Arduino IDE & AVR test harness
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#include <math.h>
//include <avr.h>


/***/

#define DEBUG      Serial
#define DEBUG_BAUD 115200

#define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK
#define PIN_DA0 2
// 10kHz clock update - 100us (1600 coreClk) interrupt
// Allows up to ~5kHz bandwidth for PWM-DAC signal reconstruction
#define AVR_MILLI_TICKS 10

/***/

//#define DA_FAST_POLL_TIMER_HPP // resource contention
//#define AVR_CLOCK_TRIM 64
#include "Common/AVR/DA_Timing.hpp"
#include "Common/AVR/DA_Analogue.hpp"
#include "Common/AVR/DA_StrmCmd.hpp"
//include "Common/AVR/DA_SPIMHW.hpp"


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

CAnalogueDbg gADC;
CFastPulseDAC gDAC;

ISR(ADC_vect) { gADC.event(); }

#endif // DA_ANALOGUE_HPP


StreamCmd gStreamCmd;
CmdSeg cmd; // Would be temp on stack but problems arise...

#include <EEPROM.h>

int8_t setID (char idzs[])
{
  int8_t i=0;
  if (EEPROM.read(i) != idzs[i])
  {
    EEPROM.write(i,idzs[i]);
    if (0 != idzs[i++])
    {
      do
      {
        if (EEPROM.read(i) != idzs[i]) { EEPROM.write(i,idzs[i]); }
      } while (0 != idzs[i++]);
    }
  }
  return(i);
} // setID

int8_t getID (char idzs[])
{
  int8_t i=0;
  do
  {
    idzs[i]= EEPROM.read(i);
  } while (0 != idzs[i++]);
  return(i);
} // getID

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
  s.print("DACt=");
  s.println(gDAC.get());
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


void setup (void)
{
  noInterrupts();
  
  //setID("Mega1" / "Nano1" / "UnoV3");
  gClock.setA(__TIME__);
  gClock.start();
#ifdef DA_ANALOGUE_HPP
  gADC.init(); gADC.start();
  gDAC.init(1); gDAC.set(0);
#endif
  pinMode(PIN_PULSE, OUTPUT);
  pinMode(PIN_DA0, OUTPUT);

  DEBUG.begin(DEBUG_BAUD);
  char sid[8];
  if (getID(sid) > 0) { DEBUG.print(sid); }
  DEBUG.print(" Test " __DATE__ " ");
  DEBUG.println(__TIME__);
  //sig(DEBUG);
  //dumpT0(DEBUG);
   
  interrupts();
  gClock.intervalStart();
  sysLog(DEBUG,0);

#ifdef DA_HACKRF24
  HackRF24 hack; hack.test(DEBUG);
#endif
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

uint8_t gLastEV=0, gAV=0x01;

void loop (void)
{
  uint8_t ev= gClock.update();
  if (ev > AVR_MILLI_TICKS)
  { // <=1KHz update rate
    // Pre-collect multiple ADC samples, for pending sysLog()
    if (gClock.intervalDiff() >= -1) { gADC.startAuto(); } else { gADC.stop(); }
    if (gClock.intervalUpdate()) { ev|= 0x80; } // 
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
    if (ev & 0xF0)
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
  }
#ifdef ARDUINO_AVR_MEGA2560
  if (ev != gLastEV)
  { // synthesise analogue voltage ramp
    gDAC.set(gAV, gAV<<1, gAV<<2);
    //analogWrite(PIN_DA0,gAV); // pin2 = timer3 PWM0
    gAV++;
    gLastEV= ev;
  }
  //gAV++; 
#else
  //digitalWrite(PIN_DA0, gClock.tick & 0x1);
#endif
} // loop
