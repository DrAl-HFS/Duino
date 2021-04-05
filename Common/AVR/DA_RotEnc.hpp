// Duino/Common/AVR/DA_RotEnc.hpp - handle EC11 Rotary Encoder with Button
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb - Mar 2021

#ifndef DA_ROT_ENC_HPP
#define DA_ROT_ENC_HPP

// Scaling from system 1ms tick to 50Hz UI events
#define UI_EVENT_RATE_TICKS 20

// TODO : factor hardware handling out from higher level features (HAL model)

// EC11 Rotary Encoder base class provides processing only
// hardware input moved to derived class for better reusability
class CRotEncBase
{
protected:
   uint8_t rawBS;  // button: press=1, release=0, 8samples @ 1kHz -> 8ms debounce (seems like plenty)
   uint8_t rawQS;  // 4bit data in lo nybble: new state in bits 3,2, previous in bits 1,0
   int8_t  rateDC; // UI event rate delay counter
   
   uint8_t updateBS (uint8_t bm)
   {  // "debounce" and count
      rawBS= (rawBS << 1) | bm; // &0x1
      if ((0x00 == rawBS) | (0xFF == rawBS))
      {  // steady state -> debounced
         if ((bCount & 0x1) ^ (rawBS & 0x1))
         {  // set new state & clear count 
            bCount= (rawBS & 0x1);
            rateDC= UI_EVENT_RATE_TICKS;
            return(0x1); // signal press/release event
         }
         else // same state, increment count if not already saturated 
            if ((bCount < 0xFD) && (--rateDC <= 0))
            {  // 20ms delay to produce 50Hz UI count rate
               rateDC= UI_EVENT_RATE_TICKS;
               bCount+= 0x2;
            }
      }
      return(0);
   } // updateBS
   
   uint8_t updateQS (uint8_t qm)
   {  // No debouncing of rotation, but invalid states ignored (may not be sufficient for an old noisy encoder)
      if (0x0C & (rawQS ^ qm))
      {  // state change detected
         rawQS= qm | (rawQS >> 2); // form current + previous mask
         switch (rawQS)
         {  // check the 8 valid quadrature patterns - 
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
   uint8_t qCount; // quadrature count, unsigned wrap 255<->0 (signed wrap +127 <-> -128)
   
   CRotEncBase (uint8_t c) { qCount=c; } // : qCount{c} start midscale
  
   bool buttonState (void) { return(bCount & 0x1); }
   bool buttonTime (void) { return(bCount >> 1); }
}; // CRotEncBase

// DFRobot module SEN0235
// Appears to have 3 resistors + 1 capacitor (HW debounced switch?)
// Inputs on D2-D4 (Arduino pins #2,3,4)
// Quadrature (terminals labelled "A" & "B") -> D3,D2
// Switch (normally closed, terminal "C") -> D4
class CRotEncDFR : public CRotEncBase
{
public:
   CRotEncDFR (uint8_t c=0x80) : CRotEncBase(c) { ; }
   
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
      r= CRotEncBase::updateBS(0 == (r & 0x10)) | CRotEncBase::updateQS(r & 0x0C);
      return(r); // return change mask
   } // update
}; // CRotEncDFR

class CRotEncDbg : public CRotEncDFR
{
public :
   uint8_t lbc;
   
   CRotEncDbg (void) { ; }
   
   uint8_t update (void) // test button timing feature
   {
      uint8_t r= CRotEncDFR::update();
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

// Bourns EC11 "raw" with minimal conditioning circuitry
class CRotEncBEC11 : public CRotEncBase
{
public:
   CRotEncBEC11 (uint8_t c=0x80) : CRotEncBase(c) { ; }
   
   void init (void)
   {  // PortD: 2,3,4
      DDRD&= 0xE3;  // 0 for inputs
      PORTD&= 0xE3; // 0 for no pull-up. quadrature pins pull-up |= 0x0C;
      //for (int8_t i=2; i<=4; i++) { pinMode(i, INPUT); }
      update();
      //uint8_t r= 0; for (int8_t i=2; i<=4; i++) { r= (r << 1) | digitalRead(i); }
   } // init

   uint8_t update (void)
   { 
      uint8_t r= PIND;
      r= CRotEncBase::updateBS(r & 0x10) | CRotEncBase::updateQS(r & 0x0C);
      return(r); // return change mask
   } // update
}; // CRotEncBEC11

#endif // DA_ROT_ENC_HPP

