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
