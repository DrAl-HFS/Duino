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

#ifndef FOSC
#define FOSC 16000000UL	// 16MHz
#endif

// SRAM access 2 cycles on 8b bus - so memcpy() is 4clks/byte ie. 4MB/s
struct Frag { uint8_t *p, n; };

Frag fragB[2];
//volatile uint8_t *twi_data;
//volatile uint8_t twi_bytes;
/* Bit definitions for the tw_status register */
#define BUSY 7

class TWIHWS
{
protected:

   void start (void) { TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE); }

   void restart (void) { TWCR |= (1<<TWINT)|(1<<TWSTA)|(1<<TWSTO); }

   void stop (void) { TWCR|= (1<<TWINT)|(1<<TWSTO); }

   void ack (void) { TWCR |= (1<<TWEA)|(1<<TWINT); }
   
   void sync (void)
   {
      while(TWCR & (1<<TWSTO));
   } // sync

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

   void retry (void) { retry_cnt++; }

   void clear (void) { retry_cnt= 0; }

}; // TWISWS

class TWISR : protected TWIHWS, TWISWS
{
protected:
   uint8_t hwAddr;
   Frag f;

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
   TWISR (void) { ; }

   using TWISWS::sync;
   
   void init (Stream& s)
   {
#if defined(TWPS0)
TWSR= 0;
#endif
      //s.print("TWISR::init() - "); 
      //s.print("TWBR,TWSR="); s.print(TWBR,HEX); s.print(','); s.print(TWSR,HEX);
      TWBR= 0x12; // -> 400kHz
      // (FOSC / 100000UL - 16)/2; // 0x48 -> 100kHz ?
      //s.print(" -> "); s.print(TWBR,HEX); s.print(','); s.println(TWSR,HEX);
   } // init

   int write (uint8_t devAddr, uint8_t *data, uint8_t bytes)
   {
      if (sync())
      {
         TWIHWS::sync();
         hwAddr= (devAddr << 1) | TW_WRITE;
         f.p= data;
         f.n= bytes;
         start();
         return(bytes);
      }
      return 0;
   } // write

   int read (uint8_t devAddr, uint8_t *data, uint8_t bytes)
   {
      if (sync())
      {
         hwAddr= (devAddr << 1) | TW_READ;
         f.p= data;
         f.n= bytes;
         start();
         return(bytes);
      }
      return 0;
   } // read

   void event (const uint8_t flags)
   {
      switch(flags)
      {
         case TW_START :												// Start condition
         case TW_REP_START :											// Repeated start condition
            if(retry_cnt > 2)
            {	// 3 NACKs -> abort
                stop();
                return;
            }
            TWDR= hwAddr; 				// Transmit SLA + Read or Write
            TWCR &= ~(1<<TWSTA);	// TWSTA must be cleared by software! This also clears TWINT!!!
            break;

         case TW_MT_SLA_ACK :											// Slave acknowledged address,
            clear();											// so clear retry counter
            TWDR= *f.p;										// Transmit data,
            f.p++;												// increment pointer
            TWCR |= (1<<TWINT);									// and clear TWINT to continue
            break;

         case TW_MT_SLA_NACK :										// Slave didn't acknowledge address,
         case TW_MR_SLA_NACK :
            retry();											// this may mean that the slave is disconnected
            restart();			// retry 3 times
            break;

         case TW_MT_DATA_ACK :										// Slave Acknowledged data,
            if(--f.n > 0)
            {	 // Send more data
               TWDR= *f.p;									// Send it,
               f.p++;											// increment pointer
               TWCR |= (1<<TWINT);								// and clear TWINT to continue
            }
            else { stop(); }
            break;

         case TW_MT_DATA_NACK :	stop(); break; // Slave didn't acknowledge data

         case TW_MT_ARB_LOST : break;				// Single master this can't be!!!

         case TW_MR_SLA_ACK : // Slave acknowledged address
            if(--f.n > 0) { ack();	} // If there is more than one byte to read acknowledge
            else { TWCR |= (1<<TWINT); }							// else do not acknowledge
            break;

         case TW_MR_DATA_ACK : 										// Master acknowledged data
            *f.p= TWDR;										// Read received data byte
            f.p++;												// Increment pointer
            if(--f.n > 0) { ack();	} // Get next databyte and acknowledge
            else { TWCR &= ~(1<<TWEA);	} // Enable Acknowledge must be cleared by software, this also clears TWINT!!!
            break;

         case TW_MR_DATA_NACK : 	// Master didn't acknowledge data -> end of read process
            *f.p= TWDR; // Read received data byte
            stop();
            break;
      } // switch
   } // event

}; // class TWISR

// Instance and link to ISR

TWISR gTWI;

SIGNAL(TWI_vect)
{
   gTWI.event( TWSR & TW_STATUS_MASK ); // Mask out prescaler bits to get TWI status
}









