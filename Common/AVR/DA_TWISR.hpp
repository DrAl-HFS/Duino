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
// (c) Project Contributors Mar 2022

#include "DA_TWI.hpp"

struct Frag { uint8_t *pB, nB, f; };

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

#define SZ(i,v)  if (0 == i) { i= v; }

class TWISR : protected TWI::SWS, TWBuffer
{
protected:

   void start (void)
   {
      TWI::SWS::start();
      TWI::HWS::start();
   } // start

   void stop (void)
   {
      TWI::HWS::stop();
      TWI::SWS::stop();
   } // stop

public:
   TWISR (void) { ; }

   using SWS::sync;
   bool sync (uint8_t wait) const
   {
      if (wait > f.nB) { wait= f.nB; }
      return TWI::SWS::sync(wait);
   } // sync

   using TWI::HWS::setClk;

   int write (const uint8_t devAddr, const uint8_t b[], const uint8_t n)
   {
      if (sync())
      {
         TWI::HWS::sync();
         hwAddr= (devAddr << 1) | TW_WRITE;
         f.pB= (uint8_t*)b; // unconst hack
         f.nB= n;
         start();
         return(n);
      }
      return 0;
   } // write

   int read (const uint8_t devAddr, uint8_t b[], const uint8_t n)
   {
      if (sync())
      {
         hwAddr= (devAddr << 1) | TW_READ;
         f.pB= b;
         f.nB= n;
         start();
         return(n);
      }
      return 0;
   } // read
   // W[2] 2 5 4 4  
   // R[32] 2 6 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 11
   int8_t event (const uint8_t flags)
   {
      int8_t iE=0;
      switch(flags)
      {
         case TW_MR_DATA_ACK : SZ(iE,1);  // Master has acknowledged receipt
            readByte();
            if (more()) { ack();	} // if further data expected, acknowledge
            else { end();	}
            break;

         case TW_START : SZ(iE,2);
         case TW_REP_START : SZ(iE,3);
            if (retry())  { commit(); }
            else { stop(); } // multiple NACKs -> abort
            break;

         case TW_MT_DATA_ACK : SZ(iE,4); // From slave device
            if (more())
            {	 // Send more data
               writeByte();
               resume();
            }
            else { stop(); } // Assume end of data
            break;

         case TW_MT_SLA_ACK :	 SZ(iE,5); // From slave device (address recognised)
            clear(); // Transaction proceeds
            writeByte();
            resume();
            break;

         case TW_MR_SLA_ACK : SZ(iE,6); // Slave acknowledged address
            if (more()) { ack();	}      // acknowledge to continue reading 
            else { resume(); } // ??
            break;

         // --- terminus est ---

         case TW_MT_SLA_NACK : SZ(iE,7);  // No address ack, disconnected / jammed?
         case TW_MR_SLA_NACK : SZ(iE,8);
            //retry();    // Assume retry avail
            restart();  // Stop-start should clear any jamming of bus
            break;

         case TW_MT_DATA_NACK :	SZ(iE,9); stop(); break; // Slave didn't acknowledge data

         // Multi-master
         case TW_MT_ARB_LOST : SZ(iE,10); break;

         case TW_MR_DATA_NACK : 	SZ(iE,11); // ack not sent
            readByte(); // get final byte
            stop();
            break;
      } // switch
      return(iE);
   } // event

}; // class TWISR

class TWIUtil : public TWISR
{
public:
   //TWIUtil (void) { ; }

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
   
}; // class TWIUtil

/* Auto-sync hack, deprecated
class TWISync : public TWISR
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
class TWIDebug : public TWIUtil // TWISync
{
public:
   uint8_t evQ[64];
   int8_t iQ;
   uint8_t evF[12];

   TWIDebug (void) { clrEv(); }

   void clrEv (void) { iQ= 0; for (int8_t i=0; i<sizeof(evF); i++) { evF[i]= 0; } }
   
   int8_t event (const uint8_t flags)
   {
      int8_t iE= TWISR::event(flags);
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

}; // class TWISRD

// Instance and link to ISR

TWIDebug gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}









