// Duino/Common/AVR/DA_TWISR.hpp - AVR two wire (I2C-like) master mode
// interrupt driven communication, encapsulated as C++ class.
// Adapted from C-code authored by Scott Nelson (as "twi.c" - without
// copyright or licence declaration) obtained from:-
// https://github.com/scttnlsn/avr-twi
// ---
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Apr-May 2022

#ifndef DA_TWMISR_HPP
#define DA_TWMISR_HPP

#include <util/twi.h>

namespace TWM { // Two Wire Master

enum ClkTok : uint8_t   // Tokens -> magic numbers for wire clock rate (for prescaler=1 and CORE_CLK=16MHz)
{
   CLK_100=0x48, CLK_150=0x2D, // Some approximate ~ +300Hz
   CLK_200=0x20, CLK_250=0x18,
   CLK_300=0x12, CLK_350=0x0E,
   CLK_400=0x0C,   // Higher rates presumed unreliable.
   CLK_INVALID=0x00 // -> 1MHz but not usable
};

enum StateFlag : uint8_t {
   LOCK=0x01, START=0x02,
   ADDR=0x04, AACK=0x08,
   SACK=0x10, RACK=0x20
};
enum CountId : uint8_t { NOUT, NIN, NVER, NUM };

enum FragMode : uint8_t {
   READ=0x01, VERA=0x03, VERF=0x05, // read, verify all, verify until fail (early terminate) mask (all contain TW_READ flag)
   WRITE=0x02, // write (compatible with TW_WRITEITE flag)
   RWVM=0x07, // read/write/verify mask
   REV= 0x08, // reverse byte order
   REPB= 0x10,  // repeated single byte
   RESTART= 0x80
};

struct Frag { uint8_t *pB, nB; FragMode m; };

// Buffer management
#define BUFF_FRAG_MAX 2
class Buffer // NB: includes interface properties
{
protected:
   volatile uint8_t state, count[3];//, dR;
   uint8_t hwAddr;
   uint8_t nF, iF, iB;
   Frag frag[BUFF_FRAG_MAX]; // Consider: factor out?

   FragMode getMode (uint8_t m=RWVM) { return(m & frag[iF].m); }

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

   bool nextFragValid (uint8_t i) const { return(((i+1) < nF) && (frag[i+1].nB > 0)); }
   uint8_t nextFrag (void)
   {
      if (nextFragValid(iF))
      {
         return(1 + (frag[++iF].m & RESTART));
      }
      //else
      return(0);
   }

   bool continuation (uint8_t i) const { return( nextFragValid(i) && ((READ & frag[i].m) == (READ & frag[i+1].m)) ); }

   uint8_t& next (void)
   {
      uint8_t i= iB;
      if (iB < frag[iF].nB) { iB++; } // next byte
      else if (continuation(iF)) { i= 0; iB= 1; iF++; } // next frag
      else { i= frag[iF].nB - 1; } // else error - repeat last (should never happen...)
      if (frag[iF].m & REPB) { i= 0; } // single byte repeated
      else if (frag[iF].m & REV) { i= frag[iF].nB - (1+i); } // reverse order -> invert index
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
         case FragMode::READ : next()= b; return(true);
         case FragMode::VERA : count[NVER]+= (b == next()); return(true);
         case FragMode::VERF : if (b == next()) { count[NVER]++; return(true); } break;
         //case FragMode::WRITE : return(false);
      }
      return(false);
   } // incoming

public:
   Buffer (void) : state{0} { ; }

   uint8_t getHWAddr (void) const { return(hwAddr| getMode(FragMode::READ)); }

   bool sync (void) const { return(LOCK != (LOCK & state)); }

   uint8_t remaining (bool lazy=true) const
   {
      uint8_t r= frag[iF].nB - iB;
      if (lazy && (r > 1)) { return(r); }
      // else sum (beware single byte frags)
      uint8_t i= iF;
      while (continuation(i++))
      {
         r+= frag[i].nB;
         if (lazy && (r > 1)) { return(r); }
      }
      return(r);
   } // remaining

   //uint8_t dremaining (bool lazy=true) { dR= remaining(lazy); return(dR); }

   bool more (void) const { return( remaining() > 0 ); }

   bool notLast (void) const { return( remaining() > 1 ); }

   friend class Sync; // allow protected access - ineffective (?)

}; // class Buffer

// Hardware register commands
class HWRC
{
protected:

   void start (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); }

   void restart (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO) | _BV(TWSTA); }

   void stop (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWSTO); }

   void ack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA); }

   void nack (void) { TWCR= _BV(TWINT) | _BV(TWEN) | _BV(TWIE); }

   void send (uint8_t b) { TWDR= b; }

   uint8_t recv (void) { return(TWDR); }

   void setClkT (ClkTok t)
   {
      //TWSR&= ~0x3; // unnecessary, status bits not writable anyway
      TWSR= 0x00; // clear PS1&0 so clock prescale= 1 (high speed)
      TWBR= t;
   } // set

   ClkTok getClkT (void) { return(TWSR); }

   void setClkPS (uint8_t s) { TWSR= s; } //& 0x3);

   uint8_t getClkPS (void) { return(TWSR & 0x3); }

}; // class HWRC

//class CCommonTWAS; // forward decl.

class ISR : public HWRC, public Buffer
{
private:
   void reply (bool accept=true) { if (accept && Buffer::notLast()) { HWRC::ack(); } else { HWRC::nack(); } }

