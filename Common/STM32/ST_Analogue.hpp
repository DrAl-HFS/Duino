// Duino/Common/STM32/ST_Analogue.hpp - alternative wrapper for RCM/STM32Duino (libMaple) ADC utils
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#ifndef ST_ANALOGUE_HPP
#define ST_ANALOGUE_HPP

// Sampling periods use ADclk = APB2clk / 2~8
// APB2clck is max 100MHz, so could match core clock.
// F401: 84MHz core So ADclk 42 ~ 10.5MHz => 1.5M ~ 21.875k samples/s
#ifdef ARDUINO_ARCH_STM32F4
#include <STM32F4ADC.h>
#define ADC_NORM_SCALE  (1.0 / BIT_MASK(12))
#define ADC_VBAT_SCALE  4.0  // Vbat ADC input uses 1:3 divider
#define ADC_FAST        ADC_SMPR_28
#define ADC_SLOW        ADC_SMPR_480
#else
#include <STM32ADC.h>
#define ADC_NORM_SCALE  1
#define ADC_VBAT_SCALE  1
#define ADC_FAST        ADC_SMPR_28_5
#define ADC_SLOW        ADC_SMPR_239_5
#endif

#define ADC_ID ADC1

#if 0
#include <Arduino.h>
#include <HardwareTimer.h>
#include <libmaple/adc.h>
#include <boards.h>
#endif

class VCal // calibration
{
public:
   const float vRef; // expect 1.2 +- 0.04 Volts, calibration data where ?
   float vB, v;
   uint16_t n, ts;

   VCal (float v=3.3) : vRef(1.2) { vB= v; n= ts= 0; } // assumptions...

   uint8_t setVB (float vNRB, uint16_t t=0)
   {
      v= vNRB;
      if ((vNRB > 0.3) && (vNRB < 0.8)) // allow Vbat 4.0~1.5 V (1.6~3.6 nominal)
      {
         float v= vRef / vNRB;
         if (0 != t) { ts= t; }
         if (v != vB)
         {
             vB= v;
             n+= (n < 0xFFFF);
             return(2);
         }
         return(1);
      }
      return(0);
   }
}; // VCal

// Extend functionality of driver for STM32 built-in 12bit ADC
class ADC : public VCal, public STM32ADC
{
//static const normScale ???
public:
   float kR, kB;

   ADC (void) : VCal(), STM32ADC(ADC_ID) { setK(); }

   void pinSetup (uint8_t pn, uint8_t n)
   {
      for (int i=0; i<n; i++) { pinMode(pn+i, INPUT_ANALOG); }
   }

   void setK (void)
   {
      kR= vRef * ADC_NORM_SCALE;
      kB= vB * ADC_NORM_SCALE;
   }

   void setRef (uint8_t ref)
   {
#ifdef ARDUINO_ARCH_STM32F4
      switch(ref & 0x1)
      {
         case 0 : adc_enable_tsvref(); break;
         case 1 : adc_enable_vbat(); break;
      }
#endif // ARDUINO_ARCH_STM32F4
   }

#ifdef ARDUINO_ARCH_STM32F4
   bool calibrate (uint16_t t=0)
   {
      adc_enable_vbat();
      adc_set_sampling_time(ADC_ID,ADC_SLOW);
      // Read internal Vref, using Vbat/Vdd as full scale.
      // This explicitly requires power to Vbat: if not
      // from a battery, then by linking to Vdd (3.3V).
      // Typically this is not automatic when powered
      // by USB or debug header (YMMV). Seems unreliable...
      uint8_t r= setVB( readSumN(17,8) * 0.125 * ADC_NORM_SCALE , t);
      if (r > 0)
      {
         if (r > 1) { setK(); }
         return(true);
      }
      return(false);
   }
#else
   bool calibrate (uint16_t t=0) { STM32ADC::calibrate(); return(true); }
#endif // ARDUINO_ARCH_STM32F4

   uint32_t readSumN (uint8_t chan, uint8_t n)
   {
      uint32_t s= adc_read(ADC_ID,chan);
      for (uint8_t i=1; i<n; i++) { s+= adc_read(ADC_ID,chan); }
      return(s);
   } // readSumN

   uint32_t readSum16 (uint8_t chan, uint16_t n)
   {
      uint32_t s= adc_read(ADC1,chan);
      for (uint16_t i=1; i<n; i++) { s+= adc_read(ADC_ID,chan); }
      return(s);
   } // readSum16

   void setSamplePeriod (adc_smp_rate r)
   {
#ifdef ARDUINO_ARCH_STM32F4
      setSamplingTime(r); // adc_set_sampling_time(_dev, r);
#else
      setSampleRate(r); // adc_set_sample_rate(_dev, r);
#endif
   } // setSamplePeriod
}; // class ADC

#endif // ST_ANALOGUE_HPP
