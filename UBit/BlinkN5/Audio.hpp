// Duino/UBit/BlinkN5/Audio.hpp - Micro:Bit V2 sound IO testing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors June 2021

#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "Common/N5/N5_Util.hpp"


#define RES_BITS 12
#define RES_AMP_MAX ((1<<10)-1)
#define RES_AMP_HALF (RES_AMP_MAX/2)

#ifdef TARGET_UBITV2

#define SPKR_PIN 27

#ifndef MIC_LED_PIN
#define MIC_LED_PIN 28
#endif

// Hardware PWM, spported on nRF52+
class CHardPWMN5 // not working as expected...
{
public:
   CHardPWMN5 (void) { ; }
   
   // https://tech.microbit.org/hardware/schematic/#v2-pinmap
   void setup (uint8_t i=0, NRF_PWM_Type *pPWM=NRF_PWM0)
   {
      if (i >= 3) return;
      pPWM->PSEL.OUT[i]= 0; // HW designation P0.00 - routed to speaker
      pPWM->MODE= 0; // straight not updown (mirror)
      pPWM->PRESCALER= 0; // DIV_1;
      pPWM->COUNTERTOP= RES_AMP_MAX; // 16MHz/4096 = 3906.25Hz
      pPWM->LOOP= 4;
      pPWM->DECODER= 0;
      pPWM->SEQ[i].REFRESH= 0;
      pPWM->SEQ[i].ENDDELAY= 0;
      //pPWM->SEQ[i].PTR= (uint32_t)w;
      //pPWM->SEQ[i].CNT= 0x10;
      pPWM->ENABLE= 1;
      //pPWM->TASKS_SEQSTART[i]= 1;
   } // setup
   
   void play (uint16_t s[], int16_t n, uint8_t i=0, NRF_PWM_Type *pPWM=NRF_PWM0)
   {
      if (i >= 3) return;
      pPWM->SEQ[i].PTR= (uint32_t)s;
      pPWM->SEQ[i].CNT= n;
      pPWM->TASKS_SEQSTART[i]= 1;
   } // play
   
}; // CHardPWMN5

#endif // TARGET_UBITV2

class CSampler
{
protected:
   uint16_t r, qs[0x10]; // quarter sine lookup

public:
   CSampler (void)
   {
      r= 1<<8;
      for (int i= 0; i < 0x10; i++)
      {
         float t= (M_PI_2 * i) / 0xF;
         qs[i]= RES_AMP_HALF * sinf(t);
      }
   }

   uint16_t sample (uint16_t i)
   {  //ts= (t * r) >> 8)
      uint16_t s= i & 0xF;
      if (i & 0x10) { s= 0xF-s; } // h-mirror
      s= qs[s];
      if (i & 0x20) { s= RES_AMP_HALF - s; } else { s+= RES_AMP_HALF; } // v-mirror
      return(s);
   } // sample
   
}; // CSampler

class CSpeaker : public CHardPWMN5, CSampler
{
protected:
   uint32_t nextTick;
   int16_t d;
   uint16_t w[0x40];
   
public:
   CSpeaker (void) { for (int i= 0; i < 0x10; i++) { w[i]= sample(i); } }
   
   void init (uint8_t pin=SPKR_PIN) { pinMode(pin, OUTPUT); analogWriteResolution(RES_BITS); }
   
   void click (uint8_t pin=SPKR_PIN) { if (0 == d) { analogWrite(pin, RES_AMP_MAX); } }
   
   void pulse (uint16_t ms=32) { d=ms; }

   void tone (void)
   {
      if (d >= 0)
      {
         CHardPWMN5::setup();
         d= -1;   // no stop ?
      }
      CHardPWMN5::play(w,0x40);
   } // tone
   
   void update (uint32_t t, uint8_t pin=SPKR_PIN)
   { 
      if ((d >= 0) && (t >= nextTick))
      {  // analogFrequency abandoned ?
         if (d > 0) { analogWrite(pin, sample(t)); d--; }
         else { analogWrite(pin, 0); }
         nextTick= t+1;
      }
   } // update
   
   void clear (void) { d= 0; }
}; // CSpeaker

#endif // AUDIO_HPP