   void send (void) { HWRC::send( Buffer::outgoing() ); }

protected:

   bool beginTransfer (const uint8_t devAddr)
   {
      if (lock())
      {
         clearAll();
         setAddr(devAddr);
         return(true);
      }
      return(false);
   } // beginTransfer

public:
   //ISR (void) { HWRC::setClk(CLK_100); }

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
            else switch(nextFrag())
            {
               case 2 : HWRC::restart(); break;
               case 1 : HWRC::start(); break; // repeated start or restart ?
               default :
                  HWRC::stop();
                  Buffer::unlock();
                  break;
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
         case TW_MR_SLA_NACK: SZ(iE,8);
         case TW_MT_SLA_NACK: SZ(iE,9);
         case TW_MT_DATA_NACK: SZ(iE,10);
         default: SZ(iE,11);
            HWRC::stop();
            Buffer::unlock();
            break;
      }
      return(iE);
   } // event

}; // class ISR

class TransferAS : public ISR
{
public:
   //TransferAS (ClkTok t=CLK_100) { setClkT(t); }

   using HWRC::setClkT;
   using HWRC::getClkT;
   using HWRC::setClkPS;
   using HWRC::getClkPS;

   // Wire compatibility hack
   void begin (uint8_t dummyHWAddr=0x00) { setClkT(CLK_100); }

   int transfer1AS (const uint8_t devAddr, uint8_t b[], const uint8_t n, const FragMode m)
   {
      if (beginTransfer(devAddr))
      {
         addFrag(b, n, m);
         start();
         return(n);
      }
      return(0);
    } // transfer1AS

   // long winded but seemingly necessary until friendship issues resolved...
   int transfer2AS
   (
      const uint8_t devAddr,
      uint8_t b1[], const uint8_t n1, const FragMode m1,
      uint8_t b2[], const uint8_t n2, const FragMode m2
   )
   {
      if (beginTransfer(devAddr))
      {
         addFrag(b1, n1, m1);
         addFrag(b2, n2, m2);
         start();
         return(n1+n2);
      }
      return(0);
    } // transfer2AS

}; // class TransferAS

// Debug extension
class Debug : public TransferAS
{
public:
   uint8_t evQ[64];
   uint8_t stQ[8];
   int8_t iEQ, iSQ;

   TWDebug (void) { clrEv(); }

   void clrEv (void) { iEQ= 0; iSQ=0; }

   int8_t event (const uint8_t flags)
   {
      const int8_t iE= ISR::event(flags);
      if (iSQ < sizeof(stQ)) { stQ[iSQ++]= Buffer::state; }
      if (iEQ < sizeof(evQ)) { evQ[iEQ++]= iE; }
      return(iE);
   } // event

   void logEv (Stream& s, const uint8_t sf=0xFE) const
   {
      //s.print("dR="); s.println(dR);
      s.print("SQ:");
      for (int8_t i=0; i<iSQ; i++) { s.print(" 0x"); s.print(stQ[i] & sf, HEX); }
      s.print("\nEQ:");
      for (int8_t i=0; i<iEQ; i++) { s.print(' '); s.print(evQ[i]); }
      s.print("\nCounts:");
      for (int8_t i=0; i < TWM::NUM; i++) { s.print(' '); s.print(count[i]); }
      s.println();
   } // log

}; // class Debug

}; // namespace TWM

// Interface instance and link to ISR

TWM::Debug gTWM;

SIGNAL(TWI_vect)
{
   gTWM.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
} // SIGNAL(TWI_vect)

// Interface name wrapper (for 'Duino / Wire compatibility)
#ifndef I2C
#define I2C gTWM
#endif

// Now basic functionality provided by accessability layer, 
// stateless hence easily inheritable by device drivers 
class CCommonTWAS
{
public:
   CCommonTWAS (void) { ; }

   int readFromAS (const uint8_t devAddr, uint8_t b[], const int n)
   {
      if (n > 0) { return I2C.transfer1AS(devAddr, b, n, TWM::READ); }
      else return(0);
   } // readFromAS

   int writeToAS (const uint8_t devAddr, const uint8_t b[], const int n)
   {
      if (n > 0) { return I2C.transfer1AS(devAddr, b, n, TWM::WRITE); }
      else return(0);
   } // writeToAS

   int readFromRevAS (const uint8_t devAddr, uint8_t b[], const int n)
   {
      if (n > 0) { return I2C.transfer1AS(devAddr, b, n, TWM::REV|TWM::READ); }
      else return(0);
   } // readFromRevAS

   int writeToRevAS (const uint8_t devAddr, const uint8_t b[], const int n)
   {
      if (n > 0) { return I2C.transfer1AS(devAddr, b, n, TWM::REV|TWM::WRITE); }
      else return(0);
   } // writeToRevAS

   // Test hack
   int writeToRevThenFwdAS (uint8_t devAddr, const uint8_t bRev[], const int nRev, const uint8_t bFwd[], const int nFwd)
   {
      if ((nRev > 0) && (nFwd > 0))
      {
         return I2C.transfer2AS(devAddr, bRev, nRev, TWM::REV|TWM::WRITE, bFwd, nFwd, TWM::WRITE);
      }
      else return(0);
   } // writeToRevThenFwdAS

}; // class CCommonTWAS

#endif // DA_TWMISR_HPP
