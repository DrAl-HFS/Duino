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
   CLK_400=0x0C   // Higher rates presumed unreliable.
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

   void set (ClkTok c=CLK_400)
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


class Sync : public Clk, public TWM::ISR
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

   int writeSync (const uint8_t devAddr, const uint8_t b[], const uint8_t n, uint8_t t=3)
   {
      int r;
      do
      {
         r= write(devAddr,b,n);
         sync(-1);
      } while ((r <= 0) && (t-- > 0));
      return(r);
   } // writeSync

   int readSync (const uint8_t devAddr, const uint8_t b[], const uint8_t n, uint8_t t=3)
   {
      int r;
      do
      {
         r= read(devAddr,b,n);
         sync(-1);
      } while ((r <= 0) && (t-- > 0));
      return(r);// get(b,r);
   } // readSync

}; // class Sync

/* Auto-sync hack, deprecated
class TWSync : public TWISR
{
   uint8_t syncSet, tickSet[2];

   int tickDiff (void) const { return((int)tickSet[1] - (int)tickSet[0]); }
   bool tickBounded (const uint8_t t, int d)
   {
      d= (d > 0);
      if ((t >= tickSet[0x1^d]) && (t <= tickSet[d])) { return(false); }
   } // tickBounded

   bool preSync (const bool wait)
   {
      if (wait)
      {
         if (TWISR::sync(true))
         {
            uint8_t t;
            int m= 0;

            if (tickSet[0] != tickSet[1])
            {  // still waiting
               const uint8_t tNow= millis();
               const int d= tickDiff();
               if (tickBounded(tNow,d))
               {
                  if (d > 0) { m= tickSet[1] - tNow; }
                  else { m= tickSet[1] + (0x100 - tNow); }
               }
            }
            else if (syncSet > 0) { m= syncSet; }
            if (m > 0) { delay(m); }
            syncSet= 0;
            tickSet[0]= tickSet[1]= t;
            return(true);
         }
         return(false);
      }
      //else
      if ((syncSet > 0) && TWISR::sync())
      {
         tickSet[0]= millis();
         tickSet[1]= tickSet[0] + syncSet;
         syncSet= 0;
         return(false);
      }
      const int d= tickDiff();
      if (0 != d)
      {
         uint8_t tNow= millis();
         if (tickBounded(tNow,d)) { return(false); } // still waiting
         //else complete
         tickSet[0]= tickSet[1]= tNow;
      }
      return(tickSet[0] == tickSet[1]);
   } // preSync

   void postSync (uint8_t tSync) { syncSet= tSync; }

public:
   TWISync (void) : syncSet{0}, tickSet{0} { ; }

   int write (uint8_t devAddr, uint8_t b[], uint8_t n, bool wait=false, uint8_t tPS=0)
   {
      int w= 0;
      if (preSync(wait))
      {
         w= TWISR::write(devAddr,b,n);
         if (w > 0) { postSync(tPS); }
      }
      return(w);
   } // write

   int read (uint8_t devAddr, uint8_t b[], uint8_t n, bool wait=false, uint8_t tPS=0)
   {
      int r= 0;
      if (preSync(wait))
      {
         r= TWISR::read(devAddr,b,n);
         if (r > 0) { postSync(tPS); }
      }
      return(r);
   } // read

}; // class TWISync
*/


// Debug extension
class Debug : public Sync
{
public:
   uint8_t evQ[64];
   uint8_t evF[12];
   uint8_t stQ[8];
   int8_t iEQ, iSQ;

   TWDebug (void) { clrEv(); }

   using Clk::set;
   
   void clrEv (void) { iEQ= 0; iSQ=0; for (int8_t i=0; i<sizeof(evF); i++) { evF[i]= 0; } }

   int8_t event (const uint8_t flags)
   {
      int8_t iE= Sync::event(flags);
      if (iSQ < sizeof(stQ)) { stQ[iSQ++]= Buffer::state; }
      if (iE < sizeof(evF)) { evF[iE]+= (evF[iE] < 0xFF); }  // saturating increment
      if (iEQ < sizeof(evQ)) { evQ[iEQ++]= iE; }
   } // event

   void dump (Stream& s, const uint8_t sf=0xFE) const
   {
      s.print("SQ:");
      for (int8_t i=0; i<iSQ; i++) { s.print(" 0x"); s.print(stQ[i] & sf, HEX); }
      s.print("\nEQ:");
      for (int8_t i=0; i<iEQ; i++) { s.print(' '); s.print(evQ[i]); }
      s.print("\nF:");
      for (int8_t i=0; i<sizeof(evF); i++) { s.print(' '); s.print(evF[i]); }
      s.println();
   } // dump

}; // class TWDebug

}; // namespace TWUtil

// Instance and link to ISR

TWUtil::Debug gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}
