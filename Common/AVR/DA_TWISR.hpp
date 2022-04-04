// Duino/Common/AVR/DA_TWISR.hpp - AVR two wire (I2C-like) interrupt driven communication, C++ class.
// Adapted from C-code authored by "kkempeneers" (as "twi.c" - without copyright or licence declaration)
// obtained from:-
// https://www.avrfreaks.net/sites/default/files/project_files/Interrupt_driven_Two_Wire_Interface.zip
// This approach provides efficient asynchronous communication without buffer copying.
// This could break in a variety of interesting ways under Arduino, subject to resource requrements.
// * CAVEAT EMPTOR *
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#include <util/twi.h> // TW_MT_* TW_MR_* etc definitions

#ifndef CORE_CLK
#define CORE_CLK 16000000UL	// 16MHz
#endif

// SRAM access 2 cycles on 8b bus - so memcpy() is 4clks/byte ie. 4MB/s @ 16MHz
struct Frag { uint8_t *p, n, f; };

Frag fragB[2];
#define BUSY 7

#define TW_CLK_100   0x48
#define TW_CLK_400   0x12

class TWIHWS
{
protected:
   // NB: Interrupt flag (TWINT) is cleared by writing 1 
   void start (void) { TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE); }

   void restart (void) { TWCR|= (1<<TWINT)|(1<<TWSTA)|(1<<TWSTO); }

   void commit (uint8_t hwAddr)
   {
      TWDR= hwAddr; 			// Transmit SLA + Read or Write
      TWCR &= ~(1<<TWSTA);	// TWSTA must be cleared by software! This also clears TWINT!!!
   } // commit

   void stop (void) { TWCR|= (1<<TWINT)|(1<<TWSTO); }

   void ack (void) { TWCR|= (1<<TWINT)|(1<<TWEA); }
   
   void end (void) { TWCR&= ~(1<<TWEA); }
   
   void sync (void) { while(TWCR & (1<<TWSTO)); } // wait for bus stop condition to clear

   void resume (void) { TWCR|= (1<<TWINT); }
   
public:
   TWIHWS (void) { ; }

}; // TWIHWS

class TWISWS
{
protected:
   volatile uint8_t status, retry_cnt;

public:
   TWISWS (void) : status{0}, retry_cnt{0} { ; }

   void start (void)
   {
      clear();
      status |= (1<<BUSY);
   } // start

   void stop (void) { status&= ~(1<<BUSY); }

   bool sync (void)
   {
      while(status & (1<<BUSY)); // Busy wait (or exit with error code)
      return(true);
   } // sync

   bool retry (bool commit=true)
   { 
      bool r= retry_cnt < 3;
      if (commit) { retry_cnt+= r; } // retry_cnt+= commit&&r; ? more efficient ?
      return(r);
   } // retry

   void clear (void) { retry_cnt= 0; }

}; // TWISWS

class TWBuffer
{
protected:
   Frag f;

public:
   TWBuffer (void) { ; }
   
   uint8_t& byte (void) { return(*f.p++); }
}; // TWBuffer

class TWISR : protected TWIHWS, TWISWS, TWBuffer
{
protected:
   uint8_t evH[11];
   uint8_t hwAddr;

   void start (void)
   {
      TWISWS::start();
      TWIHWS::start();
   } // start

   void stop (void)
   {
      TWIHWS::stop();
      TWISWS::stop();
   } // stop

public:
   TWISR (void) { for (int8_t i=0; i<11; i++) { evH[i]= 0; } }

   using TWISWS::sync;
   
   void set (uint8_t r=TW_CLK_400)
   {
      //s.print("TWISR::init() - "); 
      //s.print("TWBR,TWSR="); s.print(TWBR,HEX); s.print(','); s.print(TWSR,HEX);
      //TWSR&= ~0x3; // unnecessary, status bits not writable anyway
      TWSR= 0; // clear PS1&0 so clock prescale= 1 (high speed)
      TWBR= r;
      // TWBR= ((CORE_CLK / busClk) - 16) / 2;
      //s.print(" -> "); s.print(TWBR,HEX); s.print(','); s.println(TWSR,HEX);
   } // init

   int write (uint8_t devAddr, uint8_t b[], uint8_t n)
   {
      if (sync())
      {
         TWIHWS::sync();
         hwAddr= (devAddr << 1) | TW_WRITE;
         f.p= b;
         f.n= n;
         start();
         return(n);
      }
      return 0;
   } // write

   int read (uint8_t devAddr, uint8_t b[], uint8_t n)
   {
      if (sync())
      {
         hwAddr= (devAddr << 1) | TW_READ;
         f.p= b;
         f.n= n;
         start();
         return(n);
      }
      return 0;
   } // read

   void event (const uint8_t flags)
   {
      int8_t iE=0;
      switch(flags)
      {
         case TW_MR_DATA_ACK : iE=1;  // Master has acknowledged receipt
            TWBuffer::byte()= TWDR;  // So store and acknowledge
            if (--f.n > 0) { ack();	} // Get next databyte and acknowledge
            else { end();	}
            break;

         case TW_START : iE= 2;
         case TW_REP_START : if (0 == iE) { iE= 3; }
            if (retry())  { commit(hwAddr); }
            else { stop(); } // multiple NACKs -> abort
            break;

         case TW_MT_DATA_ACK : iE= 4; // From slave device
            if (--f.n > 0)
            {	 // Send more data
               TWDR= TWBuffer::byte(); // Send it,
               resume();
            }
            else { stop(); } // Assume end of data
            break;

         case TW_MT_SLA_ACK :	 iE= 5; // From slave device (address recognised)
            clear(); // Transaction proceeds
            TWDR= TWBuffer::byte();
            resume();
            break;

         case TW_MR_SLA_ACK : iE= 6;  // Slave acknowledged address
            if (--f.n > 0) { ack();	} // If there is more than one byte to read acknowledge
            else { resume(); }		  // else do not acknowledge
            break;

         case TW_MT_SLA_NACK : iE= 7;  // No address ack, disconnected / jammed?
         case TW_MR_SLA_NACK : if (0 == iE) { iE= 8; }
            //retry();    // Assume retry avail
            restart();  // Stop-start should clear any jamming of bus
            break;

         case TW_MT_DATA_NACK :	iE= 9; stop(); break; // Slave didn't acknowledge data

         // Multi-master
         case TW_MT_ARB_LOST : iE= 10; break;				// Single master this can't be!!!

         case TW_MR_DATA_NACK : 	iE= 11; // Master didn't acknowledge data -> end of read process
            TWBuffer::byte()= TWDR; // Read received data byte
            stop();
            break;
      } // switch
      evH[iE]+= (evH[iE] < 0xFF);   // saturating increment
   } // event

   void dump (Stream& s) const
   { 
      for (int8_t i=0; i<11; i++) { s.print(evH[i]); s.print(' '); }
      s.println();
   } // dump
   
}; // class TWISR

// Instance and link to ISR

TWISR gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}









