// Duino/Common/DA_Timing.hpp - Arduino-AVR specific class definitions for timer / clock
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_TIMING_HPP
#define DA_TIMING_HPP

#include <avr/sleep.h>
#include "DA_Util.h"

//#define AVR_CLOCK_TRIM 64 // 64/128 +50% micro-tick (4us) per ms

// 25 / 128 = 0.195, *4us = 0.78125us = 0.078% (-ve faster, +ve slower)

// Timer0 dedicated to 'Duino infrastructure
// Timer1 (16bit) used for pulse counting or fast poll (debug)
#define AVR_CLOCK_TIMER 2

// NB: Timer0 used for delay() etc.
#if (0 == AVR_CLOCK_TIMER) || (2 == AVR_CLOCK_TIMER)
#define AVR_CLOCK_IVL 250
#else 
#define AVR_CLOCK_IVL 2000
#endif // AVR_CLOCK_TIMER


/***/

#if (1 == AVR_CLOCK_TIMER)
typedef uint16_t TimerIvl;
#else
typedef uint8_t TimerIvl;
#endif


// Highly AVR-specific base class using 8bit hardware timer
// Objective is to generate interrupts at 1ms intervals.
// NB : ISR vector/handler/function requires class instance and so
// must be defined elsewhere. C++ does not provide a clean solution.
class CBaseTimer
{
protected:
   TimerIvl ivl;
   volatile uint8_t nIvl;  // ISR mini-counter
   uint8_t nRet; // Previous mini-counter (since last "upstream" application)

   void hwUpdate (TimerIvl hwIvl)
   {
#if (2 == AVR_CLOCK_TIMER)
      OCR2A=  hwIvl - 1;  // It shouldn't be necessary to reload OCR
#endif
   } // hwUpdate
   
public:
   CBaseTimer () : ivl{AVR_CLOCK_IVL} { ; } // (all zero on reset) nIvl= 0; }
  
   void start (void) const
   {
#if (2 == AVR_CLOCK_TIMER)
      TCNT2=  0;
      OCR2A=  ivl - 1; // set compare interval
      TCCR2A= 1 << WGM21; // CTC 22:21:20=010
      TCCR2B= (1 << CS22);   // 1/64 prescaler @ 16MHz -> 250k ticks/sec, 4us per ms granularity (0.4%)
      TIMSK2= (1 << OCIE2A); // Compare A Interrupt Enable
#endif // (2 == AVR_CLOCK_TIMER)
   } // init
  
   void nextIvl (void)
   { // called from SIGNAL (blocking ISR) => interrupts already masked
      hwUpdate(ivl); // It shouldn't be necessary to reload OCR in normal operation... ???
      ++nIvl;
   } // nextIvl

   uint8_t diff (void)
   {
      int8_t d= nIvl - nRet;
      return( d>=0 ? d : -d ); // paranoid? : verify...
      //if (d >= 0) { return(d); } //else
      //return(-d);
   } // diff
  
   void retire (uint8_t d) { nRet+= d; }
}; // CBaseTimer

#ifdef AVR_CLOCK_TRIM
#define TT_SETF_NEG 0x1
#define TT_CYCF_TRM 0x1
#define TT_CYC_MASK 0xFE
// Provide super-resolution trimming by adjusting
// timer interval within a duty cycle
class CTrimTimer : public CBaseTimer
{
protected:
   uint8_t cycle, set; // upper 7bits provide counter & threshold respectively
   // lsb provides trim flag and sign flag respectively
   
   int8_t trimVal (void) const
   {
      int8_t t= cycle & TT_CYCF_TRM; // trim=0,1
      if (set & TT_SETF_NEG) return(-t); else return(t); // apply sign
   }
   // advance cycle and flip trim flag at cycle threshold
   void trimStep (void)
   {
      cycle+= 0x02;
      if ((cycle & TT_CYC_MASK) == (set & TT_CYC_MASK)) { cycle^= TT_CYCF_TRM; }
   }
   // initialise trim flag for current cycle 
   void trimSet (void)
   {
      if ((cycle & TT_CYC_MASK) < (set & TT_CYC_MASK)) { cycle|= TT_CYCF_TRM; }
      else { cycle&= ~TT_CYCF_TRM; }
   }

public:
   CTrimTimer (int8_t trim=0) { if (0 != trim) setTrim(trim); }

   void setTrim (int8_t trim)
   {
      if (0 == trim) { cycle= set= 0; }
      else
      {
         if (trim < 0) { set= TT_SETF_NEG | (-trim << 1); }
         else { set= TT_CYC_MASK & (trim << 1); }
         trimSet();
      }
   }
   void nextIvl (void) // overload
   {
      if (0 == set) { CBaseTimer::nextIvl(); }
      else
      {
         hwUpdate(ivl+trimVal());
         trimStep();
         ++CBaseTimer::nIvl;
      }
   }
}; // CTrimTimer
#endif // AVR_CLOCK_TRIM

