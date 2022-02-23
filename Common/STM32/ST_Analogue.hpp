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
#include "ST_Util.hpp"

#if 0
#include <Arduino.h>
#include <HardwareTimer.h>
#include <libmaple/adc.h>
#include <boards.h>
#endif

#ifndef VREFIN_CAL
#define VREFIN_CAL 1.2
// hack VREFIN_CAL 0.7
#endif

namespace ADCProf {
   enum FMS : uint16_t { // flag, mask & shift definitions
      CHAN_M= 0x1F,     // Channel (0..18) mask (19..31 invalid)
      RJCT_M= 0x7,      // Reject count (0..7) mask = 0xE>>RJCT_S 
      RJCT_S= 5,        // Reject mask shift
      NCNT_F= 0x8000,   // no continuous *???*
      NMDS_F= 0x4000,   // no mode switch *DEPRECATE*
      NARJ_F= 0x2000    // no auto-reject
   };
}; // namespace ADCProf

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
// private utility code
struct ADCMode // profile ?
{ 
   volatile uint32_t *pMBCCR;
   uint8_t  chan, rej;

   ADCMode (uint16_t prof)
   {
      uint8_t vc;
      chan= vc= prof & ADCProf::CHAN_M;
      if (chan > 21) { chan-= 6; }
      else if (chan > 18) { chan-= 3; }
      ASSERT(chan <= 18);
      // default continuous conversion
      if (0 == (prof & ADCProf::NCNT_F)) { adc_start_continuous_convert(ADC_ID, chan); }
      rej= (prof >> ADCProf::RJCT_S) & ADCProf::RJCT_M;
      if (vc > 18) // virtual channels: special modes enabled
      {  // TSVREFE (bit22) for 19..21 versus, VBATE (bit23) for 22..24
         pMBCCR= CMX::bbp((void*)&(ADC_COMMON_BASE->CCR), 22 + (vc > 21));
         if (0 == *pMBCCR)
         {  // default auto reject on mode change
            if (0 == (prof & ADCProf::NARJ_F)) { rej+= (rej <= 0); }
            *pMBCCR= 1;
            return;
         }
      } // else 
      pMBCCR= NULL;
   } // ADCMode
   
   ~ADCMode (void)
   {
      adc_clear_continuous(ADC_ID);
      if (pMBCCR) { *pMBCCR= 0; }
   } // ~ADCMode

   // doesn't particularly belong here...
   uint16_t syncGetData (void) const
   {
      while ( !adc_is_end_of_convert(ADC_ID) ); // spin
      return adc_get_data(ADC_ID);  
   } // syncGetData

}; // ADCMode

   
//static const normScale ???
public:
   float kR, kD, kB;

   ADC (void) : VCal(), STM32ADC(ADC_ID)
   {
      setK(); 
      setSamplePeriod(ADC_FAST);
   }

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

   uint8_t readRawN (uint16_t raw[], const uint8_t n, const uint16_t prof)
   {
      const ADCMode mode(prof); 

      const uint32_t max= n + mode.rej;
      uint32_t j=0;
      for (uint32_t i=0; i<max; i++)
      {
         raw[j]= mode.syncGetData(); // overwrite until rejection complete
         j+= (i >= mode.rej);
      }
      return(n);
   } // readRawN
   
   uint32_t readRawSumN (const uint8_t n, const uint16_t prof)
   {
      const ADCMode mode(prof); 
      
      uint32_t s;
      uint8_t  i=0;
      do { s= mode.syncGetData(); } while (i++ < mode.rej); // overwrite until rejection complete
      for (i=1; i<n; i++) { s+= mode.syncGetData(); }
      return(s);
   } // readRawSumN

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
   uint8_t chan1,chan2,rej,dne; // Hacky test state variables
   
   // ADC clock PreScaler Data Bits -> clock divider value
   uint8_t decodePSDB (uint8_t psdb) // CAVEAT: GIGO
   {  // 3..0 -> 8, 6, 4, 2
      if (0x2 == psdb) { return(6); } //else
      psdb+= psdb < 0x2;
      return(1 << psdb);
   } // decodeClkDiv
   
