// Duino/Common/N5/N5_Timing.hpp - nRF5* (Micro:Bit V1 & V2 nRF51822/52833) timing utilities
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan - June 2021

#ifndef N5_TIMING
#define N5_TIMING

#include "N5_Util.hpp"
#include "../M0_Util.hpp" // Aaargh!

#define TIMER_ROLLING
//define TIMER_CC_IDX 3
#define TIMER_CC_STEP (16000)   // 1ms @ 16MHz (HFCLK not core clk)
#define RTC_PRESCALE_MAX ((1<<12)-1)


class CTimerN5
{
protected:
   uint32_t tickCount; // 49.7 days wrap-around at 1ms tick

   void setup (NRF_TIMER_Type *pT=NRF_TIMER2, uint32_t tickCount=TIMER_CC_STEP)
   {
      pT->MODE = TIMER_MODE_MODE_Timer;
      pT->PRESCALER= 0;   // HFCLK 16MHz 62.5nSec tick
      pT->TASKS_CLEAR= 1; // clean reset
      pT->BITMODE= TIMER_BITMODE_BITMODE_32Bit; // wrap at 256 sec
      pT->CC[3]= tickCount-1;   // 1ms

      // CC[3] auto clear yields periodic 0..9999 
#ifndef TIMER_ROLLING
      pT->SHORTS=   TIMER_SHORTS_COMPARE3_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
#endif
      // interrupt on CC[3] match
      pT->INTENSET= (TIMER_INTENSET_COMPARE3_Enabled << TIMER_INTENSET_COMPARE3_Pos);
   } // setup
public:
   CTimerN5 (void) {;}

   void tickEvent (NRF_TIMER_Type *pT=NRF_TIMER2)
   {
#ifdef TIMER_ROLLING
      pT->CC[3]+= TIMER_CC_STEP; // rolling mode allows longer term timing measurements using capture registers
#endif
      tickCount++; 
   }
   
   uint32_t getTick (void) { return(tickCount); }
   uint32_t getTickHW (int8_t i=0, NRF_TIMER_Type *pT=NRF_TIMER2)
   {
      if (tickCapture(i,pT) >= 0) { return(pT->CC[i]); }
      //else
      return(0);
   } // getTickHW
   
   void tickOvfloEvent (NRF_TIMER_Type *pT=NRF_TIMER2) { ; }
   
   int8_t tickCapture (int8_t i, NRF_TIMER_Type *pT=NRF_TIMER2) // { return(pT->COUNTER); } ???
   {  // NB CC[3] used to trigger tick interrupt - capturing to this is a VERY bad idea
      if (i >= 3) { return(-1); }
      pT->TASKS_CAPTURE[i]= 1; // sync ??
      return(i);
   } // tickCapture
   
   uint32_t tickCaptureInterval (int8_t i1, int8_t i2, NRF_TIMER_Type *pT=NRF_TIMER2)
   {
      uint32_t r= 0;
      if ((i1 < 3) && (i2 < 3))
      {  // overflow wrap check
         if (pT->CC[i2] >= pT->CC[i1]) { r= pT->CC[i2] - pT->CC[i1]; }
         else { r= pT->CC[i2] + 1 + (0xFFFFFFFF - pT->CC[i1]); } // form 32b complement 
      }
      return(r);
   } // tickCaptureInterval
   
   uint32_t printTCI (Stream& s, int8_t i1=1, int8_t i2=2)
   {
      const uint32_t t= tickCaptureInterval(i1,i2);
      uint32_t us= t>>4; // ticks to microseconds
      uint32_t f= (t & 0xF) * 625; // 16MHz -> 0.0625us
      s.print(us); s.print('.');
      if ((f > 0) && (f < 1000)) { s.print('0'); }
      s.print(f);
      return(t);
   }
}; // CTimerN5

class CRTClockN5 // RTC works in low power 'sleep' modes
{ // NB: 24b counter, 8Hz tick so rollover at 2^21 sec (24.2 days)
protected:
   uint32_t offsetSec; // rtc counter cannot be set, so offset required

