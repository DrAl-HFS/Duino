// Duino/Common/AVR/DA_TWI.hpp - AVR two wire (I2C-like) basics, specific
// to KK variant.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#include <util/twi.h> // TW_MT_* TW_MR_* etc definitions

#ifndef CORE_CLK
#define CORE_CLK 16000000UL	// 16MHz
#endif

namespace TWI {

class HWS // hardware state
{
protected:
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
   HWS (void) { ; }

}; // class HWS

class SWS : protected HWS // software state sits atop hardware state
{
protected:
   enum State : uint8_t { BUSY=0x80 };

   volatile uint8_t status, retry_cnt;
   uint8_t hwAddr;

public:
   SWS (void) : status{0}, retry_cnt{0}, hwAddr{0} { ; }

   void start (void)
   {
      clear();
      status|= BUSY;
   } // start

   void commit (void) { TWI::HWS::commit(hwAddr); }

   void stop (void) { status&= ~BUSY; }

   bool sync (void) const { return(0 == (status & BUSY)); }

   bool retry (bool commit=true)
   {
      bool r= retry_cnt < 3;
      if (commit) { retry_cnt+= r; } // retry_cnt+= commit&&r; ? more efficient ?
      return(r);
   } // retry

   void clear (void) { retry_cnt= 0; }

}; // class SWS

class Buffer
{
protected:
   Frag f;

public:
   Buffer (void) { ; }

   uint8_t& byte (void) { return(*f.pB++); }

   bool more (void) // { f.nB-= (f.nB > 0); return(f.nB > 0); }
   { return((int8_t)(--f.nB) > 0); }

   void readByte (void) { byte()= TWDR; }
   void writeByte (void) { TWDR= byte(); }
}; // Buffer

}; // namespace TWI
