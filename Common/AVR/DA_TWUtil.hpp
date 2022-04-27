// Duino/Common/AVR/DA_TWUtil.hpp - AVR two wire (I2C-like) utility code, C++ classes.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar - Apr 2022

// Hacky forward declaration (transitional)
struct Frag { uint8_t *pB, nB, f; };

//#include "DA_TWISR.hpp"
#include "DA_TWMISR.hpp"


#ifndef CORE_CLK
#define CORE_CLK 16000000UL	// 16MHz
#endif

namespace TWUtil {

enum ClkTok : uint8_t   // Tokens -> magic numbers for wire clock rate (for prescaler=1 and CORE_CLK=16MHz)
{
   CLK_100=0x48, CLK_150=0x2D, // Some approximate ~ +300Hz
   CLK_200=0x20, CLK_250=0x18, 
   CLK_300=0x12, CLK_350=0x0E, 
   CLK_400=0x0C,   // Higher rates presumed unreliable.
   CLK_INVALID=0x00 // -> 1MHz but not usable
};

class Clk
{
protected:
   uint8_t getBT0 (uint8_t r)
   {
      switch(r)
      {
         case CLK_400 : return(23); // 22.5us
         case CLK_100 : return(90);
         default :
         {
            uint8_t n= r / CLK_400;
            return( (22 * n) + (n / 2) + (n & 0x1) ); // = (22.5 * n) + 0.5
         }
      }
   } // getBT0
   
   uint8_t getBT (void)
   {
      uint8_t t0= getBT0(TWBR);
      switch (TWSR & 0x3)
      {
         case 0x00 : return(t0);
         case 0x01 : return(4*t0);
         //case 0x02 : return(16*t0); // >= 360us per byte
         //case 0x03 : return(64*t0); // >= 1.44ms per byte
      }
      return(250);
   } // setBT

public:

   void set (ClkTok c)
   {
      //TWSR&= ~0x3; // unnecessary, status bits not writable anyway
      TWSR= 0x00; // clear PS1&0 so clock prescale= 1 (high speed)
      TWBR= c;
   } // set

   uint32_t set (uint32_t fHz)
   {
      uint32_t sc= 0;
      if (fHz >= 100000) { TWSR= 0x00; sc= CORE_CLK; }
      else if (fHz >= 475)
      {
         TWSR= 0x03;
         sc= CORE_CLK / 64;
      } 
      if (sc > 0)
      {  //s.print(" -> "); s.print(TWBR,HEX); s.print(','); s.println(TWSR,HEX);
         TWBR= ((sc / fHz) - 16) / 2;
      }
      return(sc / ((TWBR * 2) + 16));
   } // set

}; // class Clk


class Sync : public Clk, public TWM::RW1
{
public:
   //TWUtil (void) { ; }
   
   /* Specific variant dependancy */
   //using TWI::SWS::sync;
   using TWM::ISR::sync;
   
   bool sync (const uint8_t nB) const
   {
      bool r= sync();
      if (r || (nB <= 0)) { return(r); }
      //else
      const uint16_t u0= micros();
      const uint16_t u1= u0 + nB * Clk::getBT();
      uint16_t t;
      do
      {
         if (sync()) { return(true); }
         t= micros();
      } while ((t < u1) || (t > u0)); // wrap safe (order important)
      return sync();
   } // sync

   int readFromSync (const uint8_t devAddr, const uint8_t b[], const uint8_t n, uint8_t t=3)
   {
      int r;
      do
      {
         r= readFrom(devAddr,b,n);
         sync(-1);
      } while ((r <= 0) && (t-- > 0));
      return(r);
   } // readFromSync

   int writeToSync (const uint8_t devAddr, const uint8_t b[], const uint8_t n, uint8_t t=3)
   {
      int r;
      do
      {
         r= writeTo(devAddr,b,n);
         sync(-1);
      } while ((r <= 0) && (t-- > 0));
      return(r);
   } // writeToSync

}; // class Sync

// Debug extension
class Debug : public Sync
{
public:
   uint8_t evQ[64];
   uint8_t evF[12];
   uint8_t stQ[8];
   int8_t iEQ, iSQ;

   TWDebug (void) { clrEv(); }

   //using Clk::set;
   void reset (ClkTok c)
   {
      HWRC::stop(); // ineffective? better to rely on power off-on...
      if (CLK_INVALID != c) { Clk::set(c); }
   } // reset

   void clrEv (void) { iEQ= 0; iSQ=0; for (int8_t i=0; i<sizeof(evF); i++) { evF[i]= 0; } }

   int8_t event (const uint8_t flags)
   {
      const int8_t iE= Sync::event(flags);
      if (iSQ < sizeof(stQ)) { stQ[iSQ++]= Buffer::state; }
      if (iE < sizeof(evF)) { evF[iE]+= (evF[iE] < 0xFF); }  // saturating increment
      if (iEQ < sizeof(evQ)) { evQ[iEQ++]= iE; }
      return(iE);
   } // event

   void logEv (Stream& s, const uint8_t sf=0xFE) const
   {
      s.print("SQ:");
      for (int8_t i=0; i<iSQ; i++) { s.print(" 0x"); s.print(stQ[i] & sf, HEX); }
      s.print("\nEQ:");
      for (int8_t i=0; i<iEQ; i++) { s.print(' '); s.print(evQ[i]); }
#if 0
      s.print("\nF:");
      for (int8_t i=0; i<sizeof(evF); i++) { s.print(' '); s.print(evF[i]); }
#endif
      s.println();
   } // log
   
}; // class Debug

class Debug2 : public Debug // seems broken...
{
   using Sync::writeToSync;
   using Sync::readFromSync;

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


}; // namespace TWUtil

// Instance and link to ISR

TWUtil::Debug gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}
