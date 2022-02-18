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

#include "../CMX_Util.hpp"

#if 0
#include <Arduino.h>
#include <HardwareTimer.h>
#include <libmaple/adc.h>
#include <boards.h>
#endif
#include <libmaple/rcc.h>
#ifndef PERIPH_BASE
#define PERIPH_BASE  ((uint32_t)0x40000000)
#endif
#ifndef RCC_BASE
#define RCC_BASE     ((rcc_reg_map*)(PERIPH_BASE + 0x23800))
#endif

// DISPLACE -> ?
class F4RCC
{
protected:

   uint8_t shClkDivAHB (const uint32_t cfgr)
   {  // bits 4..7 = HPRE (AHB prescaler)
      const uint8_t ahbdb= (cfgr >> 4) & 0xF;
      if (ahbdb < 8) { return(0); } // else
      // codes 8..15 -> divider 2, 4, 8, 16, 64, 128, 256, 512
      uint8_t sh= ahbdb - 7;
      sh+= (sh >= 5); // 32 skipped
      return(sh);
   } // shClkDivAHB
   
   uint8_t shClkDivAPB2 (const uint32_t cfgr)
   {  // bits 13..15 = PPRE2 (APB2 high speed prescaler)
      const uint8_t apb2db= (cfgr >> 13) & 0x7;
      if (apb2db < 4) { return(0); } // else
      // codes 4..7 -> divider 2, 4, 8, 16
      return(apb2db - 3);
   } // shClkDivAPB2
   
public:
   F4RCC (void) { ; }
   
   uint32_t clkDivAPB2 (void)
   {
      uint32_t cfgr= RCC_BASE->CFGR;
      return((uint32_t)1 << ( shClkDivAHB(cfgr) + shClkDivAPB2(cfgr) ) ); //return(((uint16_t)1 << ahbsh) * (1 << apb2sh));
   } // clkDivAPB2
   
}; // F4RCC

class F4RCCDbg : public F4RCC
{
public:
   F4RCCDbg (void) : F4RCC() { ; }
   
   uint32_t clkDivAPB2 (Stream& s)
   {
      uint32_t cfgr= RCC_BASE->CFGR;
      uint8_t sa= shClkDivAHB(cfgr);
      uint8_t sb= shClkDivAPB2(cfgr);
      s.print("F4RCC: cfgr="); s.print(cfgr,HEX);
      s.print(" shClkDiv* AHB="); s.print(sa); s.print(" APB2="); s.print(sb); s.println(" :F4RCC");
      return((uint32_t)1 << ( sa + sb ) ); //return(((uint16_t)1 << sa) * (1 << sb));
   } // clkDivAPB2

}; // F4RCCDbg

#ifndef VREFIN_CAL
#define VREFIN_CAL 1.2
// hack VREFIN_CAL 0.7
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

   uint8_t readMultiRaw (uint16_t raw[], const uint8_t n, const uint8_t crm)
   {
      const uint8_t rej= crm >> 5, chan= crm & 0x1F; 
      if (chan <= 18)
      {
         volatile uint32_t *pCCR= NULL; //CMX::bbp((void*)&(ADC_COMMON_BASE->CCR));
         uint8_t eb= 0;
         if (chan >= 16)
         {  //rej++;
            //adc_set_sampling_time(ADC_ID,ADC_SLOW);
            pCCR= CMX::bbp((void*)&(ADC_COMMON_BASE->CCR));
            eb= 22 + (18 == chan); // TSVREFE for 16&17, VBATE for 18
            pCCR[eb]= 1;
         }
         adc_start_continuous_convert(ADC_ID,chan);
         for (uint8_t i=0; i<rej; i++)
         {
            while ( !adc_is_end_of_convert(ADC_ID) ); // spin
            adc_get_data(ADC_ID); // reject
         }
         for (uint8_t i=0; i<n; i++)
         {
            while ( !adc_is_end_of_convert(ADC_ID) ); // spin
            raw[i]= adc_get_data(ADC_ID);
         }
         if (pCCR) { pCCR[eb]= 0; }
         return(n);
      }
      return(0);
   } // readMultiRaw
   
#ifdef ARDUINO_ARCH_STM32F4
   uint16 rawCal[3];
   bool calibrate (uint16_t t=0)
   {
      int8_t r;
      //  // switch on but off (?..)
      //adc_set_sampling_time(ADC_ID,ADC_SLOW);

      volatile uint32_t *pCCR= CMX::bbp((void*)&(ADC_COMMON_BASE->CCR));
      pCCR[23]= 1;   // TSVREFE on = adc_enable_tsvref();
      delayMicroseconds(10);
      rawCal[0]= adc_read(ADC_ID,16); // Therm
      pCCR[23]= 0;   // TSVREFE off
      delayMicroseconds(10);

      rawCal[1]= adc_read(ADC_ID,17); // VRefInt
      r= setVD( rawCal[1] ); 
      
      // Read Vbat via 1:3 divider, seems to use Vdd rather
      // than internal 1.2V reference as full scale.
      // This explicitly requires power to Vbat: if not
      // from a battery, then by linking to Vdd (3.3V).
      // Typically this is not automatic when powered
      // by USB or debug header (YMMV). Still debugging...
      //volatile uint32_t *pVBATE= bbp((void*)&(ADC_COMMON_BASE->CCR),22);
      pCCR[22]= 1;   // VBATE on = adc_enable_vbat();
      delayMicroseconds(10);
      rawCal[2]= adc_read(ADC_ID,18); // VBat
      pCCR[22]= 0;   // VBATE off
      r+= setVB( rawCal[2], t);
      // now set conversion coefficients as necessary
      if (r > 0) { setK(); }
      return(r >= 0);
   } // calibrate
   
   float readT (void)
   {
      volatile uint32_t *pCCR= CMX::bbp((void*)&(ADC_COMMON_BASE->CCR));
      pCCR[23]= 1;   // TSVREFE
      //adc_set_sampling_time(ADC_ID,ADC_SLOW);
      delayMicroseconds(10);
      float t= STM32ADC::readTemp();
      pCCR[23]= 0;
      return(t);
   } // readT
