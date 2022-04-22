// Duino/Common/AVR/DA_TWISR.hpp - AVR two wire (I2C-like) master mode
// interrupt driven communication, encapsulated as C++ class.
// Adapted from C-code authored by Scott Nelson (as "twi.c" - without
// copyright or licence declaration) obtained from:-
// https://github.com/scttnlsn/avr-twi
// ---
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Apr 2022

#include <util/twi.h>

#ifndef TWI_BUFFER_LENGTH
#define TWI_BUFFER_LENGTH 48
#endif
#ifdef TWI_BUFFER_LENGTH
static uint8_t gTWTB[TWI_BUFFER_LENGTH]; // test buffer
#endif

namespace TWM { // Two Wire Master

enum StateFlag : uint8_t {
   LOCK=0x01, START=0x02,
   ADDR=0x04, AACK=0x08,
   SEND=0x10, SACK=0x20,
   RECV=0x40, RACK=0x80
};

// Buffer management
#define BUFF_FRAG_MAX 2
class Buffer
{
protected:
   volatile uint8_t state; // NB: more correctly an interface property (rather than buffer...)
   uint8_t hwAddr;
   uint8_t nF, iF, iB;
   Frag frag[BUFF_FRAG_MAX];

   bool lock (void)
   {
      bool r= sync();
      if (r) { state= LOCK; }
      return(r);
   } // lock

   void unlock (void) { state&= ~LOCK; }

   void setAddr (const uint8_t devAddr, const uint8_t rwf) { hwAddr= (devAddr << 1) | (rwf & 0x1); }

   void resetFrags (void) { iF=0; iB=0; }

   void clearFrags (void) { nF=0; resetFrags(); }

   void addFrag (uint8_t b[], const uint8_t n, const uint8_t f=0)
   {
      if (nF >= BUFF_FRAG_MAX) { return; }
      frag[nF].pB= b;
      frag[nF].nB= n;
      frag[nF].f=  f;
      nF++;
   } // addFrag

   bool nextFragValid (uint8_t i) const
   {
      return((i < (nF-1)) && (frag[i].f == frag[i+1].f) && (frag[i+1].nB > 0));
   }

   uint8_t& next (void)
   {
      if (iB < frag[iF].nB) { return(frag[iF].pB[iB++]); } // most likely
      //else
      if (nextFragValid(iF)) // advance to next frag
      {
         iB= 1;
         return(frag[++iF].pB[0]);
      }
      //else error
      return(frag[iF].pB[frag[iF].nB-1]); // just repeat last
   } // next

public:
   Buffer (void) : state{0} { ; }

   bool sync (void) const { return(LOCK != (LOCK & state)); }

   int remaining (void) const // not yet working... ???
   {
      int r= (int)frag[iF].nB - (int)iB;
      if (r > 1) { return(r); }
      // else sum (beware single byte frags)
      uint8_t i= iF; //
      while (nextFragValid(i++))
      {
         r+= frag[i].nB;
         if (r > 1) { return(r); }
      }
      return(r);
   } // remaining

   bool more (void) const { return(iB < frag[iF].nB); } // remaining() > 0; } // ???

   bool notLast (void) const { return(iB < (frag[iF].nB-1)); } // remaining() > 1; } // 

}; // class Buffer

// Hardware register commands
class HWRC
{
protected:

   void start (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); }

   void stop (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); }

   void ack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); }

   void nack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE); }

   void send (uint8_t b) { TWDR= b; }

   uint8_t recv (void) { return(TWDR); }

}; // class HWRC

class ISR : HWRC, public Buffer
{
   void reply (void) { if (Buffer::notLast()) { HWRC::ack(); } else { HWRC::nack(); } }

   void send (void) { HWRC::send( Buffer::next() ); state|= SEND; }

   void recv (void) { Buffer::next()= HWRC::recv(); state|= RECV; }

public:
   // ISR (void) { ; } compile error on this ???

   int writeTo (uint8_t devAddr, const uint8_t b[], const uint8_t n)
   {
      if (lock())
      {
         setAddr(devAddr, TW_WRITE);
         clearFrags();
         addFrag(b, n);
         start();
         return(n);
      }
      return(0);
   } // writeTo

   int readFrom (uint8_t devAddr, uint8_t b[], const uint8_t n)
   {
      if (lock())
      {
         setAddr(devAddr, TW_READ);
         clearFrags();
         addFrag(b, n);
         start();
         return(n);
      }
      return(0);
   } // readFrom

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

         case TW_MT_DATA_ACK: SZ(iE,2); state|= SACK;
         case TW_MT_SLA_ACK: SZ(iE,3); if (3 == iE) { state|= AACK; }
            if (more())
            {
               send();
               nack();
            }
            else
            {
               stop();
               unlock();
            }
            break;

         case TW_START: SZ(iE,4);
         case TW_REP_START: SZ(iE,5);
            state|= START; // sent OK
            HWRC::send(hwAddr);
            nack();
            break;

         case TW_MR_SLA_ACK: SZ(iE,6);
            state|= AACK;
            reply();
            break;

         case TW_MR_DATA_NACK: SZ(iE,7);
            recv(); // fall through stop(); unlock(); break;
         case TW_MT_SLA_NACK: SZ(iE,8);
         case TW_MR_SLA_NACK: SZ(iE,9);
         case TW_MT_DATA_NACK: SZ(iE,10);
         default: SZ(iE,11);
            stop();
            unlock();
            break;
      }
      return(iE);
   } // event

}; // class ISR

}; // namespace TWM
