// Duino/Common/RotEnc.hpp - handle EC11 Rotary Encoder with Button
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Feb - April 2021

#ifndef ROT_ENC_HPP
#define ROT_ENC_HPP

// Scaling from system 1ms tick to 50Hz UI events
#define UI_EVENT_RATE_TICKS 20

#define ROTENC_DEBOUNCE_MASK 0xFF // all 8bits

#define ROTENC_EVENT_NONE    0x00
#define ROTENC_BTN_CHANGE    0x01 // unvalidated change
#define ROTENC_BTN_RELEASE   0x02 //
#define ROTENC_BTN_PRESS     0x03 //
#define ROTENC_BTN_COUNT     0x04 // hold count increased
#define ROTENC_BTN_SAT       0x08 // hold count saturated ~2.5sec
#define ROTENC_BTN_X0        0x0C // unused extension
#define ROTENC_ROT_CHANGE    0x10 // unvalidated change
#define ROTENC_ROT_EVENT_CCW 0x20
#define ROTENC_ROT_EVENT_CW  0x30

// EC11 Rotary Encoder base class provides polled event processing.
// A poll rate of 1kHz is sufficient to accurately capture any manual input.
// Hardware interaction is deferred to derived classes for better reusability
// across configurations.
class CRotEncBase
{
protected:
   uint8_t rawState[2]; // [0]: 8b button press=1, release=0, 8samples @ 1kHz -> 8ms debounce (seems like plenty)
                         // [1]: filtered button state in top bit,
                         //      4b rotation data in lo nybble (new in bits 3,2, prev. in bits 1,0)

   uint8_t updateBS (bool bm)
   {  // "debounce" and count    bm&= 0x1;
      rawState[0]= (rawState[0] << 1) | bm;
      uint8_t m= rawState[0] & ROTENC_DEBOUNCE_MASK;
      if ((m > 0x00) && (m < ROTENC_DEBOUNCE_MASK)) { return(ROTENC_BTN_CHANGE); }
      //else check for state change
      if ((0 != (rawState[1] & 0x80)) ^ bm)
      {  // set new state & signal event 
         rawState[1]^= 0x80;
         return(ROTENC_BTN_RELEASE+bm); // signal press/release event
      }
      return(0);
   } // updateBS
   
   uint8_t updateQS (uint8_t qm)
   {  // No debouncing of rotation, but invalid states ignored (may not be sufficient for an old noisy encoder)
      qm&= 0x0C; // bits 3,2 only are valid
      if (qm ^ (rawState[1] & 0x0C)) // lazy change detect
      {
         qm |= (rawState[1] & 0x0C) >> 2; // form quadrature transition mask
         rawState[1]= (rawState[1] & 0xF0) | qm; // store for next time
         switch (qm)
         {  // check the 8 valid quadrature patterns - 
            case 0b0111 : // TODO: Consider ways of encoding
            case 0b1000 : // combinations of inversion
            case 0b1110 : // and reversal
            case 0b0001 :
               return(ROTENC_ROT_EVENT_CW); //break;
           
            case 0b1011 :
            case 0b0100 :
            case 0b1101 :
            case 0b0010 :
               return(ROTENC_ROT_EVENT_CCW); //break;
         }
         return(ROTENC_ROT_CHANGE);
      }
      return(0);
   } // updateQS
   
public:
   CRotEncBase (void) { ; }
   
   bool buttonState (void) { return(rawState[1] & 0x80); }
}; // CRotEncBase

class CRotEncUI : public CRotEncBase
{
public:
   uint8_t bCount;  // saturating button state count
   int8_t  bRateDC; // UI button event rate delay counter
   uint8_t qCount;  // quadrature count, unsigned wrap 255<->0 (signed wrap +127 <-> -128)

   CRotEncUI (uint8_t c) : CRotEncBase() { qCount=c; }

   uint8_t updateBS (bool bm)
   {
      uint8_t m= CRotEncBase::updateBS(bm);
      if (m >= ROTENC_BTN_RELEASE)
      {  // start new count
         bCount= 0;
         bRateDC= UI_EVENT_RATE_TICKS;
      } // same state, check count
      else if (0xFF == bCount) { m|= ROTENC_BTN_SAT; } 
      else if (--bRateDC <= 0)
      {  // 20ms delay to produce 50Hz UI count rate
         m|= ROTENC_BTN_COUNT;
         bRateDC= UI_EVENT_RATE_TICKS;
         bCount++;
      }
      return(m);
   } // updateBS
   
   uint8_t updateQS (uint8_t qm)
   {
      uint8_t m= CRotEncBase::updateQS(qm);
      switch(m)
      {
         case ROTENC_ROT_EVENT_CW : ++qCount; break;
         case ROTENC_ROT_EVENT_CCW : --qCount; break;
      }
      return(m);
   } // updateQS

   uint8_t buttonTime (void) { return(bCount); }
}; // CRotEncUI

class CRotEncUIDbg : public CRotEncUI
{
public:
   CRotEncUIDbg (uint8_t c) : CRotEncUI(c) { ; }
   
   void dump (uint16_t t, Stream& s) const { s.print("t:"); s.print(t); dump(s); }
   void dump (Stream& s) const
   {
      s.print(" B:"); s.print(buttonState()); s.print(","); s.print(buttonTime()); 
      s.print(" R:"); s.println(qCount); 
   } // dump
}; // CRotEncUIDbg

#endif // ROT_ENC_HPP

