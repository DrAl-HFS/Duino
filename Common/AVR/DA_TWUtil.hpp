// Duino/Common/AVR/DA_TWISR.hpp - AVR two wire (I2C-like) interrupt driven communication, C++ class.
// Adapted from C-code authored by "kkempeneers" (as "twi.c" - without copyright or licence declaration)
// obtained from:-
// https://www.avrfreaks.net/sites/default/files/project_files/Interrupt_driven_Two_Wire_Interface.zip
// This approach provides efficient asynchronous communication without buffer copying.
// (SRAM access 2 cycles on 8b bus - so memcpy() is 4clks/byte ie. 4MB/s @ 16MHz.)
// ---
// * CAVEAT EMPTOR *
// This could break in a variety of interesting ways under Arduino, subject to resource requrements.
// ---
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar - Apr 2022

#include "DA_TWISR.hpp"
#include "DA_TWMISR.hpp"

//struct Frag { uint8_t *pB, nB, f; };
/*
class TWBuffer
{
protected:
   Frag f;

public:
   TWBuffer (void) { ; }

   uint8_t& byte (void) { return(*f.pB++); }

   bool more (void) // { f.nB-= (f.nB > 0); return(f.nB > 0); }
   { return((int8_t)(--f.nB) > 0); }

   void readByte (void) { byte()= TWDR; }
   void writeByte (void) { TWDR= byte(); }
}; // TWBuffer
*/
#define SZ(i,v)  if (0 == i) { i= v; }


class TWUtil : public TWI::ClkUtil, public TWISR // TWMISR, public SWS
{
public:
   //TWUtil (void) { ; }
   using TWI::SWS::sync;
   
   bool sync (const uint8_t nB) const
   {
      bool r= sync();
      if (r || (nB <= 0)) { return(r); }
      //else
      const uint16_t u0= micros();
      const uint16_t u1= u0 + nB * TWI::ClkUtil::getBT();
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
      return(r);
   } // readSync

}; // class TWUtil

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
class TWDebug : public TWUtil // TWSync
{
public:
   uint8_t evQ[64];
   int8_t iQ;
   uint8_t evF[12];

   TWDebug (void) { clrEv(); }

   using TWI::ClkUtil::setClk;
   
   void clrEv (void) { iQ= 0; for (int8_t i=0; i<sizeof(evF); i++) { evF[i]= 0; } }

   int8_t event (const uint8_t flags)
   {
      int8_t iE= TWUtil::event(flags);
      evF[iE]+= (evF[iE] < 0xFF);   // saturating increment
      evQ[iQ]= iE;
      iQ+= (iQ < sizeof(evQ));
   } // event

   void dump (Stream& s) const
   {
      s.print("Q:");
      for (int8_t i=0; i<iQ; i++) { s.print(' '); s.print(evQ[i]); }
      s.print("\nF:");
      for (int8_t i=0; i<sizeof(evF); i++) { s.print(' '); s.print(evF[i]); }
      s.println();
   } // dump

}; // class TWDebug

// Instance and link to ISR

TWDebug gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}
