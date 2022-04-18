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
#define TWI_BUFFER_LENGTH 40
#endif
static volatile uint8_t busy=0;
static struct {
   uint8_t buffer[TWI_BUFFER_LENGTH];
   uint8_t length;
   uint8_t index;
} transmission;
/*
void init() {
   TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
  TWSR = 0; // prescaler = 1

  busy = 0;

  sei();

  TWCR = _BV(TWEN);
}

uint8_t *wait() {
  while (busy);
  return(transmission.buffer+1);
}
*/
class TWMISR
{
   void start (void) { TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); }

   void stop (void) { TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); }

   void ack (void) { TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); }

   void nack (void) { TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE); }

   void send (uint8_t data) { TWDR = data; }

   void recv (void) { transmission.buffer[transmission.index++] = TWDR; }

   void reply (void)
   {
      if (transmission.index < (transmission.length - 1)) { ack(); } 
      else { nack(); }
   }

   void done (void) { busy= 0; }
   
protected:
   int get (uint8_t b[], int r)
   {
      if ((r > 0) && (b != transmission.buffer+1)) { memcpy(b, transmission.buffer+1, r); }
      return(r);
   } // get

public:
   TWMISR (void) { ; }

   bool sync (void) const { return(0 == busy); }

   int write (uint8_t devAddr, const uint8_t b[], const uint8_t n)
   {
     if (sync())
     {
        busy= 1;
        transmission.buffer[0]= (devAddr << 1) | TW_WRITE;
        transmission.length= n + 1;
        transmission.index= 0;
        memcpy(&transmission.buffer[1], b, n);

        start();
        return(n);
      }
      return(0);
   } // write

   int read (uint8_t devAddr, uint8_t b[], const uint8_t n)
   {
     if (sync())
     {
        busy= 1;
        transmission.buffer[0]= (devAddr << 1) | TW_READ;
        transmission.length= n + 1;
        transmission.index= 0;

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
         case TW_MT_SLA_ACK: SZ(iE,5);
         case TW_MT_DATA_ACK: SZ(iE,4);
            if (transmission.index < transmission.length)
            {
               send(transmission.buffer[transmission.index++]);
               nack();
            }
            else 
            {
               stop();
               done();
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
            done();
            break;

         case TW_MR_DATA_NACK: SZ(iE,11);
            recv();
            stop();
            done();
            break;
      }
      return(iE);
   } // event

}; // class TWMISR

