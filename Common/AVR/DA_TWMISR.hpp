// Duino/Common/AVR/DA_TWISR.hpp - AVR two wire (I2C-like) master mode
// interrupt driven communication, encapsulated as C++ class.
// Adapted from C-code authored by Scott Nelson (as "twi.c" - without
// copyright or licence declaration) obtained from:-
// https://github.com/scttnlsn/avr-twi
// ---
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Apr 2022
//#include <avr/io.h>
//#include <avr/interrupt.h>
#include <util/twi.h>
//#include <string.h>
//#include "twi.h"

#ifndef TWI_BUFFER_LENGTH
#define TWI_BUFFER_LENGTH 48
#endif
static uint8_t gTWTB[TWI_BUFFER_LENGTH];

static struct
{
   volatile uint8_t state;
   uint8_t hwAddr;
   Frag f;
   uint8_t iB;
 //, bb[TWI_BUFFER_LENGTH];
} buff;

class TWMISR
{
   void start (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); }

   void stop (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); }

   void ack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); }

   void nack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE); }

   void send (uint8_t b) { TWDR= b; }

   void recv (void) { buff.f.pB[buff.iB++] = TWDR; }

protected:
   void reply (void)
   {
      if (buff.iB < (buff.f.nB - 1)) { ack(); }
      else { nack(); }
   }

   void unlock (void) { buff.state= 0; }

   bool lock (void)
   {
      bool r= sync();
      if (r) { buff.state= 0x1; }
      return(r);
   }

public:
   TWMISR (void) { ; } // gTWTB; } // ??? doesnt work ???

   /* transitional hacks
   void set (const uint8_t b[], const uint8_t n)
   {
      if (gTWTB != buff.f.pB) { memcpy(buff.f.pB, b, n); }
   }
   int get (uint8_t b[], int r)
   {
      if ((r > 0) && (gTWTB != buff.f.pB)) { memcpy(b, buff.f.pB, r); }
      return(r);
   } // get
   bool setb (int8_t i)
   {
      bool r= sync();
      if (r)
      {
         if (i & 0x1) { buff.f.pB= gTWTB; }
         else { buff.f.pB= buff.bb; }
      }
      return(r);
   } // setb
*/
   bool sync (void) const { return(0 == buff.state); }

   int write (uint8_t devAddr, const uint8_t b[], const uint8_t n)
   {
     if (lock())
     {
        buff.hwAddr= (devAddr << 1) | TW_WRITE;
        buff.f.pB= b;
        buff.f.nB= n;
        buff.iB=  0;
        //set(b,n);

        start();
        return(n);
      }
      return(0);
   } // write

   int read (uint8_t devAddr, uint8_t b[], const uint8_t n)
   {
     if (lock())
     {
        buff.hwAddr= (devAddr << 1) | TW_READ;
        buff.f.pB= b;
        buff.f.nB= n;
        buff.iB=  0;

        start();
        return(n);
      }
      return(0);
   } // read

#define SZ(i,v)  if (0 == i) { i= v; }

   int8_t event (const uint8_t flags)
   {
      int8_t iE=0;
      switch (flags)
      {
         case TW_MR_DATA_ACK: SZ(iE,1);
            recv();
            reply();
            break;

         case TW_START: SZ(iE,2);
         case TW_REP_START: SZ(iE,3);
            send(buff.hwAddr);
            nack();
            break;

         case TW_MT_SLA_ACK: SZ(iE,5);
         case TW_MT_DATA_ACK: SZ(iE,4);
            if (buff.iB < buff.f.nB)
            {
               send(buff.f.pB[buff.iB++]);
               nack();
            }
            else
            {
               stop();
               unlock();
            }
            break;

         case TW_MR_SLA_ACK: SZ(iE,6);
            reply();
            break;

         case TW_MT_SLA_NACK: SZ(iE,7);
         case TW_MR_SLA_NACK: SZ(iE,8);
         case TW_MT_DATA_NACK: SZ(iE,9);
         default:
            stop();
            unlock();
            break;

         case TW_MR_DATA_NACK: SZ(iE,11);
            recv();
            stop();
            unlock();
            break;
      }
      return(iE);
   } // event

}; // class TWMISR