   void setup (NRF_RTC_Type *pT=NRF_RTC0)
   {
      pT->TASKS_CLEAR= 1;  // clean reset
      pT->PRESCALER= RTC_PRESCALE_MAX; // 32768Hz / 2^12 -> 8Hz tick
      pT->INTENSET= (RTC_INTENSET_OVRFLW_Enabled << RTC_INTENSET_OVRFLW_Pos);
   } // setup

public:
   CRTClockN5 (void) { ; }
   
   uint32_t read (uint16_t *pMilliSec) const
   {
      uint32_t t= NRF_RTC0->COUNTER; // 8Hz tick, so 3b fraction, 1/8sec= 125ms
      if (pMilliSec) { *pMilliSec= (t & 0x7) * 125; }
      return((t>>3)+offsetSec);
   }
   void rtcOvfloEvent (void) { offsetSec+= 1<<21; }
}; // CRTClockN5

class CTimerInterval
{
public:
   uint32_t when, step;
   CTimerInterval (void) { ; }
   
   bool elapsed (uint32_t now)
   {
      if (now >= when) { when+= step; return(true); }
      return(false);
   }
}; // CTimerInterval


class CClock : public CTimerN5, CRTClockN5, CTimerInterval
{
public:

   CClock (void) { ; } // default zero initialisation of all fields

   void init (uint32_t ticks=10) { CTimerN5::setup(); CRTClockN5::setup(); CTimerInterval::step= ticks; }

   void setHMS (uint8_t hms[3])
   { 
      uint32_t s= hmsU8ToSecU32(hms);
      CTimerN5::tickCount= s * 1000;
      CRTClockN5::offsetSec= s;
   }

   void start (void)
   {
      NVIC_SetPriority(TIMER2_IRQn, 1);
      NVIC_EnableIRQ(TIMER2_IRQn);
      NRF_TIMER2->TASKS_START= 1;
#if 1
      NVIC_SetPriority(RTC0_IRQn,3);
      NVIC_EnableIRQ(RTC0_IRQn);
      NRF_RTC0->TASKS_START= 1;
#endif
      CTimerInterval::when= CTimerN5::tickCount + CTimerInterval::step;
   }
   using CTimerN5::tickEvent;
   using CRTClockN5::read;
   using CRTClockN5::rtcOvfloEvent;
   bool elapsed (void) { return CTimerInterval::elapsed(CTimerN5::tickCount); }

// not really class members...
   void printSecHMS (Stream& s, uint32_t sec) const
   {
      uint8_t hms[3];
      char ch[5];

      hmsU8FromSecU32(hms, sec); // -> 24:60:60

      ch[2]= ':';
      ch[3]= 0;
      for (int i=0; i<sizeof(hms); i++)
      {
         bcd4FromU8(hms+i, hms[i]); // in-place conversion
         hexChFromU8(ch, hms[i]);
         if (i == (sizeof(hms)-1)) { ch[2]= 0; }
         s.print(ch);
      }
   } // printSecHMS

   void printMilliSec (Stream& s, uint16_t ms) const
   {
      uint8_t bcd[2];
      char ch[5];
      bcd4FromU16(bcd,ms); // hacky reuse...
      ch[4]= 0;
      bcd4ToChar(ch, sizeof(ch)-1,bcd,2);
      ch[0]= '.';
      s.print(ch);
   } // printMilliSec
// ...not really class members

   void print (Stream& s, const char end=0x00) const
   {
      uint32_t sec;
      uint16_t ms= divmod32(sec, CTimerN5::tickCount, 1000);
      printSecHMS(s,sec);
      printMilliSec(s,ms);
      if (end > 0) { s.print(end); }
      // snprintf adds 16KB code! Hence BCD->char used instead.
      // snprintf(ch,sizeof(ch)-1,"%02u:%02u:%02u.%03u", hms[0], hms[1], hms[2], ms);
#if 0
      sec= CRTClockN5::read(&ms);
      s.print(" (");
      printSecHMS(s,sec);
      printMilliSec(s,ms);
      s.print(")");
#endif
   } // print

}; // CClock

#endif // N5_TIMING
