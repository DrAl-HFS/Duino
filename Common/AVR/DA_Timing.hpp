// Duino/Common/AVR/DA_Timing.hpp - Arduino-AVR specific class definitions for timer / clock
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Nov 2020 - Apr 2021

#ifndef DA_TIMING_HPP
#define DA_TIMING_HPP

#include <avr/sleep.h>
#include "DA_Util.h"
#include "../dateTimeUtil.hpp"

//#define AVR_CLOCK_TRIM 64 // 64/128 +50% micro-tick (4us) per ms

// 25 / 128 = 0.195, *4us = 0.78125us = 0.078% (-ve faster, +ve slower)

// Timer0 dedicated to 'Duino infrastructure - delay() etc.
// Timer1 (16bit) used for pulse counting or fast poll (debug)
#define AVR_CLOCK_TIMER 2

// Allow for hacky upscaling of interrupt frequency
#ifndef AVR_HW_TICKS_MS
#define AVR_HW_TICKS_MS 1 // standard scaling
#endif

#ifdef AVR_INTR_SLOW
#define AVR_INTR_COUNT 156 // 16MHz / 1024 -> 9.984ms
#define AVR_HW_MS_TICK 10 // (approx)
#else
#define AVR_INTR_COUNT 250 // 16MHz / 64 -> 1ms
#define AVR_HW_MS_TICK 1
#endif

#if (2 == AVR_CLOCK_TIMER)
#define AVR_CLOCK_IVL (AVR_INTR_COUNT / AVR_HW_TICKS_MS)
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
#ifdef AVR_INTR_SLOW
      TCCR2B= (1 << CS22)|(1 << CS21)|(1 << CS20); // 1/1024 prescaler @ 16MHz -> 15625 ticks/sec
#else
      TCCR2B= (1 << CS22);   // 1/64 prescaler @ 16MHz -> 250k ticks/sec, 4us per ms granularity (0.4%)
#endif
      TIMSK2= (1 << OCIE2A); // Compare A Interrupt Enable
#endif // (2 == AVR_CLOCK_TIMER)
   } // init

   void nextIvl (void)
   { // called from SIGNAL (blocking ISR) => interrupts already masked
      hwUpdate(ivl); // It shouldn't be necessary to reload OCR in normal operation... ???
      ++nIvl;
   } // nextIvl

   uint8_t diff (void) const
   {
      int8_t d= nIvl - nRet;
      return( d >= 0 ? d : (nIvl + 0xFF-nRet) ); // paranoid? : verify...
   } // diff

   void retire (uint8_t d) { if (-1 == d) { nRet= nIvl; } else { nRet+= d; } }
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

/*** COMMON ***/

class CDownTimer
{
protected:
   int16_t remain;

public:
   CDownTimer (void) {;}

   void set (int16_t r) { remain= r; }
   void add (int16_t r) { remain+= r; }

   bool isSet (void) { return(remain>0); }
   bool update (uint8_t dt=1) { remain-= dt; return(remain<=0); }
}; // CDownTimer

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

   void tickTock (void) { if (tick >= 60000) { tick-= 60000; tock++; } }
   uint8_t update (void)
   {
      uint8_t d= diff();
      if (d >= AVR_HW_TICKS_MS)
      {
         uint8_t m= d / AVR_HW_TICKS_MS;
         tick+= m * AVR_HW_MS_TICK;
         tickTock();
         retire(m * AVR_HW_TICKS_MS);
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

   void getHM (uint8_t hm[2]) const { convTimeHM(hm, tock); }
   void setHM (const uint8_t hm[2]) { tock= hm[0]*60 + hm[1]; }
   // HACK! <<<
   void setS (const uint8_t s) { tick= s*1000; tickTock(); }
   void addM (int8_t dm) { tock+= dm; }
   void addS (int8_t ds) { tick+= ds*1000; tickTock(); } // overflow/wrap hazard

}; // CClock

// Extend with basic ASCII IO functions
class CClockA : public CClock
{
public:
   CClockA (uint16_t ivl=3000, int8_t trim=0) : CClock(ivl,trim) { ; }

   void setA (const char a[], uint16_t msAdd=0)
   {  // No parsing! assumes exact "hh:mm:ss"
      uint8_t tt[3];
#if 1
      u8FromTimeA(tt, a);
      setHM(tt);
      setS(tt[2]);
#else
      tt[0]= fromBCD4(char2BCD4(a+0,2),2);
      tt[1]= fromBCD4(char2BCD4(a+3,2),2);
      setHM(tt);
      tt[0]= fromBCD4(char2BCD4(a+5,2),2);
      setS(tt[0]);
#endif
      tick+= msAdd;
   }

   // Serial interface support, debug mostly?
   int8_t getStrHM (char str[], int8_t max, char end=0) const
   {
      if (max > 5)
      {
         uint8_t hm[2];
         int8_t n;
         getHM(hm);
         n= hex2ChU8(str+0, conv2BCD4(hm[0]));
         str[n++]= ':';
         n+= hex2ChU8(str+n, conv2BCD4(hm[1]));
         if (n < max) { str[n]= end; n+= (0 != end); }
         return(n);
      }
      return(0);
   } // getStrHM

   int8_t getStrS (char str[], int8_t max, int8_t bcdB=1, char end=0) const
   {
      int8_t n= 0;
      if ((max >= 2) && (bcdB > 0))
      {
         uint8_t tickBCD[3];
         convMilliBCD(tickBCD, bcdB, tick);
         n= hex2ChU8(str+0, tickBCD[0]);
         if ((bcdB > 1) && (max >= 5))
         {
            str[n++]= '.';
            n+= hex2ChU8(str+n, tickBCD[1]); // centi-seconds
         }
         if (n < max) { str[n]= end; n+= (0 != end); }
      }
      return(n);
   } // getStrS

   int8_t getStrHMS (char str[], int8_t max, char end=0) const
   {
      int8_t n= getStrHM(str, max, ':');
      if (n > 0) { n+= getStrS(str+n, max-n, 1, end); }
      return(n);
   } // getStrHMS

   int8_t printHMS (Stream& s, const char end=0x0)
   {
      char hms[10];
      int8_t i= getStrHMS(hms,sizeof(hms)-1);
      //if (i < sizeof(hms))
      hms[i++]= end;
      if (0x00 != end) { hms[i++]= 0x00; }
      s.print(hms);
      return(i);
   }

}; // CClockA

#endif // DA_TIMING_HPP
