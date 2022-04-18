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
static volatile uint8_t busy;
static struct {
   uint8_t buffer[TWI_BUFFER_LENGTH];
   uint8_t length;
   uint8_t index;
   void (*callback)(uint8_t, uint8_t *);
} transmission;
/*
void init() {
   TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
  TWSR = 0; // prescaler = 1

  busy = 0;

  sei();

  TWCR = _BV(TWEN);
}
*/
uint8_t *wait() {
  while (busy);
  return(transmission.buffer+1);
}

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

   void done (void)
   {
      busy= 0;
      if (NULL != transmission.callback) { transmission.callback(transmission.buffer[0] >> 1, transmission.buffer+1); }
   }

public:
   TWMISR (void) { ; }

void write (uint8_t address, uint8_t* data, uint8_t length, void (*callback)(uint8_t, uint8_t *))
{
  wait();

  busy = 1;

  transmission.buffer[0] = (address << 1) | TW_WRITE;
  transmission.length = length + 1;
  transmission.index = 0;
  transmission.callback = callback;
  memcpy(&transmission.buffer[1], data, length);

  start();
}

void read (uint8_t address, uint8_t length, void (*callback)(uint8_t, uint8_t *))
{
  wait();

  busy = 1;

  transmission.buffer[0] = (address << 1) | TW_READ;
  transmission.length = length + 1;
  transmission.index = 0;
  transmission.callback = callback;

  start();
}

   int8_t event (const uint8_t flags)
   {
      switch (flags)
      {
         case TW_START:
         case TW_REP_START:
         case TW_MT_SLA_ACK:
         case TW_MT_DATA_ACK:
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

         case TW_MR_DATA_ACK:
            recv();
            reply();
            break;

         case TW_MR_SLA_ACK:
            reply();
            break;

         case TW_MR_DATA_NACK:
            recv();
            stop();
            done();
            break;

         case TW_MT_SLA_NACK:
         case TW_MR_SLA_NACK:
         case TW_MT_DATA_NACK:
         default:
            stop();
            done();
            break;
      }
   } // event

}; // class TWMISR

/*
// Instance and link to ISR

TWIDebug gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}

*/
