// Duino/Common/N5/N5_Timing.hpp - nRF5* (Micro:Bit V1 nRF51822) timing utilities
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef N5_TIMING
#define N5_TIMING

#include <nrf51.h>
#include <nrf51_bitfields.h>

class CTimerN5
{
protected:
   uint32_t tickCount; // 49.7 days wrap around

   void setup (NRF_TIMER_Type *pT=NRF_TIMER2)
   {
      pT->MODE = TIMER_MODE_MODE_Timer;
      pT->PRESCALER= 4;   // 16MHz / 2^4 -> micro-second tick
      pT->TASKS_CLEAR= 1; // clean reset
      pT->BITMODE= TIMER_BITMODE_BITMODE_32Bit; // 16, 24, 32 bit
      pT->CC[0]= 1000;   // 1ms

      // CC[0] auto clear = priodic 0..9999 
      //pT->SHORTS=   TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
      // interrupt on CC[0] match
      pT->INTENSET= (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
      // | (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
   } // setup
public:
   CTimerN5 (void) {;}

   void tickEvent (NRF_TIMER_Type *pT=NRF_TIMER2)
   {
      pT->CC[0]+= 1000; // rolling mode allows longer term timing measurements using capture registers
      tickCount++; 
   }
   int8_t capTick (int8_t i, NRF_TIMER_Type *pT=NRF_TIMER2) // { return(pT->COUNTER); } ???
   {  // NB CC[0] used to trigger interrupt - capturing to this is a VERY bad idea
      if ((i < 1) || (i > 3)) { return(-1); }
      pT->TASKS_CAPTURE[i]= 1; // sync ??
      return(i);
   }
   int32_t capTickInterval (int8_t i0, int8_t i1, NRF_TIMER_Type *pT=NRF_TIMER2)
   {
      int32_t r= 0;
      if ((i0 <= 3) && (i1 <= 3))
      { 
         r= pT->CC[i0] - pT->CC[i1];
      }
      return(r);
   } // capTickInterval
}; // CTimerN5

class CRTClockN5 // RTC works in low power 'sleep' modes
{ // NB: 24b counter, 8Hz tick so rollover at 2^21 sec (24.2 days)
protected:
   uint32_t offsetSec; // rtc counter cannot be set, so offset required

   void setup (NRF_RTC_Type *pT=NRF_RTC0)
   {
      pT->TASKS_CLEAR= 1;  // clean reset
      pT->PRESCALER= 4095; // (+1) -> 32768Hz / 2^12 -> 8Hz tick
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
   void ofloEvent (void) { offsetSec+= 1<<21; }
}; // CRTClockN5

#if 0
class CInterval
{
public:
   CInterval (void) { ; }
}; // CInterval

class CIntervalTimer
{
protected:
   uint32_t millIvl, milliNext;

   void setNext (uint32_t milliWhen) { milliNext= milliWhen; }
   //if (when < rollover) { next= when; } else { next= when - rollover; }

public:
   CIntervalTimer (uint32_t mIvl=1000, uint32_t start=0) { millIvl= mIvl; intervalStart(start); }
   void intervalStart (uint32_t when) { setNext(when+interval); }

   int32_t intervalDiff (uint32_t milliNow) const { return(milliNow - milliNext); }
   bool intervalComplete (uint32_t milliNow)
   {
      int16_t d= intervalDiff(now);
      //Serial.print("IVLC"); Serial.print(now); Serial.print("-"); Serial.print(next);  Serial.print("="); Serial.println(d);
      return((d >= 0) && (d < interval));
   }
   bool intervalUpdate (uint32_t milliNow)
   {
      if (intervalComplete(milliNow))
      {
         setNext(next+interval);
         return(true);
      }
      return(false);
   }
}; // CIntervalTimer
public CIntervalTimer, 
#endif

class CClock : public CTimerN5, CRTClockN5
{
public:

   CClock (void) { ; } // default zero initialisation of all fields

   void init (void) { CTimerN5::setup(); CRTClockN5::setup(); }

   void setHMS (uint8_t hms[3])
   { 
      uint32_t s= hmsU8ToSecU32(hms);
      CTimerN5::tickCount= s * 1000;
      CRTClockN5::offsetSec= s;
   }

   void start (void)
   {
      //NVIC_SetPriority(TIMER2_IRQn, 3);
      NVIC_EnableIRQ(TIMER2_IRQn);
      NRF_TIMER2->TASKS_START= 1;

      NVIC_SetPriority(RTC0_IRQn,5);
      NVIC_EnableIRQ(RTC0_IRQn);
      NRF_RTC0->TASKS_START= 1;
   }
   using CTimerN5::tickEvent;
   using CRTClockN5::read;
   using CRTClockN5::ofloEvent;

   bool interval (uint32_t t) { return(t <= CTimerN5::tickCount); }
   uint32_t offset (uint32_t o) { return(CTimerN5::tickCount + o); }
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
         hexCharU8(ch, hms[i]);
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

   void print (Stream& s) const
   {
      uint32_t sec;
      uint16_t ms= divmod(sec, CTimerN5::tickCount, 1000);
      printSecHMS(s,sec);
      printMilliSec(s,ms);

      // snprintf adds 16KB code! Hence BCD->char used instead.
      // snprintf(ch,sizeof(ch)-1,"%02u:%02u:%02u.%03u", hms[0], hms[1], hms[2], ms);

      sec= CRTClockN5::read(&ms);
      s.print(" (");
      printSecHMS(s,sec);
      printMilliSec(s,ms);
      s.print(")");
   } // print

}; // CClock

#endif // N5_TIMING
