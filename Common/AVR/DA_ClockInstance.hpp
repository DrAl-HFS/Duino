// Duino/Common/AVR/DA_ClockInstance.hpp - Wrapper for (Arduino-AVR specific) timer / clock declaration
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Feb 2022

#ifndef DA_CLOCK_INSTANCE_HPP
#define DA_CLOCK_INSTANCE_HPP

//#define AVR_CLOCK_TRIM 64

#include "DA_Timing.hpp"

#ifndef CLOCK_INTERVAL
#define CLOCK_INTERVAL 1000
#endif // 

CClock gClock(CLOCK_INTERVAL);

#if (2 == AVR_CLOCK_TIMER)
// Connect clock to timer interrupt
// SIGNAL blocks other ISRs - check use of sleep()
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCA_vect) { gClock.nextIvl(); }
//SIGNAL(TIMER2_OCB_vect) { gClock.nextIvl(); }
#endif // AVR_CLOCK_TIMER

#endif // DA_CLOCK_INSTANCE_HPP
