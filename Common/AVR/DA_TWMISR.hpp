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
static struct
{
   volatile uint8_t state;
   uint8_t n, i;
   uint8_t hwAddr;
   uint8_t b[TWI_BUFFER_LENGTH];
} buff;

class TWMISR
{
   void start (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); }

   void stop (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); }

   void ack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); }

   void nack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE); }

   void send (uint8_t b) { TWDR= b; }

   void recv (void) { buff.b[buff.i++] = TWDR; }

protected:
   void reply (void)
   {
      if (buff.i < (buff.n - 1)) { ack(); }
      else { nack(); }
   }

   void unlock (void) { buff.state= 0; }

   bool lock (void)
   {
      bool r= sync();
      if (r) { buff.state= 0x1; }
      return(r);
   }

   // transitional hack
   int get (uint8_t b[], int r)
   {
      if ((r > 0) && (b != buff.b+0)) { memcpy(b, buff.b+0, r); }
      return(r);
   } // get

public:
   TWMISR (void) { ; }

   bool sync (void) const { return(0 == buff.state); }

   int write (uint8_t devAddr, const uint8_t b[], const uint8_t n)
   {
     if (lock())
     {
        buff.hwAddr= (devAddr << 1) | TW_WRITE;
        buff.n= n;
        buff.i= 0;
        memcpy(buff.b+buff.i, b, n);

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
        buff.n= n;
        buff.i= 0;

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
            if (buff.i < buff.n)
            {
               send(buff.b[buff.i++]);
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

