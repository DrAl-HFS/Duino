// Duino/Common/AVR/DA_TWUtil.hpp - AVR two wire (I2C-like) adaptor/utility
// code, C++ classes.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar-May 2022

#ifndef DA_TWUTIL_HPP
#define DA_TWUTIL_HPP

#include "DA_TWMISR.hpp"
#include <avr/sleep.h>


#ifndef CORE_CLK
#define CORE_CLK 16000000UL	// 16MHz
#endif

namespace TWUtil {

class CClk
{
protected:
   uint8_t getBT0 (uint8_t r)
   {
      switch(r)
      {
         case TWM::CLK_400 : return(23); // 22.5us
         case TWM::CLK_100 : return(90);
         default :
         {
            uint8_t n= r / TWM::CLK_400;
            return( (22 * n) + (n / 2) + (n & 0x1) ); // = (22.5 * n) + 0.5
         }
      }
   } // getBT0

   uint8_t getBT (void)
   {
      uint8_t t0= getBT0(I2C.getClkT());
      switch (I2C.getClkPS())
      {
         case 0x00 : return(t0);
         case 0x01 : return(4*t0);
         // others hopelessly slow...
         //case 0x02 : return(16*t0); // >= 360us per byte
         //case 0x03 : return(64*t0); // >= 1.44ms per byte
      }
      return(250); // just a hack...
   } // setBT

public:

   // General clock rate quert/change, inefficient for regular use on AVR
   // due (sloooow) software division.
   uint32_t getSet (uint32_t fHz=0)
   {
      uint32_t sc= 0;
      if (fHz > 0)
      {
         if (fHz >= 100000) { I2C.setClkPS(0x00); sc= CORE_CLK; }
         else if (fHz >= 475)
         {
            I2C.setClkPS(0x03);
            sc= CORE_CLK / 64;
         }
         if (sc > 0)
         {  //s.print(" -> "); s.print(TWBR,HEX); s.print(','); s.println(TWSR,HEX); TWBR=
            uint8_t t= ((sc / fHz) - 16) / 2;
            I2C.setClkT(t);
         }
      } else { sc= CORE_CLK / (1 << (4 * I2C.getClkPS())); }
      return(sc / ((I2C.getClkT() * 2) + 16));
   } // getSet

}; // class CClk

/* using Clk::set;
   void reset (ClkTok c)
   {
      HWRC::stop(); // ineffective? better to rely on power off-on...
      if (CLK_INVALID != c) { Clk::set(c); }
   } // reset
*/

class CSync : public CClk
{
public:
   Sync (void) { set_sleep_mode(SLEEP_MODE_IDLE); sleep_enable(); }

   //using CClk::getSet;

   bool sync (const uint8_t nB) const
   {
      bool r= I2C.sync();
      if (r || (nB <= 0)) { return(r); }
      //else
      const uint16_t u0= micros();
      const uint16_t u1= u0 + nB * CClk::getBT(); // (uint16_t) ?
      uint16_t t;
      do
      {
         if (I2C.sync()) { return(true); }
         sleep_cpu();
         t= micros();
      } while ((t < u1) || (t > u0)); // wrap safe (order important)
      return I2C.sync();
   } // sync

}; // class CSync

class CCommonTW : public CSync, public CCommonTWAS
{
//public:
//   using Sync::sync;
protected:
   uint8_t ub[1];

   int transfer1 (const uint8_t devAddr, uint8_t b[], const uint8_t n, const TWM::FragMode m, uint8_t t=3)
   {
      int r;
      do
      {
         r= I2C.transfer1AS(devAddr,b,n,m);
         sync(-1);
      } while ((r <= 0) && (t-- > 0));
      return(r);
   } // transfer1

   int transfer2
   (
      const uint8_t devAddr,
      uint8_t b1[], const uint8_t n1, const TWM::FragMode m1,
      uint8_t b2[], const uint8_t n2, const TWM::FragMode m2,
      uint8_t t=3
   )
   {
      int r;
      do
      {
         r= I2C.transfer2AS(devAddr,b1,n1,m1,b2,n2,m2);
         sync(-1);
      } while ((r <= 0) && (t-- > 0));
      return(r);
   } // transfer1

public:
   //using CClk::getSet;

   int readFrom (const uint8_t devAddr, const uint8_t b[], const uint8_t n)
      { return transfer1(devAddr,b,n,TWM::READ); }

   int writeTo (const uint8_t devAddr, const uint8_t b[], const uint8_t n)
      { return transfer1(devAddr,b,n,TWM::WRITE); }

