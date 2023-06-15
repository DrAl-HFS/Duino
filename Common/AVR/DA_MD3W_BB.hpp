// Duino/Common/AVR/DA_MD3WBB.hpp - Native AVR "toolkit" for Maxim-Dallas 3Wire bus interface bit-banging
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors May 2023

#ifndef DA_MD3W_BB_HPP
#define DA_MD3W_BB_HPP

//#ifdef ARDUINO_ARCH_AVR

// AVR specific version reduces code size (useful for TinyAVR)
// and allows fine tuning of bit-bang rate.

// Assume a single port is to be used for 3Wire
#define MD3W_DDR  DDRD
#define MD3W_PORT PORTD
#define MD3W_PIN  PIND
//#define MD3W_PU   PUD

#ifndef MD3W_BB_BIT_CE
#define MD3W_BB_BIT_CE 2 // PD2
#endif
#ifndef MD3W_BB_BIT_DIO
#define MD3W_BB_BIT_DIO 3 // PD3
#endif
#ifndef MD3W_BB_BIT_CLK
#define MD3W_BB_BIT_CLK 4 // PD4
#endif

class MD3WTiming
{
protected:
   MD3WTiming (void) { ; }

#if F_CPU == 16000000UL
   // AVR specific 5V device power (older 3Wire device 2MHz max) minimum delay.
   // Half a 2MHz clock cycle is 4 AVR 16MHz clock cycles.
   // Fast direct modification of a GPIO pin requires a 2 clk instruction, so in theory
   // an additional 2 clk delay will suffice, assuming actual CPU clock rate is within:
   void waitHalfT (void) { asm volatile ("rjmp .+0"); }  // Risky: 2 clk, 1 "nop" instructions (16MHz)
   //void waitHalfT (void) { asm volatile ("lpm"); }        // Reliable: 3 clk, 1  " (20MHz)
   //void waitHalfT (void) { asm volatile ("lpm \n nop"); } // Safest: 4 clk, 2  " (24MHz)
   void waitTwoT (void) { delayMicroseconds(1); }
#else // Assume 3.3V device power (3W 0.5MHz max)
   void waitHalfT (void) { delayMicroseconds(1); }
   void waitTwoT (void) { delayMicroseconds(4); }
#endif
}; // MD3WTiming

class MD3WSignals : MD3WTiming
{
protected:
   MD3WSignals (void) { ; }
   

   void releasePins (void)
   {
      MD3W_DDR&= ~(_BV(MD3W_BB_BIT_CE) | _BV(MD3W_BB_BIT_CLK) | _BV(MD3W_BB_BIT_DIO));
   } // releasePins

   void setDataRead (void) { MD3W_DDR&= ~_BV(MD3W_BB_BIT_DIO); }

   void initPins (void)
   {
      MD3W_PORT&= ~(_BV(MD3W_BB_BIT_CE) | _BV(MD3W_BB_BIT_CLK) | _BV(MD3W_BB_BIT_DIO));
      MD3W_DDR|= _BV(MD3W_BB_BIT_CE) | _BV(MD3W_BB_BIT_CLK) | _BV(MD3W_BB_BIT_DIO);
   } // initPins

   void start (void) { MD3W_PORT|= _BV(MD3W_BB_BIT_CE); waitTwoT(); }
   
   void end (void) { MD3W_PORT&= ~_BV(MD3W_BB_BIT_CE); waitTwoT(); }

   void clkH (void) { MD3W_PORT|= _BV(MD3W_BB_BIT_CLK); waitHalfT(); }

   void clkL (void) { MD3W_PORT&= ~_BV(MD3W_BB_BIT_CLK); waitHalfT(); }
   
   void clkStrobe (void) { clkH(); clkL(); }

   uint8_t readlsb (void) { return(0x00 != ( MD3W_PIN & _BV(MD3W_BB_BIT_DIO) ) ); }

   void writelsb (uint8_t b)
   {
      if (0x1 & b) { MD3W_PORT|= _BV(MD3W_BB_BIT_DIO); }
      else { MD3W_PORT&= ~_BV(MD3W_BB_BIT_DIO); }
      waitHalfT();
   } // writelsb
   
}; // class MD3WSignals

#endif // DA_MD3W_BB_HPP
