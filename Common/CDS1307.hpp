// Duino/Common/CDS1307.hpp - class wrapper for Dallas basic RTC with battery backed storage
// https://github.com/DrAl-HFS/Duino.git ?
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef CDS1307_HPP
#define CDS1307_HPP

#include "DN_Util.hpp"
#ifdef ARDUINO_ARCH_AVR
#include "AVR/DA_TWUtil.hpp"
#define I2C_BASE_CLASS TWUtil::CCommonTW
#else
#include "CCommonI2C.hpp"
#define I2C_BASE_CLASS CCommonI2CX1
#endif

namespace DS1307HW
{
  enum Device : uint8_t { ADDR=0x68 };  // NB: 7msb -> 0xD0|RNW on wire
  enum Reg : uint8_t {
    T_SS, T_MM, T_HH,   DOW,
    D_DD, D_MM, D_YY,   SQ_CTRL };
  enum User : uint8_t { FIRST=8, LAST=63 };
  enum TDFlag : uint8_t { T_SS_STOP=0x80, T_HH_12H=0x40, T_HH_12H_PM=0x20 };
  //enum Mask : uint8_t { T_SS_MASK=0x7F,
  enum SqCtrl : uint8_t { SQ_OUT=0x80, SQWE=0x20, RSM=0x3 };
  //_B6=0x80
}; // namespace DS1307HW

class CDS1307 : protected I2C_BASE_CLASS
{
protected:
   uint8_t devAddr (void) { return(DS1307HW::ADDR); }

   // CAVEATS:
   // 1) due to order reversal, <n> measures from rightmost
   // arg ie. truncation is conceptually from left so that
   // if n=1 then seconds will be set.
   // 2) An extra byte is required to append the starting
   // register address (sent first due to order reversal).
   int setDateTimeBCD (uint8_t ymdwhmsA[], int n=7)
   {
      if ((0x7 & n) != n) { return(0); }
      // Functionally elegant but programmatically obscure...
      ymdwhmsA[n]= DS1307HW::SQ_CTRL-n; // starting register address
      return writeToRev(devAddr(), ymdwhmsA, n+1)-1;
   } // setDateTimeBCD

public:
   //using Clk::set;
   //CDS1307 (void) { ; }

   int readTimeBCD (uint8_t hms[], int n=3)
   {
#if 1
      uint8_t w[1]={ DS1307HW::T_SS }; // Garbage...
      int r= writeToThenReadFromRev(devAddr(), w, sizeof(w), hms, n);
      if (r >= n) { return(n); } else { return(0); }
#else
      int r= writeTo(devAddr(), DS1307HW::T_SS);
      if (r > 0) { r= readFromRev(devAddr(), hms, n); }
      return(r);
#endif
   } // readTimeBCD

   int readDateBCD (uint8_t ymdw[], int n=3)
   {
      int r= writeTo(devAddr(), DS1307HW::D_DD-(4 == n));
      if (r > 0) { r= readFromRev(devAddr(), ymdw, n); }
      return(r);
   } // readDateBCD

}; // CDS1307

int clamp (const int x, const int a, const int b)
{ // assert(a <= b);
  if (x < a) { return(a); }
  if (x > b) { return(b); }
  return(x);
} // clamp

class CDS1307Util : public CDS1307
{
public:
   //CDS1307Util (void) { ; }

   void setSqCtrl (DS1307HW::SqCtrl mode)
   {
      const uint8_t am[2]={DS1307HW::SQ_CTRL, mode}; // This works
      writeTo(devAddr(), am, 2);
   } // setSqCtrl

   int adjustSec (const int dSec=1)
   {
      int r=0, s=0;
      if (0 != dSec)
      {
         const uint8_t bcd[2]= { 0x00, 0 };
         if (readTimeBCD(bcd,1))
         {
            s= fromBCD4(bcd[0],2);
            r= clamp(s + dSec, 0, 59);
            if (r != s)
            {
               bcd4FromU8(bcd, r);
               //writeTo(devAddr(), bcd, 2);
               setDateTimeBCD(bcd,1);
            }
         }
      }
      return(r-s);
   } // adjustSec

}; // CDS1307Util

// ASCII (debug) conversions
#include "dateTimeUtil.hpp"
class CDS1307A : public CDS1307Util
{
protected:

   // Factor out -> Duino ?
   void printDTFBCD (Stream& s, char f[4], const uint8_t b[3], const char end)
   {
      hexChFromU8(f, b[0]);
      s.print(f);
      hexChFromU8(f, b[1]);
      s.print(f);
      hexChFromU8(f, b[2]);
      f[2]= end;
      s.print(f);
   } // printDTFBCD

