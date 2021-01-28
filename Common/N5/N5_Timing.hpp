// Duino/Common/N5/N5_Timing.hpp - nRF5* (Micro:Bit V1 nRF51822) timing utilities
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

   void setup (NRF_TIMER_Type *pT=NRF_TIMER2)
   {
      pT->MODE = TIMER_MODE_MODE_Timer;
      pT->PRESCALER= 4;   // 16MHz / 2^4 -> micro-second tick
      pT->TASKS_CLEAR= 1; // clean reset
      pT->BITMODE= TIMER_BITMODE_BITMODE_16Bit; // 24, 32 bit comparison unnecessary
      pT->CC[0]= 1000;   // 1ms

      // Enable auto clear of count & interrupt for CC[0]
      pT->SHORTS=   TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
      pT->INTENSET= (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
      // | (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
   } // setup
}; // CTimerN5

class CRTClockN5
{
public:
   CRTClockN5 (void) { ; }

   void setup (NRF_RTC_Type *pT=NRF_RTC0)
   {
      pT->TASKS_CLEAR= 1; // clean reset
      //pT->COUNTER=   0; // CLEAR ???
      pT->PRESCALER= 2048;   // 32768Hz / 2^11 -> 16Hz
   } // setup
}; // CRTClockN5

class CClock : protected CTimerN5, CRTClockN5
{
public:
   uint32_t milliTick; // 49.7 days wrap around

   CClock (void) { ; } // default zero initialisation of all fields

   void init (void) { CTimerN5::setup(); CRTClockN5::setup(); }

   void start (void)
   {
      NVIC_EnableIRQ(TIMER2_IRQn);
      NRF_TIMER2->TASKS_START= 1;

      //NVIC_EnableIRQ(RTC0_IRQn);
      NRF_RTC0->TASKS_START= 1;
   }

   void nextIvl (void) { milliTick++; }

   void printSec (Stream& s, uint32_t sec) const
   {
      uint8_t hms[3];
      char ch[5];

      sec2HMS(hms, sec); // -> 24:60:60
      // snprintf adds 16KB code! Use BCD->char instead...
      // snprintf(ch,sizeof(ch)-1,"%02u:%02u:%02u.%03u", hms[0], hms[1], hms[2], ms);

      ch[2]= ':';
      ch[3]= 0;
      for (int i=0; i<sizeof(hms); i++)
      {
         u8ToBCD4(hms+i, hms[i]); // in-place conversion
         hex2ChU8(ch, hms[i]);
         if (i == (sizeof(hms)-1)) { ch[2]= 0; }
         s.print(ch);
      }
   } // printSec

   void printMilliSec (Stream& s, uint16_t ms) const
   {
      uint8_t bcd[2];
      char ch[5];
      u16ToBCD4(bcd,ms); // hacky reuse...
      ch[4]= 0;
      bcd4ToChar(ch, sizeof(ch)-1,bcd,2);
      ch[0]= '.';
      s.print(ch);
   } // printMilliSec

   void print (Stream& s) const
   {
      uint32_t sec;
      uint16_t ms= divmod(sec, milliTick, 1000);
      printSec(s,sec);
      printMilliSec(s,ms);

      s.print(" (");
      sec= NRF_RTC0->COUNTER; // 16Hz
      ms= ((sec & 0xF) * 625) / 10;
      sec>>= 4;
      printSec(s,sec);
      printMilliSec(s,ms);
      s.print(")");
   } // print

}; // CClock

#endif // N5_TIMING
