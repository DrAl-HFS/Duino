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

class CIntervalTimer
{
protected:
   IntervalTimer it;
   volatile uint32_t   tick;
   uint32_t next, ivl;
   
public:
	 CIntervalTimer (void) { ; }
   
   // CAVEAT: wrap at 49.5 days (2^32 ms) not handled!
   void init (VFPtr f, uint32_t swIvl, uint32_t hwIvl=1000) { it.begin(f,hwIvl); ivl= swIvl; }
   void tickEvent (void) { ++tick; }
   
   bool update (void)
   {
      bool r= (tick >= next);
      if (r) { next+= ivl; }
      return(r);
   }
}; // CIntervalTimer

#endif // TN_TIMING_HPP
