// Duino/UBit/BlinkN5/Buttons.hpp - Micro:Bit button util
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef BUTTONS_HPP
#define BUTTONS_HPP

#include "Common/N5/N5_Util.hpp"

// Generic button class with basic state & event tracking
class CButton
{
private:
   uint8_t  bse;  // button state and event mask
   uint8_t  ts;   // ticks (update intervals) in current state
   
public:
   CButton (void) { ; }
   bool getChange (const bool state) { return((bse & 0x1) ^ state); }
   bool getState (void) { return(bse & 0x1); }
   bool getPress (void) { return(bse & 0x80); }
   bool getRelease (void) { return(bse & 0x40); }

   void update (const bool state)
   {
      const bool change= getChange(state);
      bse= 0x3F & ((bse << 1) | state);
      if (change)
      {  
         bse|= 1 << (6 + state); // 0x80 press, 0x40 release
         ts= 0;
      } else { ts+= (ts < 0xFF); } // hold count (saturating)
   } // update

   uint8_t getTime (const bool state)
   {  // NB yields zero for newly pressed
      if (getState() == state) { return(ts); }
      return(0);
   } // getTime
   
}; // CButton

// Arduinish
#define PIN_BTN_A 5
#define PIN_BTN_B 11

class CUBitButtons
{
public:
   CButton a, b;
   
   CUBitButtons (void) {;}
   
   void init (void)
   {
      pinMode(PIN_BTN_A, INPUT);
      pinMode(PIN_BTN_B, INPUT);
   }
   void update (void)
   {  // Caveat Emptor : just single time point samples taken 
      // periodically: vulnerable to switch bounce. Software
      // debouncing needs interrupt trigger plus settling time
      // or search for stability/settling across many closely
      // spaced reads...
      a.update(digitalRead(PIN_BTN_A));
      b.update(digitalRead(PIN_BTN_B));
   }
   uint8_t getTimeAB (const bool state)
   {
      return min(a.getTime(state), b.getTime(state));
   }

}; // CUBitButtons

#endif // BUTTONS_HPP
