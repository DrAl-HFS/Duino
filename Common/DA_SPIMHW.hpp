// Duino/Common/DA_SPIMHW.hpp - Arduino-AVR SPI Master Hardware wrapper
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020

// HW Ref:  http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf

#ifndef DA_SPI_M_HW_HPP
#define DA_SPI_M_HW_HPP

#include <SPI.h>
//#include "Common/DA_FastPollTimer.hpp" // Debug/test


/***/

#ifdef AVR // 328P specific ?
// Pin number used for SPI select (active low)
// any logic 1|0 output will do...
// Just use SS pin - must be an output to prevent
// SPI HW switching to slave mode (master + slave
// operation requires some extra signalling/arbitration)
#define PIN_SEL SS // (#10 on Uno)
// 9
// Hardware SPI pins used implicitly
//#define PIN_SCK SCK   // (#13 on Uno)
//#define PIN_DAT MOSI  // (#11 on Uno)
#endif


//uint16_t rd16BE (const uint8_t b[2]) { return((256*(uint16_t)(b[0])) | b[1]); }

#if (SS == PIN_SEL) // Directly toggle SS (PB2) for performance
#define SET_SEL_LO() PORTB &= ~(1<<2) // should generate CBI / SBI 
#define SET_SEL_HI() PORTB |= (1<<2)  // instructions as below
/* ...arduino.../hardware/tools/avr/avr/include/avr/iom328p.h
#define SET_SEL_LO() { asm("CBI 0x25, 2"); } // IO address 0x20 + 0x5 ?
#define SET_SEL_HI() { asm("SBI 0x25, 2"); }
TODO - properly comprehend gcc assembler arg handling...
#define ASM_CBI(port,bit) { asm("CBI %0, %1" : "=r" (port) : "0" (u)); }
#define SET_SEL_LO() { asm("CBI %0, 2" : "=" PORTB ); }
#define SET_SEL_HI() { asm("SBI %0, 2" : "=" PORTB ); }
*/
#else   // portable & robust (but slow) versions
#define SET_SEL_LO() digitalWrite(PIN_SEL, LOW)
#define SET_SEL_HI() digitalWrite(PIN_SEL, HIGH)
#endif

class DA_SPIMHW // : public CFastPollTimer
{
protected:
   void beginTrans (SPISettings s=SPISettings(8E6, MSBFIRST, SPI_MODE2)) { SPI.beginTransaction(s); }
   
   // Atomic (select hi->lo) 16bit write Big Endian
   void writeA16BE (const uint8_t b[2])
   {
      SET_SEL_LO(); // Falling edge (low level enables input)
      //SPI.transfer16( rd16BE(b+i) ); // Total waste of time - splits into bytes internally
      SPI.transfer(b[1]);  // MSB (send in big endian byte order)
      SPI.transfer(b[0]);  // LSB
      SET_SEL_HI(); // Rising edge latches to target register
   } // write16
   
   int8_t rwn (uint8_t r[], const uint8_t w[], const int8_t n)
   {
      int8_t i;
      SET_SEL_LO(); // Falling edge enables input (no individual latching)
      for (i=0; i<n; i++) { r[i]= SPI.transfer(w[i]); }
      SET_SEL_HI(); // complete
      return(i);
   } // rwn
   
   void endTrans (void) { SPI.endTransaction(); }

public:
#ifdef DA_FAST_POLL_TIMER_HPP
   uint8_t dbgTransClk, dbgTransBytes;
#endif // DA_FAST_POLL_TIMER_HPP

   DA_SPIMHW ()
   {
      SPI.begin();
#if (PIN_SEL != SS) // SS is already an output
      pinMode(PIN_SEL, OUTPUT);
#endif // PIN_SEL
      digitalWrite(PIN_SEL, HIGH);
   }
   
   int8_t readWriteN (uint8_t r[], const uint8_t w[], const int8_t n)
   {
      int8_t nr;
      beginTrans(SPISettings(8E6, MSBFIRST, SPI_MODE1));
      nr= rwn(r,w,n);
      endTrans();
      return(nr);
   }
}; // class DA_SPIMHW

#endif // DA_SPI_M_HW_HPP
