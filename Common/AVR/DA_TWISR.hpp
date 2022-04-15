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

#include <util/twi.h> // TW_MT_* TW_MR_* etc definitions

#ifndef CORE_CLK
#define CORE_CLK 16000000UL	// 16MHz
#endif

struct Frag { uint8_t *pB, nB, f; };

//Frag fragB[2];
#define BUSY 7

namespace TWI
{
   enum Clk : uint8_t   // Magic numbers for wire clock rate (when prescaler=1)
   {
      CLK_100=0x48, CLK_150=0x2D, // Some approximate ~ +300Hz
      CLK_200=0x20, CLK_250=0x18,
      CLK_300=0x12, CLK_350=0x0E,
      CLK_400=0x0C   // Higher rates presumed unreliable.
   };
}; // namespace TWI

class TWIHWS // hardware state
{
protected:
   uint8_t getBT0 (uint8_t r)
   {
      switch(r)
      {
         case TWI::CLK_400 : return(23); // 22.5us
         case TWI::CLK_100 : return(90);
         default :
         {
            uint8_t n= r / TWI::CLK_400;
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

   // NB: Interrupt flag (TWINT) is cleared by writing 1

   void start (void) { TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE); }

   void restart (void) { TWCR|= (1<<TWINT)|(1<<TWSTA)|(1<<TWSTO); }

   void commit (uint8_t hwAddr)
   {
      TWDR= hwAddr; 			// Transmit SLA + Read or Write
      TWCR&= ~(1<<TWSTA);	// TWSTA must be cleared by software! This also clears TWINT!!!
   } // commit

   void stop (void) { TWCR|= (1<<TWINT)|(1<<TWSTO); }

   void ack (void) { TWCR|= (1<<TWINT)|(1<<TWEA); }

   void end (void) { TWCR&= ~(1<<TWEA); }

   void sync (void) { while(TWCR & (1<<TWSTO)); } // wait for bus stop condition to clear

   void resume (void) { TWCR|= (1<<TWINT); }

public:
   TWIHWS (void) { ; }

   void setClk (TWI::Clk c=TWI::CLK_400)
   {
      //TWSR&= ~0x3; // unnecessary, status bits not writable anyway
      TWSR= 0x00; // clear PS1&0 so clock prescale= 1 (high speed)
      TWBR= c;
   } // setClk

   uint32_t setClk (uint32_t fHz)
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
   } // setClk

}; // TWIHWS

class TWISWS : protected TWIHWS // software state sits atop hardware state
{
protected:
   volatile uint8_t status, retry_cnt;
   uint8_t hwAddr;

public:
   TWISWS (void) : status{0}, retry_cnt{0} { ; }

   void start (void)
   {
      clear();
      status |= (1<<BUSY);
   } // start

   void commit (void) { TWIHWS::commit(hwAddr); }

   void stop (void) { status&= ~(1<<BUSY); }

   bool sync (void) const { return(0 == (status & (1<<BUSY))); }

   bool sync (const uint8_t nB) const
   {
      bool r= sync();
      if (r || (nB <= 0)) { return(r); }
      //else
      const uint16_t u0= micros();
      const uint16_t u1= u0 + nB * getBT();
      uint16_t t;
      do
      {
         if (sync()) { return(true); }
         t= micros();
      } while ((t < u1) || (t > u0)); // wrap safe (order important)
      return sync();
   } // sync

   bool retry (bool commit=true)
   {
      bool r= retry_cnt < 3;
      if (commit) { retry_cnt+= r; } // retry_cnt+= commit&&r; ? more efficient ?
      return(r);
   } // retry

   void clear (void) { retry_cnt= 0; }

}; // class TWISWS

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

class TWISR : protected TWISWS, TWBuffer
{
protected:

   void start (void)
   {
      TWISWS::start();
      TWIHWS::start();
   } // start

   void stop (void)
   {
      TWIHWS::stop();
      TWISWS::stop();
   } // stop

public:
   TWISR (void) { ; }

   using TWISWS::sync;
   bool sync (uint8_t wait) const
   {
      if (wait > f.nB) { wait= f.nB; }
      return TWISWS::sync(wait);
   } // sync

   using TWIHWS::setClk;

   int write (const uint8_t devAddr, const uint8_t b[], const uint8_t n)
   {
      if (sync())
      {
         TWIHWS::sync();
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









