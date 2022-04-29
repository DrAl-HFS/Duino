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
   SACK=0x10, RACK=0x20
};
enum CountId : uint8_t { NOUT, NIN, NVER, NUM };

//enum RcvMode : uint8_t { MRCV, MVER, MVRL, MRWVM };

#define RWVMASK 0x03
enum FragMode : uint8_t {
   WR=0x00, RD=0x01, // write, read,
   VA=0x02, VF=0x03, // verify all, verify until fail (early terminate) mask
   REV= 0x80
};

struct Frag { uint8_t *pB, nB; FragMode m; };

// Buffer management
#define BUFF_FRAG_MAX 2
class Buffer
{
protected:
   volatile uint8_t state, count[3]; // NB: more correctly an interface property (rather than buffer...)
   uint8_t hwAddr;
   uint8_t nF, iF, iB;
   Frag frag[BUFF_FRAG_MAX]; // Consider: factor out?

   FragMode getMode (uint8_t m=RWVMASK) { return(m & frag[iF].m); }

   bool lock (void)
   {
      bool r= sync();
      if (r) { state= LOCK; }
      return(r);
   } // lock

   void unlock (void) { state&= ~LOCK; }

   void setAddr (const uint8_t devAddr) { hwAddr= devAddr << 1; }

   void resetFrags (void) { iF=0; iB=0; }

   void clearAll (void)
   {
      nF=0;
      resetFrags();
      for (int8_t i=0; i < CountId::NUM; i++) { count[i]= 0; }
   } // clearAll

   void addFrag (uint8_t b[], const uint8_t n, const FragMode m)
   {
      if (nF >= BUFF_FRAG_MAX) { return; }
      frag[nF].pB= b;
      frag[nF].nB= n;
      frag[nF].m=  m;
      nF++;
   } // addFrag

   bool nextFragValid (uint8_t i) const
   {
      return(((i+1) < nF) && (frag[i].m == frag[i+1].m) && (frag[i+1].nB > 0));
   }

   uint8_t& next (void)
   {
      uint8_t i= iB;
      if (iB < frag[iF].nB) { iB++; } // next byte
      else if (nextFragValid(iF)) { i= 0; iB= 1; iF++; } // next frag
      else { i= frag[iF].nB - 1; } // else error - repeat last (should never happen...)
      if (frag[iF].m & REV) { i= frag[iF].nB - (1+i); }
      return(frag[iF].pB[i]);
   } // next

   uint8_t outgoing (void)
   {
      count[NOUT]++;
      return next();
   } // outgoing

   bool incoming (const uint8_t b)
   {
      count[NIN]++;
      switch(getMode())
      {
         case FragMode::RD : next()= b; return(true);
         case FragMode::VA : count[NVER]+= (b == next()); return(true);
         case FragMode::VF : if (b == next()) { count[NVER]++; return(true); } break;
         //case FragMode::WR : return(false);
      }
      return(false);
   } // incoming
   //void verify (void) { nV+= (Buffer::next() == HWRC::recv()); Buffer::state|= RECV; }

public:
   Buffer (void) : state{0} { ; }

   uint8_t getHWAddr (void) const { return(hwAddr| getMode(FragMode::RD)); }

   bool sync (void) const { return(LOCK != (LOCK & state)); }

   uint8_t remaining (bool lazy=true) const
   {
      uint8_t r= frag[iF].nB - iB;
      if (lazy && (r > 1)) { return(r); }
      // else sum (beware single byte frags)
      uint8_t i= iF;
      while (nextFragValid(i++))
      {
         r+= frag[i].nB;
         if (lazy && (r > 1)) { return(r); }
      }
      return(r);
   } // remaining

   bool more (void) const { remaining() > 0; }

   bool notLast (void) const
   {
#if 1
      return(remaining() > 1);  // { 4 3 4 6 7 }* ???
#else
      return(iB < (frag[iF].nB-1)); // { 4 3 4 6 {1}* 7 }
#endif
   }

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

class ISR : public HWRC, public Buffer
{
   void reply (bool accept=true) { if (accept && Buffer::notLast()) { HWRC::ack(); } else { HWRC::nack(); } }

   void send (void) { HWRC::send( Buffer::outgoing() ); }


public:
   // ISR (void) { ; } compile error on this ???

#define SZ(i,v)  if (0 == i) { i= v; }

   int8_t event (const uint8_t flags)
   {
      int8_t iE=0;
      switch (flags)
      {
         case TW_MR_DATA_ACK: SZ(iE,1);
            reply(Buffer::incoming(HWRC::recv()));
            break;

         case TW_MT_DATA_ACK: SZ(iE,2); state|= SACK;
         case TW_MT_SLA_ACK: SZ(iE,3); if (3 == iE) { state|= AACK; }
            if (more())
            {
               send();
               HWRC::nack();
            }
            else
            {
               HWRC::stop();
               Buffer::unlock();
            }
            break;

         case TW_START: SZ(iE,4);
         case TW_REP_START: SZ(iE,5);
            Buffer::state|= START; // sent OK
            HWRC::send(Buffer::getHWAddr());
            HWRC::nack();
            break;

         case TW_MR_SLA_ACK: SZ(iE,6);
            Buffer::state|= AACK;
            reply();
            break;

         case TW_MR_DATA_NACK: SZ(iE,7);
            Buffer::incoming(HWRC::recv()); // fall through stop(); unlock(); break;
         case TW_MT_SLA_NACK: SZ(iE,8);
         case TW_MR_SLA_NACK: SZ(iE,9);
         case TW_MT_DATA_NACK: SZ(iE,10);
         default: SZ(iE,11);
            HWRC::stop();
            Buffer::unlock();
            break;
      }
      return(iE);
   } // event

}; // class ISR

// Basic functionality...
class RW1 : public ISR
{
public:
   //RW1 (void) { ; }

   int transfer (const uint8_t devAddr, uint8_t b[], const uint8_t n, const FragMode m)
   {
      if (lock())
      {
         clearAll();
         setAddr(devAddr);
         addFrag(b, n, m);
         start();
         return(n);
      }
      return(0);
    } // transfer

   int readFrom (const uint8_t devAddr, uint8_t b[], const uint8_t n)
      { return transfer(devAddr,b,n,RD); }

   int writeTo (uint8_t devAddr, const uint8_t b[], const uint8_t n)
      { return transfer(devAddr,b,n,WR); }

   int readFromRev (const uint8_t devAddr, uint8_t b[], const uint8_t n)
      { return transfer(devAddr,b,n,REV|RD); }

   int writeToRev (uint8_t devAddr, const uint8_t b[], const uint8_t n)
      { return transfer(devAddr,b,n,REV|WR); }

}; // class RW1

}; // namespace TWM
