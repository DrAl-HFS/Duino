// Duino/Common/AVR/DA_RotEnc.hpp - handle AVR IO for EC11 Rotary Encoder with Button
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Feb - April 2021

#ifndef DA_ROT_ENC_HPP
#define DA_ROT_ENC_HPP

#include "../RotEnc.hpp" // Generic classes level up, ugly, fix?

// DFRobot module SEN0235
// Appears to have 3 resistors + 1 capacitor (switch HW debounced)
// Inputs on D2-D4 (Arduino pins #2,3,4)
// Quadrature (terminals labelled "A" & "B") -> D3,D2
// Switch (normally closed, terminal "C") -> D4
class CRotEncDFR : public CRotEncUIDbg
{
public:
   CRotEncDFR (uint8_t c=0x80) : CRotEncUIDbg(c) { ; }
   
   void init (void)
   {  // PortD: 2,3,4
      DDRD&= 0xE3;  // 0 for inputs
      PORTD&= 0xE3; // 0 for no pull-up
      //for (int8_t i=2; i<=4; i++) { pinMode(i, INPUT); }
      update();
      //uint8_t r= 0; for (int8_t i=2; i<=4; i++) { r= (r << 1) | digitalRead(i); }
   } // init

   uint8_t update (void) // apply any changes, return event mask
   {  
      uint8_t r= PIND;  // invert NC button so 1 means pressed
      return(CRotEncUI::updateBS(0 == (r & 0x10)) | CRotEncUI::updateQS(r));
   } // update

}; // CRotEncDFR


//#ifndef MBD_DEF_H
//typedef union { uint8_t u8[2]; uint16_t u16[1]; } UU16;
//#endif

// Bourns EC11 "raw" with minimal conditioning circuitry (load resistor ~1k on Gnd return path)
class CRotEncDualBEC11 
{
public:
   CRotEncUIDbg e0,e1;
   uint8_t r;
   
   CRotEncDualBEC11 (uint8_t c=0x80) : e0{c},e1{c} { ; }
   
   void init (void)
   {
#ifdef ARDUINO_AVR_MEGA2560 // choose port .?. PC2 #35 .. PC7 #30
      DDRC&= 0x03;  // 0 for inputs
      PORTC|= 0xFC; // pull-up for raw (switched from floating to Gnd) inputs.
      //SFIOR|&= ~PUD; ???
#else // PortD: 2-7
      DDRD&= 0x03;  // 0 for inputs
      PORTD|= 0xFC; // pull-up for raw (switched from floating to Gnd) inputs.
#endif
      update();
   } // init

   uint16_t update (void)
   { 
#ifdef ARDUINO_AVR_MEGA2560   
      r= PINC;
#else
      const uint8_t r= PIND; 
#endif
      UU16 m;  // invert buttons (closure grounds pull-up)
      m.u8[0]= e0.updateBS(0 == (r & 0x10)) | e0.updateQS(r);
      m.u8[1]= e1.updateBS(0 == (r & 0x80)) | e1.updateQS(r>>3);
      return(r); //m.u16[0]); // merge change masks
   } // update
   
    void dump (Stream& s) const { s.print('r'); s.print(r,HEX); s.print(" E0:"); e0.dump(s); s.print("E1:"); e1.dump(s); }
}; // CRotEncDualBEC11

#endif // DA_ROT_ENC_HPP

