// Duino/UBit/N5Common/N5_Timing.hpp - nRF5* (Micro:Bit V1 nRF51822) timing utilities
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef N5_TIMING
#define N5_TIMING

#include <nrf51.h>
#include <nrf51_bitfields.h>

class CTimerN5
{
public:
   CTimerN5 (void) {;}
   
//protected:
   void setup (NRF_TIMER_Type *pT=NRF_TIMER2)
   {		
      pT->MODE = TIMER_MODE_MODE_Timer;
      pT->PRESCALER= 4;   // 16 clocks -> micro-second tick
      pT->TASKS_CLEAR= 1; // clean reset
      pT->BITMODE= TIMER_BITMODE_BITMODE_16Bit; // 24, 32 unnecessary
      pT->CC[0]= 1000;   // 1ms
         
      // Enable auto clear of count & interrupt for CC[0]
      pT->SHORTS=   TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
      pT->INTENSET= (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
      // | (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
   } // timerSetup
}; // CTimerN5

class CClock : protected CTimerN5
{
public:
   uint32_t tick;

   CClock (void) { ; } // tick= 0; // default initialisation with zero
   
   void init (void) { CTimerN5::setup(); }
   
   void start (void)
   {
      NVIC_EnableIRQ(TIMER2_IRQn);
      NRF_TIMER2->TASKS_START= 1;
   }
   
   void nextIvl (void) { tick++; }
   
   void print (Stream& s) const
   {
      char b[16];
      uint32_t r= tick / 1000;
      uint16_t ms= tick % 1000;
      uint8_t hms[3];
      
      hms[2]= r % 60;
      r/= 60;
      hms[1]= r % 60;
      r/= 60;
      hms[0]= r % 24;
      // r/= 24;
      snprintf(b,sizeof(b)-1,"%02u:%02u:%02u.%03u", hms[0], hms[1], hms[2], ms); // +16KB !! Flash Img
      s.print(b);
   }
}; // CClock

#endif // N5_TIMING
