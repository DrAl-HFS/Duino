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
volatile uint8_t hwAddr;
volatile uint8_t *twi_data;
volatile uint8_t twi_bytes;
/* Bit definitions for the tw_status register */
#define BUSY 7

class TWIHW
{
protected:

   void start (void) { TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE); }

   void stop (void) { TWCR|= (1<<TWINT)|(1<<TWSTO); }

   void sync (void)
   {
      while(TWCR & (1<<TWSTO));
   } // sync

public:
   TWIHW (void) { ; }

}; // TWIHW

class TWISR : protected TWIHW
{
protected:
   volatile uint8_t status, retry_cnt;

   void start (void)
   {
      retry_cnt= 0;
      TWIHW::start();
      status |= (1<<BUSY);
   } // start

   void stop (void)
   {
      TWIHW::stop();
      status&= ~(1<<BUSY);
   } // stop

public:
   TWISR (void) : status{0}, retry_cnt{0} { ; }

   void init (void)
   {
#if defined(TWPS0)
TWSR= 0;
#endif
      TWBR= (FOSC / 100000UL - 16)/2; // 100kHz ?
   } // init

   bool sync (void)
   {
      while(status & (1<<BUSY)); // Busy wait (or exit with error code)
      return(true);
   } // sync

   int write (uint8_t address, uint8_t *data, uint8_t bytes)
   {
      if (sync())
      {
         TWIHW::sync();
         hwAddr= (address << 1) | TW_WRITE;
         twi_data= data;
         twi_bytes= bytes;
         start();
         return(bytes);
      }
      return 0;
   } // write

   int read (uint8_t address, uint8_t *data, uint8_t bytes)
   {
      if (sync())
      {
         hwAddr= (address << 1) | TW_READ;
         twi_data= data;
         twi_bytes= bytes;
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
            retry_cnt= 0;											// so clear retry counter
            TWDR= *twi_data;										// Transmit data,
            twi_data++;												// increment pointer
            TWCR |= (1<<TWINT);									// and clear TWINT to continue
            break;

         case TW_MT_SLA_NACK :										// Slave didn't acknowledge address,
         case TW_MR_SLA_NACK :
            retry_cnt++;											// this may mean that the slave is disconnected
            TWCR |= (1<<TWINT)|(1<<TWSTA)|(1<<TWSTO);			// retry 3 times
            break;

         case TW_MT_DATA_ACK :										// Slave Acknowledged data,
            if(--twi_bytes > 0)
            {	 // Send more data
               TWDR= *twi_data;									// Send it,
               twi_data++;											// increment pointer
               TWCR |= (1<<TWINT);								// and clear TWINT to continue
            }
            else { stop(); }
            break;

         case TW_MT_DATA_NACK :	stop(); break; // Slave didn't acknowledge data

         case TW_MT_ARB_LOST : break;				// Single master this can't be!!!

         case TW_MR_SLA_ACK : // Slave acknowledged address
            if(--twi_bytes > 0) { TWCR |= (1<<TWEA)|(1<<TWINT);	} // If there is more than one byte to read acknowledge
            else { TWCR |= (1<<TWINT); }							// else do not acknowledge
            break;

         case TW_MR_DATA_ACK : 										// Master acknowledged data
            *twi_data= TWDR;										// Read received data byte
            twi_data++;												// Increment pointer
            if(--twi_bytes > 0) { TWCR |= (1<<TWEA)|(1<<TWINT);	} // Get next databyte and acknowledge
            else { TWCR &= ~(1<<TWEA);	} // Enable Acknowledge must be cleared by software, this also clears TWINT!!!
            break;

         case TW_MR_DATA_NACK : 	// Master didn't acknowledge data -> end of read process
            *twi_data= TWDR; // Read received data byte
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