   int writeTo (const uint8_t devAddr, const uint8_t a, const uint8_t *p=NULL, const uint8_t n=0)
   {
      if ((NULL == p) || (0 == n)) { return transfer1(devAddr,&a,1,TWM::WRITE); }
      //else
      return transfer2(devAddr, &a,1,TWM::WRITE, p,n,TWM::WRITE);
#if 0
      if (I2C.sync()) // async ?
      {
         ub[0]= b;
         return writeTo(devAddr,ub,1);
      }
      else { return(0); }
#endif
   } // writeTo

   int writeToFill (const uint8_t devAddr, const uint8_t a, const uint8_t b, const uint8_t n)
      { return transfer2(devAddr, &a,1, TWM::WRITE, &b,n, TWM::REPB|TWM::WRITE); }

   int readFromRev (const uint8_t devAddr, const uint8_t b[], const uint8_t n)
      { return transfer1(devAddr,b,n,TWM::REV|TWM::READ); }

   int writeToRev (const uint8_t devAddr, const uint8_t b[], const uint8_t n)
      { return transfer1(devAddr,b,n,TWM::REV|TWM::WRITE); }

   int writeToRevThenFwd (uint8_t devAddr, const uint8_t bRev[], const int nRev, const uint8_t bFwd[], const int nFwd)
   {
      if ((nRev > 0) && (nFwd > 0))
      {
         return transfer2(devAddr, bRev, nRev, TWM::REV|TWM::WRITE, bFwd, nFwd, TWM::WRITE);
      }
      else return(0);
   } // writeToRevThenFwd

   int writeToThenReadFrom (uint8_t devAddr, const uint8_t bWF[], const int nWF, const uint8_t bRR[], const int nRR)
   {
      if ((nWF > 0) && (nRR > 0))
      {
         return transfer2(devAddr, bWF, nWF, TWM::WRITE, bRR, nRR, TWM::READ);
      }
      else return(0);
   } // writeToThenReadFrom

   int writeToThenReadFromRev (uint8_t devAddr, const uint8_t bWF[], const int nWF, const uint8_t bRR[], const int nRR)
   {
      if ((nWF > 0) && (nRR > 0))
      {
         return transfer2(devAddr, bWF, nWF, TWM::WRITE, bRR, nRR, TWM::REV|TWM::READ);
      }
      else return(0);
   } // writeToThenReadFrom

   int writeToRevThenReadFromFwd (uint8_t devAddr, const uint8_t bWR[], const int nWR, const uint8_t bRF[], const int nRF)
   {
      if ((nWR > 0) && (nRF > 0))
      {//TWM::RESTART|
         return transfer2(devAddr, bWR, nWR, TWM::REV|TWM::WRITE, bRF, nRF, TWM::READ);
      }
      else return(0);
   } // writeToThenReadFrom

}; // class CCommonTW

#if 0 // deprecated
class Debug2
{
   void capt (uint8_t v[3])
   {
      v[0]= iF;
      v[1]= frag[iF].nB;
      v[2]= iB;
   } // capt

   void rmMon (Stream& s)
   {
      uint8_t bst[2][3], rm[32], i=0;
      capt(bst[0]); rm[0]= remaining();
      do
      {
         const uint8_t t= remaining();
         if (t != rm[i]) { i+= (i < (sizeof(rm)-1)); rm[i]= t; }
      } while (rm[i] > 0);
      capt(bst[1]);
      s.print("writeToSync() -\nbst: ");
      dump<uint8_t>(s, bst[0], sizeof(bst[0]), "{", "} ");
      dump<uint8_t>(s, bst[1], sizeof(bst[1]), "{", "}\n");
      dump<uint8_t>(s, rm, i+1, "rm: ", "\n");
   } // rmMon

   int readFromSync (Stream& s, const uint8_t devAddr, const uint8_t b[], const uint8_t n, uint8_t t=3)
   {
      int r, r0;
      do
      {
         r= readFrom(devAddr,b,n);
         //sync(-1);
         if (r <= 0) { sync(-1); } else { rmMon(s); }
     } while ((r <= 0) && (t-- > 0));
      return(r);
   } // readFromSync

   int writeToSync (Stream& s, const uint8_t devAddr, const uint8_t b[], const uint8_t n, uint8_t t=3)
   {
      int r, r0;
      do
      {
         r= writeTo(devAddr,b,n);
         sync(-1);
         if (r <= 0) { sync(-1); } else { rmMon(s); }
      } while ((r <= 0) && (t-- > 0));
      return(r);
   } // writeToSync

}; // class Debug2
#endif

}; // namespace TWUtil

#endif // DA_TWUTIL_HPP

