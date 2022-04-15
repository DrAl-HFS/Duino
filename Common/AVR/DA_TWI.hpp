// Duino/Common/AVR/DA_TWI.hpp - AVR two wire (I2C-like) basics.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#include <util/twi.h> // TW_MT_* TW_MR_* etc definitions

#ifndef CORE_CLK
#define CORE_CLK 16000000UL	// 16MHz
#endif

namespace TWI {

enum Clk : uint8_t   // Magic numbers for wire clock rate (when prescaler=1)
{
   CLK_100=0x48, CLK_150=0x2D, // Some approximate ~ +300Hz
   CLK_200=0x20, CLK_250=0x18, 
   CLK_300=0x12, CLK_350=0x0E, 
   CLK_400=0x0C   // Higher rates presumed unreliable.
};
enum State : uint8_t { BUSY=0x80 };
   
class HWS // hardware state
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
   HWS (void) { ; }

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
   
}; // class HWS


class SWS : protected HWS // software state sits atop hardware state
{
protected:
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

}; // class SWS

}; // namespace TWI
