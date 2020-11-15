// Duino/Common/DA_timing.h - Arduino-AVR specific class definitions for timer / clock
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_TIMING_H
#define DA_TIMING_H

#include <avr/sleep.h>
#include "DA_Util.h"

#ifdef AVR
// 0.08
#define AVR_CLOCK_TRIM  -25 // 25 / 128 = 0.195, *4us = 0.78125us = 0.078% faster

#define AVR_CLOCK_OFLO  // timer overflow interrupt (versus compare)
#define AVR_CLOCK_TN 2
// NB: Timer0 used for delay() etc.
#if (0 == AVR_CLOCK_TN) || (2 == AVR_CLOCK_TN)
#define AVR_CLOCK_IVL 250
#else 
#define AVR_CLOCK_IVL 2000
//#define AVR_CLOCK_TRIM -3 // 0.15% faster
#endif // AVR_CLOCK_TN
#endif


/***/




/***/

#if (1 == AVR_CLOCK_TN)
typedef uint16_t TimerIvl;
#else
typedef uint8_t TimerIvl;
#endif

// temporary hack for timer overflow interrupt
// (until compare & reload is working)
void ofloIvl (TimerIvl ivl)
{
#ifdef AVR_CLOCK_OFLO // TODO: address clock cycle gain during RMW sequence
#if (2 == AVR_CLOCK_TN)
   TCNT2-= ivl;
#endif // (0 == AVR_CLOCK_TN)
#if (0 == AVR_CLOCK_TN)
   TCNT0-= ivl;
#endif // (0 == AVR_CLOCK_TN)
#if (1 == AVR_CLOCK_TN)
   TCNT1-= ivl;
#endif // (1 == AVR_CLOCK_TN)
#endif // AVR_CLOCK_OFLO
} // ofloIvl

// Highly AVR-specific base class using 8 or 16bit hardware timers
// Objective is to generate interrupts at 1ms intervals.
// NB : ISR vector/handler/function requires class instance and so
// must be defined elsewhere. C++ does not provide a clean solution.
class CBaseTimer
{
protected:
   TimerIvl ivl;
   volatile uint8_t nIvl;  // ISR mini-counter
   uint8_t nRet; // Previous mini-counter (since last "upstream" application)
  
   
public:
   CBaseTimer () : ivl{AVR_CLOCK_IVL} { ; } // (all zero on reset) nIvl= 0; }
  
   void start (void) const
   { // TODO - test compare (auto reload) accuracy
#ifdef AVR_CLOCK_TN
#if (2 == AVR_CLOCK_TN)
#ifdef AVR_CLOCK_OFLO
      TCNT2=  (0xFF - ivl) + 1;   // preload timer
      TCCR2A=  0;
      TCCR2B= (1 << CS22);  // 1/64 prescaler @ 16MHz -> 250k ticks/sec, 4us per ms granularity (0.4%)
      TIMSK2= (1 << TOIE2); // Overflow Interrupt Enable
#else // Compare
      TCNT2=  0;
      OCR2A=  ivl - 1; // set interval
      TCCR2A= 1 << WGM21; // CTC
      TCCR2B= (1 << CS22);   // 1/64 prescaler @ 16MHz -> 250k ticks/sec, 4us per ms granularity (0.4%)
      TIMSK2= (1 << OCIE2A); // Compare A Interrupt Enable
#endif
#endif // (2 == AVR_CLOCK_TN)
#if (0 == AVR_CLOCK_TN)
      TCNT0=  (0xFF - ivl) + 1;   // preload timer
      TCCR0A=  0;
      TCCR0B= (1 << CS01)|(1 << CS00);  // 1/64 prescaler as above
      TIMSK0= (1 << TOIE0);  // Overflow Interrupt Enable
#endif // (0 == AVR_CLOCK_TN)
#if (1 == AVR_CLOCK_TN)
      TCNT1=  (0xFFFF - ivl) + 1;   // preload timer
      TCCR1A= 0;
      TCCR1B= (1 << CS11);  // 1/8 prescaler @ 16MHz -> 2M ticks/sec, 0.5us per ms granularity (0.05%)
      TIMSK1= (1 << TOIE1);  // Overflow Interrupt Enable
#endif // (1 == AVR_CLOCK_TN)
#endif // AVR_CLOCK_TN
   } // init
  
   void nextIvl (void)
   { // called from SIGNAL (blocking ISR) => interrupts already masked
      ofloIvl(ivl);
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
      if (0 == set) { ofloIvl(ivl); }
      else
      {
         ofloIvl(ivl+trimVal());
         trimStep();
      }
      ++nIvl;
   }
}; // CTrimTimer

#if 0
// Extend base timer with simple synchronisation delay
// This was intended for external hardware reset
// synchronisation but has effectively been supersceded
// by state machines in main timing loop.
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


// Simple clock running off timer class, handy for debug logging 
// A high precision crystal resonator <=20ppm, temperature compensation,
// battery backup and some calendar awareness could facilitate a tolerable
// chronometer...
class CClock : public CTrimTimer
{
public:
   uint16_t tick, tock; // milliseconds (rollover 60 sec) and minutes (rollover ~45.5 days)
   CClock (int8_t trim=0) : CTrimTimer(trim) { ; } // if (pS) { setHM(pS); } }
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

   using CTrimTimer::nextIvl; // void nextIvl (void) { CTrimTimer::nextIvl(); }
   
   void setHM (const uint8_t hm[2]) { tock= hm[0]*60 + hm[1]; }
   void getHM (uint8_t hm[2]) const { convTimeHM(hm, tock); }
   // Serial interface support, debug mostly?
   int8_t getStrHM (char str[], int8_t max, char end=0)
   {
      if (max > 5)
      {
         uint8_t hm[2];
         getHM(hm);
         hex2ChU8(str+0, conv2BCD4(hm[0]));
         str[2]= ':';
         hex2ChU8(str+3, conv2BCD4(hm[1]));
         str[5]= end;
         return(5+(0!=end));
      }
      return(0);
   } // getStrHM

}; // CClock

#endif // DA_TIMING_H
