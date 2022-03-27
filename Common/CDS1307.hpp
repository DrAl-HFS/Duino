// Duino/Common/CDS1307.hpp - class wrapper for Dallas basic RTC with battery backed storage
// https://github.com/DrAl-HFS/Duino.git ?
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef CDS1307_HPP
#define CDS1307_HPP

#include "DN_Util.hpp"
#include "CCommonI2C.hpp"

namespace DS1307HW
{
  enum Device : uint8_t { ADDR=0x68 };
  enum Reg : uint8_t {
    T_SS, T_MM, T_HH,   DOW,
    D_DD, D_MM, D_YY,   SQ_CTRL };
  enum User : uint8_t { FIRST=8, LAST=63 };
  enum TDFlag : uint8_t { T_SS_STOP=0x80, T_HH_12H=0x40, T_HH_12H_PM=0x20 };
  //enum Mask : uint8_t { T_SS_MASK=0x7F,
  enum SqCtrl : uint8_t { SQ_OUT=0x80, SQWE=0x20, RSM=0x3 };
  //_B6=0x80
}; // namespace DS1307HW

class CDS1307 : protected CCommonI2CX1
{
protected:
   // CAVEATS:
   // 1) due to order reversal, <n> measures from rightmost
   // arg ie. if n=1 then seconds will be set.
   // 2) An extra byte is required to append the starting
   // register address (sent first due to order reversal).
   int setDateTimeBCD (uint8_t ymdwhmsA[], int n=7)
   {
      if ((0x7 & n) != n) { return(0); }
#if 1 // Functionally elegant but programmatically obscure...
      ymdwhmsA[n]= DS1307HW::SQ_CTRL-n; // starting register address
      return writeToRev(DS1307HW::ADDR, ymdwhmsA, n+1)-1;
#else
      I2C.beginTransmission(DS1307HW::ADDR);
      I2C.write(DS1307HW::SQ_CTRL-n); // starting register address
      n= writeRev(ymdwhmsA,n);
      I2C.endTransmission();
      return(n);
#endif
   } // setDateTimeBCD

public:
   //CDS1307 (void) { ; }

   int readTimeBCD (uint8_t hms[], int n=3)
   {
      writeTo(DS1307HW::ADDR, DS1307HW::T_SS);
      return readFromRev(DS1307HW::ADDR, hms, n);
   } // readTimeBCD

   int readDateBCD (uint8_t ymdw[], int n=3)
   {
      writeTo(DS1307HW::ADDR, DS1307HW::D_DD-(4 == n));
      return readFromRev(DS1307HW::ADDR, ymdw, n);
   } // readDateBCD

   void setSqCtrl (DS1307HW::SqCtrl mode)
   {
      const uint8_t am[2]={DS1307HW::SQ_CTRL, mode}; // feasible ?
      writeTo(DS1307HW::ADDR, am, 2);
   } // setSqCtrl

   // Hack test
   int setSBCD (const uint8_t ss=0x59)
   {
      const uint8_t v[2]= { 0x00, ss }; // This works
      writeTo(DS1307HW::ADDR, v, 2);
   }

}; // CDS1307

// ASCII (debug) conversions
#include "dateTimeUtil.hpp"
class CDS1307A : public CDS1307
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
      if (r > 2) { printTimeBCD(s,hms,end); }
      return(r);
   } // printTime

   int printDate (Stream& s, const char end='\n')
   {
      uint8_t ymd[3];
      const int r= readDateBCD(ymd);
      if (r > 2) { printDateBCD(s,ymd,end); }
      return(r);
   } // printTime

   int setA (const char *time, const char *day=NULL, const char *date=NULL, bool grtr=true)
   {
      uint8_t ymdwhmsA[8];
      int o=0,n=0;
      if (date)
      {
         bcd4FromDateA(ymdwhmsA,date);
         n= 3;
      } else { o= 3; }
      if (day)
      {
         ymdwhmsA[3]= dayNumEn(day);
         n++;
      } else { o+= (3 == o); }
      if (o < 4) { ymdwhmsA[3]= consDOW(ymdwhmsA[3]); }
      if (time)
      {
         bcd4FromTimeA(ymdwhmsA+4,time);
         n+=3;
      }
      if (grtr)
      {
         uint8_t v[8];
         int r, i= readTimeBCD(v,7);
         v[0]&= ~DS1307HW::T_SS_STOP;
         ymdwhmsA[7]= v[7]= 0;
         r= strcmp(ymdwhmsA,v);
         if (0) // 7 == i)
         {
            i= o;
            do
            {
               r= sign((unsigned int)ymdwhmsA[i] - (unsigned int)v[i]);
            } while ((0 == r) && (++i < n));
         }
         if (r <= 0) { return(0); }
      }
      return setDateTimeBCD(ymdwhmsA+o,n);
   } // setA

   // -> debug
   void dump (Stream& s)
   {
      uint8_t b[16];
      uint16_t tm[2];
      uint8_t q=sizeof(b), r, i=0;
      writeTo(DS1307HW::ADDR,0x00); // reset internal r/w ptr
      s.println("CDS1307A::dump() - ");
      do
      {
         tm[0]= millis();
         r= 1+DS1307HW::LAST - i;
         if (r < q) { q= r; }
         r= readFrom(DS1307HW::ADDR, b, q);
         tm[1]= millis(); //  s.print('/'); s.print(DS1307HW::USR_LAST);
         //s.print("q,r,i= "); s.print(q); s.print(','); s.print(r); s.print(','); s.print(i); s.print(": ");
         dumpHexTab(s, b, r);
         s.print("dt="); s.println(tm[1]-tm[0]);
         i+= r;
      } while (i <= DS1307HW::LAST);
   } // dump

}; // CDS1307A

#endif // CDS1307_HPP
