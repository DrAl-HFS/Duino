// Duino/Common/AVR/DA_RotEnc.hpp - Rotary Encoder +Button util
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef DA_ROT_ENC_HPP
#define DA_ROT_ENC_HPP

// EC11 Rotary Encoder : DFRobot module SEN0235
// Inputs on D2-D4 (Arduino pins #2,3,4)
// Quadrature (terminals labelled "A" & "B") -> D2,D3 respectively
// Switch (normally closed, terminal "C") -> D4
class CRotEnc
{
protected:
   uint8_t rawBS; // button, 8samples @ 1kHz -> 8ms debounce
   uint8_t rawQS;
public:
   uint8_t bCount; // lsb = state
   int8_t  qCount;
   
   CRotEnc (void) { ; }
  
   void init (void)
   {  // PortD: 2,3,4
      DDRD&= 0xE3;  // 0 for inputs
      PORTD&= 0xE3; // 0 for no pull-up
      //for (int8_t i=2; i<=4; i++) { pinMode(i, INPUT); }
      read();
   } // init
  
   uint8_t read (void)
   { 
      uint8_t t= rawQS & 0xC; // extract previous state
      //uint8_t r= 0; for (int8_t i=2; i<=4; i++) { r= (r << 1) | digitalRead(i); }
      uint8_t r= PIND;
      rawBS= (rawBS << 1) | (0 == (r & 0x10)); // PIND4); // invert NC
      r&= 0xC;
      if ((0x00 == rawBS) | (0xFF == rawBS))
      {  // steady state - debounced
         if ((bCount & 0x1) ^ (rawBS & 0x1)) { bCount= (rawBS & 0x1); r|= 0x1; }
         else if (bCount < 0xFD) { bCount+= 0x2; }
      }
      rawQS= swapHiLo4U8(t) | (r & 0xC); // combine new quadrature state (lo) with previous (hi)
      switch (rawQS)
      { // quadrature change
         case 0xC4 :
         case 0x40 :
         case 0x08 :
         case 0x8C :
            qCount--; r|= 0x2; break;
        
        case 0xC8 :
        case 0x4C :
        case 0x04 :
        case 0x80 :
           qCount++; r|= 0x2; break;
      }
      return(r&0x3); // return change mask
   }
   
}; // CRotEnc

#endif // #include <EEPROM.h>

