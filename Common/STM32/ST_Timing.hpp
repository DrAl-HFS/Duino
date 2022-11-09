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
   {  // All working on F103, with ADC. Still no joy on F401...
      pause();
#if 0
      setMode( TIMER_CH1, TIMER_OUTPUT_COMPARE );
      setCompare( TIMER_CH1, -1 );  // clamp to overflow defined by setPeriod()
      setPeriod(ivl_us);            // set appropriate prescale & overflow
      attachInterrupt( TIMER_CH1, func );
#else // simple overflow interrupt -
      //setMode( 0, 0 ); setOverflow(?); ticks or us?
      setPeriod(ivl_us);
      setMode( 1, TIMER_OUTPUT_COMPARE );
      refresh();  // Commit parameters count, prescale, and overflow
      attachInterrupt( 0 , func );
      attachInterrupt( 1 , func );
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
      //s.print("TIMER_CH1=");s.println(TIMER_CH1);
      s.print("hwTickRes= "); s.println( getPrescaleFactor() );
      s.print("hwTickOflo= "); s.println( getOverflow() );
      s.print("hwTickVal= "); s.print( hwTickVal() ); s.print(','); s.println( hwTickVal() );
   } // dbgPrint

}; // CTimer

#if 1 // clock

#include "../dateTimeUtil.hpp"

#include <RTClock.h>

class DateTime : public tm_t
{
public:
   DateTime (void) { memset(this, 0, sizeof(*this)); } //const char *ahms=NULL) { if (ahms) { hmsFromA(ahms);} }

// Doesn't belong in class, but uses a method (erroneous declaration)
// Scan n packed bcd4 digit pairs at locations spaced by ASCII stride
/*
void u8bcd4FromA (uint8_t u[], const int8_t n, const char a[], const uint8_t aStride=3) const
{
   for (int8_t i=0; i<n; i++) { u[i]= bcd2bin( bcd4FromA(a+(i*aStride), 2) ); }
} // u8bcd4FromA
*/
   void u8ToBCD4ToA (char a[], const uint8_t u[], const int8_t n, const uint8_t aStride=3) const // TODO : factor out
   {
      for (int8_t i=0; i<n; i++)
      {
         uint8_t bcd;
         bcd4FromU8(&bcd, u[i]);
         hexChFromU8( a+(i*aStride), bcd);
      }
   } // u8ToBCD4ToA

   int bytesBCD4 (void) { return(6); } // yy/mm/dd,hh:mm:ss 12digits = 6bytes

   int getBCD4 (uint8_t bcd[6])
   {
      bcd4FromU8(bcd+0, (year + 1970) % 100);
      bcd4FromU8(bcd+1, month);
      bcd4FromU8(bcd+2, day);
      bcd4FromU8(bcd+3, hour);
      bcd4FromU8(bcd+4, minute);
      bcd4FromU8(bcd+5, second);
      return(6);
   }

   // CAVEAT: Hacky assumption of element ordering...
   // Assumes hh:mm:ss format. No parse/check!
   void hmsFromA (const char a[]) { u8FromTimeA(&hour,a); } // u8bcd4FromA(&hour,3,a); }
   //void yFromA (const char a[]) { int y= atoi(a); if (y > 1970) { year= y-1970; } }
   //void mFromA (const char a[]) { int8_t m= monthNumJulian(a); if (m > 0) { month= m; } }
   //void dFromA (const char a[]) { day= bcd2bin(bcd4FromASafe(a,2)); }

   void mdyFromA (const char a[]) // { mFromA(a); dFromA(a+5); yFromA(a+7); }
   {
      uint8_t u[3];
      u8FromDateA(u,a);
      year= atoi(a+7)-1970;
      month= u[1];
      day=  u[2];
   } // mdyFromA

   int strHMS (char s[], const int m, const signed char endCh=0) const
   {
      const int n= 8+(endCh>0);
      if (m >= n)
      {
         u8ToBCD4ToA(s, &hour, 3, 3);
         s[2]= s[5]= ':';
         s[8]= endCh;
         if (n < m) { s[n]= 0x00; }
         return(n);
      }
      return(0);
   } // strHMS

   int strYMD (char s[], const int m, const char endCh=0) const // return snprintf(s, m, "%u/%u/%u%c", 1970+year, month, day, endCh);
   {
      const int n= 8+(endCh>0);
      if (m >= n)
      {
         uint8_t bcd[3];
         //bcd4FromU16(bcd+0, year+1970 );
         bcd4FromU8(bcd+0, (year+1970) % 100 );
         bcd4FromU8(bcd+1, month);   // TODO : iterative versions?
         bcd4FromU8(bcd+2, day);
         hexChFromU8(s+0, bcd[0]);
         hexChFromU8(s+3, bcd[1]);
         hexChFromU8(s+6, bcd[2]);
         s[2]= s[5]= '/';
         s[8]= endCh;
         if (n < m) { s[n]= 0x00; }
         return(n);
      }
      return(0);
   } // strYMD

   void print (Stream& s, uint8_t opt=0x00) const
   {
static const char endCh[]={0x0,' ','\t','\n'};
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

   // yymmddhhmmss
   int getBCD4 (uint8_t bcd[6])
   {
      getTime(*this);
      return DateTime::getBCD4(bcd);
   }

   void print (Stream& s, uint8_t opt=0x13)
   {
      getTime(*this);
      DateTime::print(s,opt);
   }

}; // CClock

#endif // clock


#endif // ST_TIMING_HPP
