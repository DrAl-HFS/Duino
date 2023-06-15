// Duino/Common/CMD3W_BB.hpp - For DS13x2 and similar, Maxim-Dallas 3Wire bus interface, bit-banging
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors May 2023

#ifndef CMD3W_BB_HPP
#define CMD3W_BB_HPP

// Hacky reversion to preprocessor, consider <const> or <constexpr> within namespace ?

// MAXIM/DALLAS 3Wire bus Read / Not-Write command flag 
#define MD3W_BB_RNW 0x01

#ifdef ARDUINO_ARCH_AVR

// Use native AVR version (default pins PD2,3,4)
#include "AVR/DA_MD3W_BB.hpp"

#else
// Arduino portable version: easily flexible pin selection
// Arduino API pin numbers (these just happen to match PortD bit numbers)
#ifndef MD3W_BB_PIN_CE
#define MD3W_BB_PIN_CE 2
#endif
#ifndef MD3W_BB_PIN_DIO
#define MD3W_BB_PIN_DIO 3
#endif
#ifndef MD3W_BB_PIN_CLK
#define MD3W_BB_PIN_CLK 4
#endif

class MD3WTiming
{
protected:
   MD3WTiming (void) { ; }

   // Portable and low-power (3.3V means older 3Wire device is 0.5MHz max) compatibility.
   void waitHalfT (void) { delayMicroseconds(1); }
   void waitTwoT (void) { delayMicroseconds(4); }
}; // MD3WTiming

class MD3WSignals : MD3WTiming
{
   void setMode (const uint8_t m)
   {
      pinMode(MD3W_BB_PIN_CE, m);
      pinMode(MD3W_BB_PIN_CLK, m);
      pinMode(MD3W_BB_PIN_DIO, m);
   } // setMode

protected:
   MD3WSignals (void) { ; }
   
   void setDataRead (void) { pinMode(MD3W_BB_PIN_DIO, INPUT); }
   
   void initPins (void)
   {
      digitalWrite(MD3W_BB_PIN_CE, LOW);
      digitalWrite(MD3W_BB_PIN_CLK, LOW);
      digitalWrite(MD3W_BB_PIN_DIO, LOW);
      setMode(OUTPUT);
   } // initPins

   void releasePins (void) { setMode(INPUT); }
   
   void start (void) { digitalWrite(MD3W_BB_PIN_CE, HIGH); waitTwoT(); }
   
   void end (void) { digitalWrite(MD3W_BB_PIN_CE, LOW); waitTwoT(); }

   void clkH (void) { digitalWrite(MD3W_BB_PIN_CLK, HIGH); waitHalfT(); }

   void clkL (void) { digitalWrite(MD3W_BB_PIN_CLK, LOW); waitHalfT(); }
   
   void clkStrobe (void) { clkH(); clkL(); }
   
   uint8_t readlsb (void) { return digitalRead(MD3W_BB_PIN_DIO); }
   void writelsb (uint8_t b) { digitalWrite(MD3W_BB_PIN_DIO, 0x01 & b); waitHalfT(); }
}; // class MD3WSignals

#endif

class CMD3W_BB  : public MD3WSignals
{
public:
   CMD3W_BB (void) { ; }

   void begin () { releasePins(); }

   void end () { releasePins(); }

   void beginTransmission (uint8_t command)
   {
      initPins();
      start(); 
      write(command, (command & MD3W_BB_RNW) == MD3W_BB_RNW);
   }

   void endTransmission (void) { MD3WSignals::end(); }

   void write (uint8_t b, bool cmdRD= false)
   {
      for (int8_t i= 0; i < 7; i++) // bits 0..6
      {
         writelsb(b);
         clkStrobe(); // data read by device on rising edge
         b>>= 1;
      }
      writelsb(b); // bit 7
      clkH(); // If switching to read, enact change before clock falling edge
      if (cmdRD) { setDataRead(); }
      clkL();
   } // write

   uint8_t read (void)
   {  // use a mask to assemble bits so that AVR shifting workload is reduced
      uint8_t mrb= 0x2, b= readlsb(); // first bit already present on IO pin

      for (uint8_t i= 1; i < 8; i++)
      {
         clkStrobe(); // next bit
         if (readlsb()) { b|= mrb; }
         mrb<<= 1;
      }
      clkStrobe(); // possibly more to read?...
      return(b);
   } // read

}; // class CMD3W_BB

// Wrapper class for library compatibility
class CFacadeMD3W  : public CMD3W_BB
{
public:
   CFacadeMD3W (uint8_t ioPin=3, uint8_t clkPin=4, uint8_t cePin=2)  : CMD3W_BB() { ; }

private:
   void resetPins (void) { releasePins(); } // High impedance for power efficiency

}; // class CFacadeMD3W


#endif // CMD3W_BB_HPP