public:
   ADCDbg (void) : ADC()
   {
      chan1=0; chan2=15; rej=0; dne=0; calF=0;
   }
   
   void init (Stream& s, uint8_t cd=0x2) { s.print("ADCDbg() - clkDiv="); s.println(setClkDiv(cd)); }
   
   void log (Stream& s, uint8_t m=0x10)
   {
      if (m & 0xF)
      {
         s.print("ADC1: 0x"); s.print((uint32_t)ADC1_BASE,HEX); 
         if (m & 0x1)
         {
            s.print(" SR=0x"); s.print(ADC1_BASE->SR,HEX);
            s.print(" CR1=0x"); s.print(ADC1_BASE->CR1,HEX);
            s.print(" CR2=0x"); s.print(ADC1_BASE->CR2,HEX);
         }
         if (m & 0x2)
         {
            s.print(" SMPR1="); dumpBits(s, ADC1_BASE->SMPR1, (3<<5)|9);
            s.print(" SMPR2="); dumpBits(s, ADC1_BASE->SMPR2, (3<<5)|9);
         }
         if (m & 0x4)
         {
            s.print(" SQR1="); dumpBits(s, ADC1_BASE->SQR1 & 0xFFFFFF, (5<<5)|5);
            s.print(" SQR2="); dumpBits(s, ADC1_BASE->SQR2, (5<<5)|6);
            s.print(" SQR3="); dumpBits(s, ADC1_BASE->SQR2, (5<<5)|6);
            s.print(" JSQR="); dumpBits(s, ADC1_BASE->JSQR & 0x3FFFFF, (5<<5)|6);
         }
         //if (m & 0x8) { }
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
      if (m & 0x80)
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
      if (++chan1 >= ADC_TEST_NUM_CHAN) { chan1=0; }
      aV= readRawSumN(16,chan1) * (1.0/16) * kD;
      s.print(" CH"); s.print(chan1); s.print('='); s.print(aV,4); s.print("V ");

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

   void test2 (Stream& s, uint16_t iter)
   {
      uint16_t r[16], m;
      uint32_t sum;
      if (++chan2 > 24) { chan2=15; dne^= 0x40; }
      int8_t n= readRawN(r, 16, ((uint16_t)dne<<8)|rej|chan2);
      if (n > 0)
      {
         s.print("RMR["); s.print(chan2); s.print("]: "); 
         s.print(r[0]);
         sum= r[0];
         for (int8_t i=1; i<n; i++) { s.print(','); s.print(r[i]); sum+= r[i]; }
         m= sum/n;
         s.print("; M="); s.println(m);
         s.print("DIFF:");
         s.print(r[0]-m); for (int8_t i=1; i<n; i++) { s.print(','); s.print(r[i]-m); }
         s.println(" :RMR");
      }
   } // test2
   
   // ADC input clock divider (relative to APB2 clock rate)
   uint32_t getClkDiv (bool global=false) 
   {
      uint32_t d= decodePSDB((ADC_COMMON_BASE->CCR >> 16) & 0x3);
      if (global)
      {
         F4RCC rcc;
         d*= rcc.clkDivAPB2();
      }
      return(d);
   } // getClkDiv
   
   uint8_t setClkDiv (uint8_t psdb)
   {
      uint32_t ccr= ADC_COMMON_BASE->CCR;
      if ((psdb & 0x3) != psdb) { psdb= (ccr >> 16) & 0x3; }
      else
      {
         ccr&= ~(0x3 << 16);
         ccr|= psdb << 16;
         ADC_COMMON_BASE->CCR= ccr;
      }
      return decodePSDB(psdb);
   } // setClkDiv

   // Determine ADC clock rate from the clock divider hierarchy: AHB - APB2 (HS) - ADC.
   uint32_t clk (Stream& s)
   {
      uint32_t d= getClkDiv(true);
      uint32_t c= ST_CORE_CLOCK / d;
      s.print("clkDiv="); s.println(d);
      s.print("ADClk="); s.print(c * 1E-6); s.println("MHz");
      return(c);
   }
}; // ADCDbg

#endif // ST_ANALOGUE_HPP
