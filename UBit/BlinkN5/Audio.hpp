// Duino/UBit/BlinkN5/Audio.hpp - Micro:Bit V2 sound IO
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors June 2021

#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "Common/N5/N5_Util.hpp"

#ifdef TARGET_UBITV2

#define SPKR_PIN 27

#ifndef MIC_LED_PIN
#define MIC_LED_PIN 28
#endif

#endif

class CSpeaker
{
protected:
   uint32_t nextTick;
   uint16_t d, r, w[0x10];
   
public:
   CSpeaker (void)
   {
      r= 1<<8;
      for (int i= 0; i < 0x10; i++)
      {
         float t= (M_PI_2 * i) / 0xF;
         w[i]= 0x100 * sinf(t);
      }
   }
   
   uint16_t sample (uint16_t i)
   {
      uint16_t s= i & 0xF;
      if (i & 0x10) { s= 0xF-s; } // h-mirror
      s= w[s];
      if (i & 0x20) { s= 0x100 - s; } else { s+= 0x100; } // v-mirror
      return(s);
   } // sample
   
   void init (uint8_t pin=SPKR_PIN) { pinMode(pin, OUTPUT); analogWriteResolution(12); }
   
   void click (uint8_t pin=SPKR_PIN) { if (d <= 0) { analogWrite(pin, 0x0FFF); } }
   
   void pulse (uint16_t ms=32) { d=ms; }

   // https://tech.microbit.org/hardware/schematic/#v2-pinmap
   void tone (NRF_PWM_Type *pPWM=NRF_PWM0) // no effect ...
   {
      pPWM->PSEL.OUT[0]= 0; // HW designation P0.00
      pPWM->MODE= 1; // updown (mirror)
      pPWM->PRESCALER= 0; // DIV_1;
      pPWM->COUNTERTOP= 800; // 20kHz
      pPWM->LOOP= 1;
      pPWM->DECODER= 0;
      pPWM->SEQ[0].REFRESH= 0;
      pPWM->SEQ[0].ENDDELAY= 0;
      pPWM->SEQ[0].PTR= (uint32_t)w;
      pPWM->SEQ[0].CNT= 0x10;
      pPWM->ENABLE= 1;
   } // tone
   
   void update (uint32_t t, uint8_t pin=SPKR_PIN)
   { 
      if (t >= nextTick)
      {  // analogFrequency abandoned ?
         //ts= (t * r) >> 8)
         if (d > 0) { analogWrite(pin, sample(t)); d--; }
         else { analogWrite(pin, 0); }
         nextTick= t+1;
      }
   } // update
   
}; // CSpeaker

#endif // AUDIO_HPP
