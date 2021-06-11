
// Duino/Common/N5/N5_ClockInstance.hpp - nRF5* (Micro:Bit) instantiation of clock class and ISR setup
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors June 2021

#ifndef N5_CLOCK_INSTANCE_HPP
#define N5_CLOCK_INSTANCE_HPP

#include "N5_Timing.hpp"


/*** ISR ***/

CClock gClock;

// NB - 'duino "magic" name linking requires undecorated symbol
extern "C" {

void TIMER2_IRQHandler (void)
{
   NRF_TIMER_Type *pT=NRF_TIMER2;
	if (0 != pT->EVENTS_COMPARE[3])
   {
		pT->EVENTS_COMPARE[3]= 0; // Clear event (SHORTS resets count)
      while (0 != pT->EVENTS_COMPARE[3]); // spin sync
      gClock.tickEvent();
   }
} // TIMER2_IRQHandler

void RTC0_IRQHandler (void)
{
	if (0 != NRF_RTC0->EVENTS_OVRFLW)
   {
      NRF_RTC0->EVENTS_OVRFLW= 0; // clear & spin sync
      while (0 != NRF_RTC0->EVENTS_OVRFLW);
      gClock.rtcOvfloEvent(); // update offset
   }
} // RTC0_IRQHandler

} // extern "C"

#endif // N5_CLOCK_INSTANCE_HPP
