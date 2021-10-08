// Duino/Common/STM32/ST_Timing.hpp - wrapper for RCM/STM32Duino (libMaple) timer utils
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2021

#ifndef ST_TIMING_HPP
#define ST_TIMING_HPP

// Extend LibMaple utils, http://docs.leaflabs.com/docs.leaflabs.com/index.html
// This timer provides limited range but high precision at the hardware level
// (16bits @ core clock rate), plus adequate range & precision (>60sec @ 1ms)
// for human-oriented features at the software level
class CTimer : protected HardwareTimer
{
protected:
   volatile uint16_t nIvl; // event count
   uint16_t nRet;           // & tracking
   
   uint16_t diffWrap16 (uint16_t a, uint16_t b) const
   {
      int16_t d= a - b;
      return( d>=0 ? d : (a + 0xFFFF-b) );
   } // diffWrap16

public:
   CTimer (int8_t iTN=1) : HardwareTimer(iTN) { ; }

   void start (voidFuncPtr func, uint32_t ivl_us=1000)
   {  // All working on F103, with ADC
      pause();
#if 0
      setMode( TIMER_CH1, TIMER_OUTPUT_COMPARE );
      setCompare( TIMER_CH1, -1 );  // clamp to overflow defined by setPeriod()
      setPeriod(ivl_us);            // set appropriate prescale & overflow
      attachInterrupt( TIMER_CH1, func );
#else // simple overflow interrupt - 
      //setMode( 0, 0 ); setOverflow(?); ticks or us?
      setPeriod(ivl_us);
      refresh();  // Commit parameters count, prescale, and overflow
      attachInterrupt( 0 , func ); 
#endif
      resume();   // Start counting
   }

   void nextIvl (void) { ++nIvl; }

   uint16_t diff (void) const { return diffWrap16(nIvl, nRet); }

   void retire (uint16_t d) { if (-1 == d) { nRet= nIvl; } else { nRet+= d; } }

   uint16_t swTickVal (void) const { return(nIvl); }
   uint16_t hwTickVal (void) { return getCount(); }
/*
   void hwSpinWait (uint16_t until)
   {
      uint16_t now, oflo= getOverflow();
      if (until > oflo) { until-= oflo; }
      now= hwTickVal();
      do
      {
         now= hwTickVal();
      } while (now < until);
   } // hwTickSpin
*/
   // DEBUG
   void dbgPrint (Stream& s)
   {
#if 0 //ndef ARDUINO_ARCH_STM32F4
      //s.print("hwClkCfg "); s.println( getClkCfg1(), HEX); //s.print(','); s.println(c[1],HEX); ;
      s.print("hwClkRate= "); s.print( hwClkRate() ); s.println("MHz"); // getClockSpeed() ?
#endif
      s.print("hwTickRes= "); s.println( getPrescaleFactor() );
      s.print("hwTickOflo= "); s.println( getOverflow() );
      s.print("hwTickVal= "); s.print( hwTickVal() ); s.print(','); s.println( hwTickVal() );
   } // dbgPrint

}; // CTimer

#if 1 // clock

#include "dateTimeUtil.h"

#include <RTClock.h>

class DateTime : public tm_t
{
public:
   DateTime (void) { memset(this, 0, sizeof(*this)); } //const char *ahms=NULL) { if (ahms) { hmsFromA(ahms);} }

// Doesn't belong in class, but uses a method (erroneous declaration)
// Scan n packed bcd4 digit pairs at locations spaced by ASCII stride
void u8bcd4FromA (uint8_t u[], const int8_t n, const char a[], const uint8_t aStride=3) const
{
   for (int8_t i=0; i<n; i++) { u[i]= bcd2bin( bcd4FromA(a+(i*aStride), 2) ); }
} // u8bcd4FromA
void u8bcd4ToA (char a[], const uint8_t u[], const int8_t n, const uint8_t aStride=3) const
{
   for (int8_t i=0; i<n; i++) { hex2ChU8( a+(i*aStride), bin2bcd(u[i]) ); }
} // u8bcd4ToA

   // CAVEAT: Hacky assumption of element ordering...
   // Assumes hh:mm:ss format. No parse/check!
   void hmsFromA (const char a[]) { u8bcd4FromA(&hour,3,a); }
   void yFromA (const char a[]) { int y= atoi(a); if (y > 1970) { year= y-1970; } }
   void mFromA (const char a[]) { int8_t m= monthNumJulian(a); if (m > 0) { month= m; } }
   void dFromA (const char a[]) { day= bcd2bin(bcd4FromASafe(a,2)); }

   void mdyFromA (const char a[]) { mFromA(a); dFromA(a+5); yFromA(a+7); }

   int strHMS (char s[], const int m, const signed char endCh=0) const
   // { return snprintf(s, m, "%02u:%02u:%02u%c", hour, minute, second, endCh);  }
   {
      const int n= 8+(endCh>0);
      if (m >= n)
      {
         u8bcd4ToA(s, &hour, 3, 3);
#if 0
         hex2ChU8(s+0,bin2bcd(hour));
         hex2ChU8(s+3,bin2bcd(minute));
         hex2ChU8(s+6,bin2bcd(second));
#endif
         s[2]= s[5]= ':';
         s[8]= endCh;
         if (n > 8) { s[n]= 0; }
         return(n);
      }
      return(0);
   } // strHMS
   
   int strYMD (char s[], const int m, const char endCh=0) const
      { return snprintf(s, m, "%u/%u/%u%c", 1970+year, month, day, endCh);  }

   void print (Stream& s, uint8_t opt=0x00) const
   {
static const char endCh[]={' ','\t','\n',0x00};
      char b[16];
      if (opt & 0xF0)
      {
         //int8_t i= 
         strYMD(b, sizeof(b)-1, endCh[(opt>>4)&0x3]);
         s.print(b);
      }
      strHMS(b, sizeof(b)-1, endCh[opt&0x3]);
      s.print(b);
   } // printTime
}; // DateTime

class CClock : public RTClock, public DateTime
{
public:
   CClock () { ; }

   void setA (const char amdy[], const char ahms[])
   {
      mdyFromA(amdy);
      hmsFromA(ahms);
      setTime(makeTime(*this));
   }

   void print (Stream& s, uint8_t opt=0x00)
   {
      getTime(*this);
      DateTime::print(s,opt);
   }
}; // CClock

#endif // clock


#endif // ST_TIMING_HPP