   void printDateBCD (Stream& s, const uint8_t ymd[3], const char end)
   {
      char f[4]="00/";
      printDTFBCD(s,f,ymd,end);
   } // printDateBCD

   void printTimeBCD (Stream& s, const uint8_t hms[3], const char end)
   {
      char f[4]="00:";
      printDTFBCD(s,f,hms,end);
   } // printTimeBCD

   uint8_t consDOW (uint8_t dow) { dow&= 0x7; dow+= (0 == dow); return(dow); } // -> 1..7

public:
   //CDS1307 (void) { ; }

   int printTime (Stream& s, const char end='\n')
   {
      uint8_t hms[3];
      const int r= readTimeBCD(hms);
      if (r > 2)
      {
         printTimeBCD(s,hms,end);
         if (DS1307HW::T_SS_STOP == hms[r-1]) { return(-1); }
      }
      return(r);
   } // printTime

   int printDate (Stream& s, const char end='\n')
   {
      uint8_t ymd[3];
      const int r= readDateBCD(ymd);
      if (r > 2) { printDateBCD(s,ymd,end); }
      return(r);
   } // printTime

   int setA (const char *time, const char *day=NULL, const char *date=NULL, int r=1)
   {
      uint8_t ymdwhmsA[8];
      int o=0,n=0;

      if (0 == r)
      {
         uint8_t s[2];
         readTimeBCD(s,1);
         if (DS1307HW::T_SS_STOP == s[0]) { r= 2; }
      }
      if (date)
      {
         bcd4FromDateA(ymdwhmsA,date);
         n= 3;
      } else { o= 3; }
      if (day) { ymdwhmsA[3]= dayNumEn(day); }
      else
      {  // 2000 (leap year) began Sat (2001,2007,2018 begin with Mon, 2020 begin We)
         o+= (n < 3);
         if (n > 0)
         {  // DOW calculation
            uint8_t ymd[3];
            for (int i= 0; i<3; i++) { ymd[i]= fromBCD4(ymdwhmsA[i],2); }
            //if (ymd[0] >= 20) { ymd[0]-= 20; }
            unsigned int sd= sumDaysJulianU8(ymd);
            // 2000/01/01 -> Sat (6th day -> idx 5)
            ymdwhmsA[3]= 1 + ((5 + sd) % 7); // NB: modulo works on *index* not number
         }
      }
      if (n > 0) { ymdwhmsA[3]= consDOW(ymdwhmsA[3]); n++; }
      if (time)
      {
         bcd4FromTimeA(ymdwhmsA+4,time);
         n+=3;
      }
      if (1 == r)
      {  // set forward only
         uint8_t v[8];
         readTimeBCD(v,7);
         v[0]&= ~DS1307HW::T_SS_STOP;
         ymdwhmsA[7]= v[7]= 0;
         r= strcmp(ymdwhmsA,v); // NB: potential for breakage by DOW...
         /* r= sign((unsigned int)ymdwhmsA[i] - (unsigned int)v[i]);  */
      }
      if (r > 0) { r= setDateTimeBCD(ymdwhmsA+o,n); }
      return(r);
   } // setA

   // hack
   unsigned int testDays (Stream& s, const char *date)
   {
      uint8_t ymd[4];
      u8FromDateA(ymd,date);
      unsigned int sd, dYM[2];
      sd= setDaysJulianU8(dYM,ymd);
      ymd[3]= 1 + ((5 + sd) % 7); // 2020 Jan 1st -> Sat
      s.print("CDS1307A::testDays() ");
      for (int i=0; i<4; i++) { s.print(ymd[i]); s.print(' '); }
      s.print("-> ");
      s.print(dYM[0]); s.print(' '); s.print(dYM[1]); s.print(' '); s.println(sd);
   }

   // -> debug
   void dump (Stream& s)
   {
      uint8_t b[32];
      uint16_t tm[2];
      uint8_t q=sizeof(b), r, i=0, dt=0;
      writeTo(devAddr(),0x00); // reset internal r/w ptr
      s.println("CDS1307A::dump() - ");
      do
      {
         tm[0]= millis();
         r= 1+DS1307HW::LAST - i;
         if (r < q) { q= r; }
         r= readFrom(devAddr(), b, q);
         tm[1]= millis(); //  s.print('/'); s.print(DS1307HW::USR_LAST);
         //s.print("q,r,i= "); s.print(q); s.print(','); s.print(r); s.print(','); s.print(i); s.print(": ");
         dumpHexTab(s, b, r);
         dt+= tm[1]-tm[0];
         i+= r;
      } while (i <= DS1307HW::LAST);
      s.print("dt="); s.println(dt);
   } // dump

}; // CDS1307A

#endif // CDS1307_HPP
