// Duino/Common/AVR/DA_RotEnc.hpp - handle EC11 Rotary Encoder with Button
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef DA_ROT_ENC_HPP
#define DA_ROT_ENC_HPP

// Scaling from system 1ms tick to 50Hz UI events
#define UI_EVENT_RATE_TICKS 20

// EC11 Rotary Encoder : DFRobot module SEN0235
// Inputs on D2-D4 (Arduino pins #2,3,4)
// Quadrature (terminals labelled "A" & "B") -> D2,D3 respectively
// Switch (normally closed, terminal "C") -> D4
class CRotEnc
{
protected:
   uint8_t rawBS; // button: press=1, release=0, 8samples @ 1kHz -> 8ms debounce
   uint8_t rawQS; // 4bit mask in lo nybble
   int8_t  rateDC; // UI event rate delay counter
   
   uint8_t updateBS (uint8_t bm)
   {  // "debounce" and count
      rawBS= (rawBS << 1) | bm; // &0x1
      if ((0x00 == rawBS) | (0xFF == rawBS))
      {  // steady state -> debounced
         if ((bCount & 0x1) ^ (rawBS & 0x1)) { bCount= (rawBS & 0x1); rateDC= UI_EVENT_RATE_TICKS; return(0x1); } // set new state & clear count
         else if (bCount < 0xFD)
         { // same state, increment count if not already saturated 
            if (--rateDC <= 0)
            {  // 20ms delay to produce 50Hz UI count rate
               rateDC= UI_EVENT_RATE_TICKS;
               bCount+= 0x2;
            }
         }
         //rawBS^= 0x1; // block update for debounce interval
      }
      return(0);
   } // updateBS
   
   uint8_t updateQS (uint8_t qm)
   {
      if (rawQS ^ qm)
      {  // signal change
         rawQS= qm | (rawQS >> 2);
         switch (rawQS)
         {  // check quadrature patterns - 
            case 0b0111 : // TODO: Consider ways of encoding
            case 0b1000 : // combinations of inversion
            case 0b1110 : // and reversal
            case 0b0001 :
               qCount++; return(0x2); //break;
           
            case 0b1011 :
            case 0b0100 :
            case 0b1101 :
            case 0b0010 :
               qCount--; return(0x2); //break;
         }
      }
      return(0);
   } // updateQS
   
public:
   uint8_t bCount; // bit 0 = state, saturating count in bits 1:7
   int8_t  qCount;
   
   CRotEnc (void) { ; }
  
   void init (void)
   {  // PortD: 2,3,4
      DDRD&= 0xE3;  // 0 for inputs
      PORTD&= 0xE3; // 0 for no pull-up
      //for (int8_t i=2; i<=4; i++) { pinMode(i, INPUT); }
      update();
      //uint8_t r= 0; for (int8_t i=2; i<=4; i++) { r= (r << 1) | digitalRead(i); }
   } // init

   uint8_t update (void)
   { 
      uint8_t r= PIND;  // invert NC button so 1 means pressed
      r= updateBS(0 == (r & 0x10)) | updateQS(r & 0xC);
      return(r); // return change mask
   } // update
   
}; // CRotEnc

class CRotEncDbg : public CRotEnc
{
public :
   uint8_t lbc;
   
   CRotEncDbg (void) { ; }
   
   uint8_t update (void)
   {
      uint8_t r= CRotEnc::update();
      if ((bCount & 0x1) && (bCount != lbc)) { lbc= bCount; r|= 0x4; }
      return(r);
   } // update
   
   void dump (uint16_t t, Stream& s) const
   {
      Serial.print("t:"); Serial.print(t); 
      Serial.print(" B:"); Serial.print(bCount); 
      Serial.print(" C:"); Serial.println(qCount); 
   } // dump
}; // CRotEncDbg

#endif // #include <EEPROM.h>

