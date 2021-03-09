// Duino/Common/AVR/DA_SPIMHW.hpp - Arduino-AVR SPI Master Hardware wrapper
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#ifndef DA_SPIMHW_HPP
#define DA_SPIMHW_HPP

#include <SPI.h>
//#include "Common/DA_FastPollTimer.hpp" // Debug/test


/***/

// Communication with MCP41010 seems unreliable, even at lower speed.
// Have to send a great many times to see a change, and behaviour is
// not predictable; quasi-random hysteresis.. ?
// Board design (routing) problems? Circuit design issue?

#ifdef AVR // 328P specific ?
// Pin number used for SPI select (active low)
// any logic 1|0 output will do...
// Just use SS pin (D10 on Uno) - must be an output to prevent
// SPI HW switching to slave mode (master + slave
// operation would require extra signalling/arbitration)
#define PIN_SEL1 SS // ADS9833 device select (inverted)  CPOL1 (falling) CPHA0 (leading)
#define PIN_SEL2 9  // MCP41010 device select (inverted) CPOL0 (rising) CPHA0 (leading)
// Hardware SPI pins used implicitly
//#define PIN_SCK SCK   // (D13 on Uno)
//#define PIN_DAT MISO  // (D12 on Uno)
//#define PIN_DAT MOSI  // (D11 on Uno)
#endif


//uint16_t rd16BE (const uint8_t b[2]) { return((256*(uint16_t)(b[0])) | b[1]); }

#if (SS == PIN_SEL1) // Directly toggle SS (PB2) for performance
#define SET_SEL1_LO() PORTB &= ~(1<<2) // should generate CBI / SBI 
#define SET_SEL1_HI() PORTB |= (1<<2)  // instructions as below
/* ...arduino.../hardware/tools/avr/avr/include/avr/iom328p.h
#define SET_SEL1_LO() { asm("CBI 0x25, 2"); } // IO address 0x20 + 0x5 ?
#define SET_SEL1_HI() { asm("SBI 0x25, 2"); }
TODO - properly comprehend gcc assembler arg handling...
#define ASM_CBI(port,bit) { asm("CBI %0, %1" : "=r" (port) : "0" (u)); }
#define SET_SEL1_LO() { asm("CBI %0, 2" : "=" PORTB ); }
#define SET_SEL1_HI() { asm("SBI %0, 2" : "=" PORTB ); }
*/
#else   // portable & robust (but slow) versions
#define SET_SEL1_LO() digitalWrite(PIN_SEL1, LOW)
#define SET_SEL1_HI() digitalWrite(PIN_SEL1, HIGH)
#endif
#if 0
#define SET_SEL2_LO() digitalWrite(PIN_SEL2, LOW)
#define SET_SEL2_HI() digitalWrite(PIN_SEL2, HIGH)
#else
#define SET_SEL2_LO() PORTB &= ~(1<<1)
#define SET_SEL2_HI() PORTB |= (1<<1)
#endif

class DA_SPIMHW // : public CFastPollTimer
{
protected:
   void beginTransS1 (void) { SPI.beginTransaction(SPISettings(8E6, MSBFIRST, SPI_MODE2)); }
   void beginTransS2 (void) { SPI.beginTransaction(SPISettings(8E6, MSBFIRST, SPI_MODE0)); }

   // AD9833 specific
   // Atomic (select hi->lo) write necesary to latch each register
   // 16bit write in Big Endian byte order
   void writeS1A16BE (const uint8_t b[2])
   {
      SET_SEL1_LO(); // Falling edge (low level enables input)
      //SPI.transfer16( rd16BE(b+i) ); // Total waste of time - splits into bytes internally
      SPI.transfer(b[1]);  // MSB (send in big endian byte order)
      SPI.transfer(b[0]);  // LSB
      SET_SEL1_HI(); // Rising edge latches to target register
   } // writeS1A16BE

   void writeS2 (const uint8_t cmd, const uint8_t b)
   {
      SET_SEL2_LO();    // active low
      SPI.transfer(cmd);  // command byte
      SPI.transfer(b);  // data byte
      SET_SEL2_HI();    // end
   } // writeS2
#if 0
   int8_t rwn (uint8_t r[], const uint8_t w[], const int8_t n)
   {
      int8_t i;
      SET_SEL1_LO(); // Falling edge enables input
      for (i=0; i<n; i++) { r[i]= SPI.transfer(w[i]); } // no individual latching
      SET_SEL1_HI(); // complete
      return(i);
   } // rwn
#endif

   void endTrans (void) { SPI.endTransaction(); }

public:
#ifdef DA_FAST_POLL_TIMER_HPP
   uint8_t dbgTransClk, dbgTransBytes;
#endif // DA_FAST_POLL_TIMER_HPP

   DA_SPIMHW ()
   {
      SPI.begin();
#if (PIN_SEL1 != SS) // SS is already an output
      pinMode(PIN_SEL1, OUTPUT);
#endif // PIN_SEL1
      digitalWrite(PIN_SEL1, HIGH);
      // second select for MCP41010 digital potentiometer
      pinMode(PIN_SEL2, OUTPUT);
      digitalWrite(PIN_SEL2, HIGH);
   }

#if 0
   int8_t modeReadWriteN (uint8_t r[], const uint8_t w[], const int8_t n, const uint8_t mode)
   {
      int8_t nr;
      beginTrans(mode);
      nr= rwn(r,w,n);
      endTrans();
      return(nr);
   }
#endif
}; // class DA_SPIMHW

#endif // DA_SPIMHW_HPP
