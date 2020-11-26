// Duino/Common/DA_FastPollTimer.h - Debug & test timer
// (factored from DA_Timing)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_FAST_POLL_TIMER_HPP
#define DA_FAST_POLL_TIMER_HPP

//#include "DA_Util.h"

class CFastPollTimer
{
public:
  uint16_t last;
  
  CFastPollTimer (void)
  {
#if (1 == AVR_FAST_TIMER)
      TCCR1A= 0;
      TCCR1B= (1 << CS10);  // no prescale @ 16MHz -> 0.0625us per ms (granularity 0.00625%)
      TCCR1C= 0;            //   rollover at 4ms
      // OCR1A / B, ICR1 unused
      TIMSK1= 0; // (1 << TOIE1);  // Overflow Interrupt Enable
      TCNT1=  0;   // set
#endif // AVR
  } // CFastPollTimer
  
  uint16_t stamp (void)
  {
#if (1 == AVR_FAST_TIMER)
      return TCNT1;
#endif // AVR
  } // stamp
  
  uint16_t diff (void)
  {
     uint16_t now= stamp();
     int16_t dt= now - last;
     last= now;
     if (dt < 0) { dt += 0xFFFF; }
     return(dt);
  } // diff
  
}; // CFastPollTimer

#endif // DA_FAST_POLL_TIMER_HPP
