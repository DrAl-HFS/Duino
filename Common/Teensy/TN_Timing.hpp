// Duino/Common/Teensy/TN_Timing.hpp - Teensy timing utils
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Apr 2021

#ifndef TN_TIMING_HPP
#define TN_TIMING_HPP

// Possibilities...
//#include "TeensyTimerTool.h" ???
//#include "TimerOne.h"

typedef void (*VFPtr)(void);

#define IVL_COUNT_MAX 1
class CMultiIntervalTimer : public IntervalTimer
{
protected:
   volatile uint32_t tick;
   uint32_t last;
   uint32_t next[IVL_COUNT_MAX];
   uint16_t ivl[IVL_COUNT_MAX];
   
public:
	 CMultiIntervalTimer (void) { ; }
   
   // CAVEAT: wrap at 49.5 days (2^32 ms) not handled!
   void init (VFPtr f, uint32_t swIvl, uint32_t hwIvl=1000)
   { 
      IntervalTimer::begin(f,hwIvl);
      last= tick= 0;
      next[0]= ivl[0]= swIvl;
   }
   void tickEvent (void) { ++tick; }
   
   uint32_t diff (void) { return(tick - last); }
   void retire (void) { last= tick; }
   
   uint8_t update (void)
   {
      uint8_t m= diff() > 0;
      for (int i=0; i<IVL_COUNT_MAX; i++)
      {
         if ((ivl[i] > 0) && (tick >= next[i]))
         {
            m|= 0x2 << i;
            next[i]+= ivl[i];
         }
      }
      return(m);
   }
}; // CMultiIntervalTimer

#endif // TN_TIMING_HPP
