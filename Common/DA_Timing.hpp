// Duino/Common/DA_timing.h - Arduino-AVR specific class definitions for timer / clock
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_TIMING_H
#define DA_TIMING_H

#include <avr/sleep.h>
#include "DA_NumCh.hpp"

#ifdef AVR
#define AVR_CLOCK_OFLO  // timer overflow interrupt (versus compare)
#define AVR_CLOCK_TN 2
// NB: Timer0 used for delay() etc.
#if (0 == AVR_CLOCK_TN) || (2 == AVR_CLOCK_TN)
#define AVR_CLOCK_IVL 250
#define AVR_CLOCK_TRIM  0 // -1 0.4% faster
#else 
#define AVR_CLOCK_IVL 2000
#define AVR_CLOCK_TRIM -3 // 0.15% faster
#endif // AVR_CLOCK_TN
#endif


/***/

// TODO : assess compiler generated (fast?) division by constant

uint_fast16_t convTimeD (uint_fast8_t d[1], uint_fast16_t t)
{
  d[0]= t / (24 * 60);
  return(t - (d[0] * 24 * 60));
} // convTimeD

void convTimeHM (uint_fast8_t hm[2], uint_fast16_t t)
{
   hm[0]= t / 60;
   hm[1]= t - (hm[0] * 60);
} // convTimeHM

// generate specified number of BCD bytes (yielding 2, 4 or 5 digits)
void convMilliBCD (uint_fast8_t bcd[], const int8_t n, uint_fast16_t m)
{
   if (n > 0)
   {  // TODO : investigate feasibility of divmod()
      uint8_t c, s= m / 1000;
      bcd[0]= conv2BCD4(s); // set seconds digits
      if (n > 1)
      {
         m-= s * 1000;  // get remainder
         c= m / 10;
         bcd[1]= conv2BCD4(c);  // centi-seconds
         if (n > 2) { bcd[2]= swapHiLo4U8(m - c * 10); }
      }
   }
} // convMilliBCD


/***/

// Highly AVR-specific base class using 8 or 16bit hardware timers
// Objective is to generate interrupts at 1ms intervals.
// NB : ISR vector/handler/function requires class instance and so
// must be defined elsewhere. C++ does not provide a clean solution.
class CBaseTimer
{
protected:
#if (1 == AVR_CLOCK_TN)
  uint16_t ivl;
#else
  uint8_t ivl;
#endif
  volatile uint8_t nIvl;  // ISR mini-counter
  uint8_t nRet; // Previous mini-counter (since last "upstream" application)
  
public:
  CBaseTimer (int8_t trim) : ivl{AVR_CLOCK_IVL+trim} { ; } // (all zero on reset) nIvl= 0; }
  
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
  
  uint16_t nextIvl (void)
  { // called from SIGNAL (blocking ISR) => interrupts already masked
    //noInterrupts();
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
    ++nIvl;
    //interrupts();
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

#if 0
// Extend base timer with simple synchronisation delay
// This was intended for external hardware reset
// synchronisation but has effectively been supersceded
// by state machines in main timing loop.
class CDelTimer : public CBaseTimer
{
protected:
  uint8_t nDel;
  
public:
  CDelTimer (int8_t trim=0) : CBaseTimer(trim) { ; } // (all zero on reset) nDel= 0; }
    //sleep_mode();
  
  void setDelay (uint8_t n) { nDel= nIvl + n; }
  
  int8_t wait (uint8_t n=0)
  {
    int8_t r=-1;
    nDel+= n;
    do
    {
      r= nDel - nIvl;
      if (r <= -64) { r= -r; }
      //if (r > 0) { sleep_cpu(); }
    } while (r > 0);
    return(r);
  } // wait
}; // CDelTimer
#endif

// Simple clock running off timer class, handy for debug logging 
// A high precision crystal resonator <=20ppm, temperature compensation,
// battery backup and some calendar awareness could facilitate a tolerable
// chronometer...
class CClock : public CBaseTimer
{
public:
   uint16_t tick, tock; // milliseconds (rollover 60 sec) and minutes (rollover ~45.5 days)
   CClock (int8_t trim=0) : CBaseTimer(trim) { ; } // if (pS) { setHM(pS); } }
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
