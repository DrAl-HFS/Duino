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
#define ADC_VBAT_SCALE  4.0   // Vbat ADC input uses 1:3 divider
#define ADC_VBAT_SHIFT  2     // scale integer by left shift
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

#ifndef VREFIN_CAL
#define VREFIN_CAL 1.15
#endif

#if 0
#include <libmaple/bitband.h>
// bb_perip(a,b)
#else
extern "C" { // TODO: displace to where ?

   // General bitband pointer function
   volatile uint32_t *bbp (volatile void *p, const uint8_t b=0)
   {
      ASSERT(b == (b & 0x1F)); // 0x7 byte-wise ???
      uint32_t o, w= (uint32_t)p;
      o= w & 0x000FFFFF;
      w&= 0xF0000000;
      w|= 0x02000000 + (o<<5) + (b<<2);
      return((uint32_t*)w);
   }

}; // extern "C"
#endif

class VCal // calibration
{
public:
   const float vRef; // expect 1.2 +- 0.04 Volts, calibration data where ?
   float vD, vB;
   uint16_t n, ts;
   // dbg tmp uint16_t r14;

   VCal (float v=3.3) : vRef(VREFIN_CAL) { vB= vD= v; n= ts= 0; } // assumptions...

   int8_t setVD (uint16_t raw12)
   {
      float v= vRef / (raw12 * ADC_NORM_SCALE);
      if (v != vD)
      {
         vD= v;
         return(1);
      }
      return(0);
   } // setVD

   int8_t setVB (uint16_t raw12, uint16_t t=0)
   {
      int8_t r= 0;
      uint16_t r2_12= raw12 << ADC_VBAT_SHIFT;
      float v= r2_12 * ADC_NORM_SCALE * vD;
      if (v < 1.6) { r= -1; } // brown-out
      if (v > 3.6) { r= -2; } // damaging
      //else
      if (0 != t) { ts= t; }
      if (v != vB)
      {
          vB= v;
          n+= (n < 0xFFFF);
          r+= (0 == r);
      }
      return(r);
   } // setVB
   
}; // VCal

// Extend functionality of driver for STM32 built-in 12bit ADC
class ADC : public VCal, public STM32ADC
{
//static const normScale ???
public:
   float kR, kD, kB;

   ADC (void) : VCal(), STM32ADC(ADC_ID) { setK(); }

   void pinSetup (uint8_t pn, uint8_t n)
   {
      for (int i=0; i<n; i++) { pinMode(pn+i, INPUT_ANALOG); }
   }

   void setK (void)
   {
      kR= vRef * ADC_NORM_SCALE;
      kD= vD * ADC_NORM_SCALE;
      kB= vB * ADC_NORM_SCALE;
   }

#ifdef ARDUINO_ARCH_STM32F4
   uint16 rawCal[3];
   bool calibrate (uint16_t t=0)
   {
      int8_t r;
      //adc_enable_vbat(); adc_enable_tsvref(); // switch on but off (?..)
      adc_set_sampling_time(ADC_ID,ADC_SLOW);

      volatile uint32_t *pCCR= bbp((void*)&(ADC_COMMON_BASE->CCR));
      pCCR[23]= 1;   // TSVREFE
      delayMicroseconds(10);
      rawCal[0]= adc_read(ADC_ID,16);
      pCCR[23]= 0;
      delayMicroseconds(10);
      // T
      rawCal[1]= adc_read(ADC_ID,17);
      r= setVD( rawCal[1] ); 
      
      // Read Vbat via 1:3 divider, seems to use Vdd rather
      // than internal 1.2V reference as full scale.
      // This explicitly requires power to Vbat: if not
      // from a battery, then by linking to Vdd (3.3V).
      // Typically this is not automatic when powered
      // by USB or debug header (YMMV). Still debugging...
      //volatile uint32_t *pVBATE= bbp((void*)&(ADC_COMMON_BASE->CCR),22);
      pCCR[22]= 1; // VBATE
      delayMicroseconds(10);
      rawCal[2]= adc_read(ADC_ID,18);
      pCCR[22]= 0;
      r+= setVB( rawCal[2], t);
      // now set conversion coefficients as necessary
      if (r > 0) { setK(); }
      return(r >= 0);
   } // calibrate
   
   void log (Stream& s, uint8_t m=0x10)
   {
      if (m & 0x1)
      {
         const volatile uint32_t *p= bbp((void*)&(ADC1_BASE->CR2));
         //volatile uint32 *p= bb_perip((void*)&(ADC1_BASE->CR2),0);
         s.print("ADC1: 0x"); s.print((uint32_t)ADC1_BASE,HEX); 
         s.print(" bbp: 0x"); s.print((uint32_t)p); s.print("->"); s.print(*p);
         s.print(" SR=0x"); s.print(ADC1_BASE->SR,HEX);
         s.print(" CR1=0x"); s.print(ADC1_BASE->CR1,HEX);
         s.print(" CR2=0x"); s.print(ADC1_BASE->CR2,HEX);
         s.println(" :ADC1");
      }
      if (m & 0x10)
      {
         s.print("ADC*:");
         s.print(" CCR=0x"); s.print(ADC_COMMON_BASE->CCR,HEX);
         s.print(" CSR=0x"); s.print(ADC_COMMON_BASE->CSR,HEX);
         s.print(" CDR=0x"); s.print(ADC_COMMON_BASE->CDR,HEX);
         s.println(" :ADC*");
      }
      if (m & 0x2)
      {
         s.print("rawCal: "); 
         s.print(" Ch16=0x"); s.print(rawCal[0],HEX);
         s.print(" Ch17=0x"); s.print(rawCal[1],HEX);
         s.print(" Ch18=0x"); s.print(rawCal[2],HEX);
         s.println(":rawCal"); 
      }
   } // log
   bool debugCalibrate (Stream& s, uint16_t t=0)
   {
      bool r= calibrate(t);
      //log(s,0x2);
      s.print(" vD="); s.print(VCal::vD,4);
      s.print(" vB="); s.println(VCal::vB,4);
      return(r);
   } // debugCalibrate

#else
   bool calibrate (uint16_t t=0) { STM32ADC::calibrate(); return(true); }
#endif // ARDUINO_ARCH_STM32F4

   uint32_t readSumN (uint8_t chan, uint8_t n)
   {
      uint32_t s;
      setContinuous();
      s= adc_read(ADC_ID,chan);
      for (uint8_t i=1; i<n; i++) { s+= adc_read(ADC_ID,chan); }
      resetContinuous();
      return(s);
   } // readSumN

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