#else
   bool calibrate (uint16_t t=0) { STM32ADC::calibrate(); return(true); }
#endif // ARDUINO_ARCH_STM32F4

   uint32_t readSumN (uint8_t chan, uint8_t n)
   {
      uint32_t s;
      adc_start_continuous_convert(ADC_ID,chan);
      while ( !adc_is_end_of_convert(ADC_ID) ); // spin
      s= adc_get_data(ADC_ID);
      for (uint8_t i=1; i<n; i++)
      {
         while ( !adc_is_end_of_convert(ADC_ID) ); // spin
         s+= adc_get_data(ADC_ID);
      }
      resetContinuous();
      return(s);
   } // readSumN

   // return result bits
   uint8_t setSamplePeriod (adc_smp_rate r)
   {
#ifdef ARDUINO_ARCH_STM32F4
      setSamplingTime(r); // adc_set_sampling_time(_dev, r);
      ADC1_BASE->SMPR1|= 0x07FC0000; // always 480 cycles for channels 16 through 18 on ADC1
      if (r == ADC_SMPR_3) { return(6); } // bits
#else
      setSampleRate(r); // adc_set_sample_rate(_dev, r);
#endif
      return(12);
   } // setSamplePeriod

}; // class ADC

#ifndef ADC_TEST_NUM_CHAN
#define ADC_TEST_NUM_CHAN 1
#endif
 
class ADCDbg : public ADC
{
private:
   uint16_t calF;
   uint8_t chan;
   
public:
   ADCDbg (void) : ADC() { chan=15; calF=0; setSamplePeriod(ADC_FAST); }
   
   void log (Stream& s, uint8_t m=0x10)
   {
      if (m & 0x1)
      {
         s.print("ADC1: 0x"); s.print((uint32_t)ADC1_BASE,HEX); 
         //const volatile uint32_t *p= CMX::bbp((void*)&(ADC1_BASE->CR2));
         //volatile uint32 *p= bb_perip((void*)&(ADC1_BASE->CR2),0);
         //s.print(" bbp: 0x"); s.print((uint32_t)p); s.print("->"); s.print(*p);
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
      bool r= ADC::calibrate(t);
      //log(s,0x2);
      s.print(" vD="); s.print(VCal::vD,4);
      s.print(" vB="); s.println(VCal::vB,4);
      return(r);
   } // debugCalibrate
   
   void test1 (Stream& s, uint16_t iter)
   {
      float aV;
      //log(s);
      if (++chan >= ADC_TEST_NUM_CHAN) { chan=0; }
      aV= readSumN(chan,16) * (1.0/16) * kD;
      s.print(" CH"); s.print(chan); s.print('='); s.print(aV,4); s.print("V ");

       //---
       //if ((0 == adc.n) || 
      if (iter > (VCal::ts+100))
      {
         calF+= !debugCalibrate(s, iter);
         s.print(" calF="); s.print(calF);
      }
      float vR= readVref();
      float tC= readT();
      s.print(" tC="); s.print(tC,4); s.print("C vR="); s.print(vR,4); s.println('V');
   } // test1

   void test2 (Stream& s)
   {
      uint16_t r[20];
      if (++chan > 18) { chan=16; }
      int8_t n= readMultiRaw(r, 20, (1<<5)|chan);
      if (n > 0)
      {
         s.print("RMR["); s.print(chan); s.print("]: "); s.print(r[0]);
         for (int8_t i=1; i<n; i++) { s.print(','); s.print(r[i]); }
         s.println(" :RMR");
      }
   } // test2
   
   uint8_t clkDiv (void)
   {  // 3..0 -> 8, 6, 4, 2
      uint8_t psdb= (ADC_COMMON_BASE->CCR >> 16) & 0x3;
      if (0x2 == psdb) { return(6); } //else
      psdb+= psdb < 0x2;
      return(1 << psdb);
   } // clkDiv 
   
   uint32_t clk (Stream& s)
   {  // PCLK2 (=APB2?) / PSD
      F4RCCDbg rcc;
      uint16_t a= rcc.clkDivAPB2(s);
      uint8_t  b= clkDiv();
      uint32_t d= a * b;
      uint32_t c= 84000000 / d;
      s.print("clkDiv="); s.print(a); s.print('*');s.print(b); s.print("="); s.println(d);
      s.print("ADClk="); s.print(c * 1E-6); s.println("MHz");
      return(c);
   }
}; // ADCDbg

#endif // ST_ANALOGUE_HPP