#if 0
// Extend base timer with simple synchronisation delay
// This was intended for external hardware reset
// synchronisation but was supersceded.
class CDelayTimer : public CTrimTimer
{
protected:
   uint8_t nDel;
  
public:
   CDelTimer (int8_t trim=0) : CTrimTimer(trim) { ; } // (all zero on reset) nDel= 0; }
    //sleep_mode();
  
   void setDelay (uint8_t n) { nDel= nIvl + n; }
  
   int8_t wait (uint8_t n=0)
   {
      int8_t r=-1;
      nDel+= n;
      do
      {  // spin
         r= nDel - nIvl;
         if (r <= -64) { r= -r; }
         //if (r > 0) { sleep_cpu(); }
      } while (r > 0);
      return(r);
   } // wait
}; // CDelayTimer
#endif

class CIntervalTimer
{
protected:
   uint16_t interval, next;
   
   void setNext (uint16_t when, uint16_t rollover)
   {
      if (when < rollover) { next= when; } else { next= when - rollover; }
   }
   
public:
   CIntervalTimer (uint16_t ivl=1000, uint16_t start=0) { interval= ivl; intervalStart(start); }
   void intervalStart (uint16_t when, uint16_t rollover=60000) { setNext(when+interval, rollover); }
   
   int16_t intervalDiff (uint16_t now) const { return(now - next); }
   bool intervalComplete (uint16_t now)
   {
      int16_t d= intervalDiff(now);
      //Serial.print("IVLC"); Serial.print(now); Serial.print("-"); Serial.print(next);  Serial.print("="); Serial.println(d);
      return((d >= 0) && (d < interval));
   }
   bool intervalUpdate (uint16_t now, uint16_t rollover=60000)
   {
      if (intervalComplete(now))
      {
         setNext(next+interval, rollover);
         return(true);
      }
      return(false);
   }
}; // CIntervalTimer

uint16_t tickDiffWrapU16 (uint16_t ta, uint16_t tb)
{
   if (ta > tb) { return(ta-tb); } else { return(ta+60000-tb); }
} // tickDiffWrapU16

// Simple clock running off timer class, handy for debug logging 
// A high precision crystal resonator <=20ppm, temperature compensation,
// battery backup and some calendar awareness could facilitate a tolerable
// chronometer...
class CClock : public CIntervalTimer,
#ifdef AVR_CLOCK_TRIM
    public CTrimTimer
#else
    public CBaseTimer
#endif
{
public:
   uint16_t tick, tock; // milliseconds (rollover 60 sec) and minutes (rollover ~45.5 days)
   CClock (uint16_t ivl=3000, int8_t trim=0) : CIntervalTimer(ivl)
#ifdef AVR_CLOCK_TRIM
      , CTrimTimer(trim)
#endif
      { ; } // CClock

   uint8_t update (void)
   {
      uint8_t d= diff();
      if (d > 0)
      {
         tick+= d;
         if (tick >= 60000)
         {
            tick-= 60000;
            tock++;
         }
         retire(d);
      }
      return(d);
   } // update

   // Simple case for single interval
   // For multi-interval should set flags
   int16_t intervalDiff (void) { return CIntervalTimer::intervalDiff(tick); }
   void intervalStart (void) { return CIntervalTimer::intervalStart(tick); }
   bool intervalUpdate (void) { return CIntervalTimer::intervalUpdate(tick); }
   
#ifdef AVR_CLOCK_TRIM
   using CTrimTimer::nextIvl; // void nextIvl (void) { CTrimTimer::nextIvl(); }
#endif
   
   void setHM (const uint8_t hm[2]) { tock= hm[0]*60 + hm[1]; }
   void setS (const uint8_t s) { tick= s*1000; }
   void getHM (uint8_t hm[2]) const { convTimeHM(hm, tock); }
   // Serial interface support, debug mostly?
   int8_t getStrHM (char str[], int8_t max, char end=0)
   {
      if (max > 5)
      {
         uint8_t hm[2];
         int8_t n;
         getHM(hm);
         n= hex2ChU8(str+0, conv2BCD4(hm[0]));
         str[n++]= ':';
         n+= hex2ChU8(str+n, conv2BCD4(hm[1]));
         str[n]= end;
         n+= (0!=end);
         return(n);
      }
      return(0);
   } // getStrHM

}; // CClock

#endif // DA_TIMING_HPP
