// Duino/SGAD9833/DA_ad9833HW.hpp - Arduino-AVR specific class definitions for
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Dec 2020

// HW Ref:  http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf

#ifndef DA_AD9833_HW_HPP
#define DA_AD9833_HW_HPP

// See following include for basic definitions & information
#include "Common/MBD/ad9833Util.h" // Transitional ? - links to UU16 etc
#include "Common/AVR/DA_Args.hpp"
#include "Common/AVR/DA_SPIMHW.hpp"


/***/

#define F_TO_FSR(f) (f * (((uint32_t)1<<28) / 25E6))


/***/

// wrapper class: tweak settings for AD9833
class DA_AD9833SPI : public DA_SPIMHW
{
public:
   DA_AD9833SPI () : DA_SPIMHW() {;}

   void setGain (uint8_t b)
   {
      beginTransS1();
      //writeS2(0x21, 0x00); // shutdown
      writeS2(0x11, b);
      endTrans();
   } // setGain
   
   void writeAD9833 (const uint8_t b[], const uint8_t n)
   {
#ifdef DA_FAST_POLL_TIMER_HPP
      // Direct twiddling of select-pin helps performance but variable latency
      // ('duino interrupts & setup?) results in throughput of 0.6~0.8 MByte/s
      stamp();
#endif // DA_FAST_POLL_TIMER_HPP
      beginTransS1();
      for (uint8_t i=0; i<n; i+= 2) { writeS1A16BE(b+i); }
      endTrans();
#ifdef DA_FAST_POLL_TIMER_HPP
      dbgTransClk= diff();
      dbgTransBytes= n;
#endif // DA_FAST_POLL_TIMER_HPP
   } // writeAD9833
}; // class DA_AD9833SPI

// displace ??
static uint32_t toFSR (const CNumBCDX& v) { return v.extractScale(10737,-3); }


struct DA_AD9833Reg16
{
   UU16 r;
   void setZeroFSR (const uint8_t ia)
   {  // zero data, set address
      r.u8[0]= 0;
      r.u8[1]= (ia+0x1) << 6;
   } // setZeroFSR

   void setZeroPSR (const uint8_t ia)
   {
      r.u8[0]= 0;
      r.u8[1]= ((0x6+ia) << 5);
   }
}; // DA_AD9833Reg16

// Classes are used because Arduino has limited support for modularisation and sanitary source reuse.
// Default build settings (minimise size) prevent default class-method-declaration inlining
class DA_AD9833FreqPhaseReg
{  // Associate registers for frequency (pair) and phase to simplify higher API -
public:  // - not concerned with frequency/phase-shift keying...
   UU16 fr[2], pr;

   DA_AD9833FreqPhaseReg  () {;} // should all be zero on reset

   void setFSR (const uint32_t fsr)
   {
      fr[0].u16=  AD9833_FSR_MASK & fsr;
      fr[1].u16=  AD9833_FSR_MASK & (fsr >> 14);
   } // setFSR

   uint32_t getFSR (uint8_t lsh) const
   {
      switch(lsh)
      {
         case 2 : return( (fr[0].u16 << 2) | (((uint32_t)fr[1].u16 & AD9833_FSR_MASK) << 16) );
         default : return( (fr[0].u16 & AD9833_FSR_MASK) | (((uint32_t)fr[1].u16 & AD9833_FSR_MASK) << 14) );
      }
   } // getFSR

   void setPSR (const uint16_t psr) { pr.u16= AD9833_PSR_MASK & psr; }

   // Masking of address bits. NB: assumes all zero!

   void setFAddr (const uint8_t ia) // assert((ia & 0x1) == ia));
   {  // High (MS) bytes - odd numbered in little endian order
      const uint8_t a= (ia+0x1) << 6;
      fr[0].u8[1]|= a;
      fr[1].u8[1]|= a;
   } // setFAddr

   void setZeroFSR (const uint8_t ia)
   {  // zero data, set address
      const uint8_t a= (ia+0x1) << 6;
      fr[1].u8[0]= fr[0].u8[0]= 0;
      fr[1].u8[1]= fr[0].u8[1]= a;
   } // setZeroFSR
   bool isZeroFSR (void) const { return(0 == (AD9833_FSR_MASK & (fr[0].u16 | fr[1].u16))); }

   void setPAddr (const uint8_t ia) { assert(ia==ia&1); pr.u8[1]|= ((0x6+ia) << 5); }
   void setZeroPSR (const uint8_t ia)
   {  // zero data, set address
      pr.u8[0]= 0;
      pr.u8[1]= ((0x6+ia) << 5);
   } // setZeroPSR
}; // class DA_AD9833FreqPhaseReg

// Consider : rename to reflect full register map/set rather than single?
class DA_AD9833Reg : protected DA_AD9833SPI
{
public:
   union
   {
      byte b[14]; // compatibility hack
      struct { UU16 ctrl; DA_AD9833FreqPhaseReg fpr[2]; };
   };

   using DA_AD9833SPI::setGain;
   
   DA_AD9833Reg (void)
   {
      ctrl.u8[1]= AD9833_FL1_B28|AD9833_FL1_RST;
      fpr[0].setPAddr(0); fpr[1].setPAddr(1);
      // register order inversion test: no change...
      //if (inv) { ix= 1; ctrl.u8[1]|= AD9833_FL1_FSEL|AD9833_FL1_PSEL; }
   } // DA_AD9833Reg

   void setFSR (uint32_t fsr, const uint8_t ia=0)
   {
      fpr[ia].setFSR(fsr);
      fpr[ia].setFAddr(ia);
      //else { fpr[ia].setFAddr(ia^ix); }
   }
   void setZeroFSR (const uint8_t ia=0) { fpr[ia].setZeroFSR(ia); }
   bool isZeroFSR (const uint8_t ia=0) { return fpr[ia].isZeroFSR(); }

   void setPSR (const uint16_t psr, const uint8_t ia=0)
   {
      fpr[ia].setPSR(psr);
      fpr[ia].setPAddr(ia);
      //else { fpr[ia].setPAddr(ia^ix); }
   }

   void write (uint8_t f, uint8_t c) { return writeAD9833(b+(f<<1), c<<1); }

   void write (const uint8_t wm=0x7F)
   {
      if (0x07 == wm) { return writeAD9833(b, 6); }
      else
      {
         uint8_t first= 0, count=0, tm= wm & 0x7F;
         while (0 == (tm & 0x1)) { tm>>= 1; ++first; }
         while (tm) { tm>>= 1; ++count; }
         write(first,count);
      }
      if ((wm & 0x80) && (ctrl.u8[1] & AD9833_FL1_RST))
      {
         ctrl.u8[1]^= AD9833_FL1_RST;
         writeAD9833(b,2);
      }
   } // write
}; // class DA_AD9833Reg

// A separate class (instance) for chirp allows easy state preservation
// Although an application-level feature, tight hardware coupling is necessary
// to achieve the required performance , so it will remain here.
class DA_AD9833Chirp : protected DA_AD9833SPI
{
   uint8_t  duty;
   //DA_AD9833Reg16 zfsr;
   //uint16_t zeroFSR;
   union
   {  // C++ eccentricity : class instance within anon struct/union requires array declaration...
      byte b[8];
      struct { UU16 ctrl; DA_AD9833FreqPhaseReg fpr[1]; };
   };

public:
   DA_AD9833Chirp (void) {;}

   void begin (uint32_t fsr)
   {
      //zfsr.setZeroFSR(1);
      ctrl.u8[0]= 0;
      ctrl.u8[1]= AD9833_FL1_B28|AD9833_FL1_FSEL|AD9833_FL1_PSEL;
      fpr[0].setFSR(fsr);
      //fpr[0].fr[0].u16= zfsr.r.u16; // hack
      
      fpr[0].setFAddr(1);
      fpr[0].setZeroPSR(1);
      writeAD9833(b,8); // write everything
      // Prepare for compact writes (ctrl + upper word of fsr)
   } // begin

   void chirp (uint8_t step=8, uint8_t dus=5)
   {
      if (0 != --duty) { return; }
      duty= 4; // 4:1 (20%)
      fpr[0].fr[0].u16= fpr[0].fr[1].u16; // reuse lo word as incrementable hi
      ctrl.u8[0]= 0; // sine
      ctrl.u8[1]= AD9833_FL1_HLB|AD9833_FL1_FSEL|AD9833_FL1_PSEL; // compact writes: hi word only
      beginTransS1(); // noInterrupts();
      writeS1A16BE(b+0);
      writeS1A16BE(b+2);
      for (int8_t i=0; i<10; i++)
      {
         // spin delay 5~10us ???
         delayMicroseconds(dus);
         fpr[0].fr[0].u16+= step;
         writeS1A16BE(b+0);
         writeS1A16BE(b+2);
      }
      delayMicroseconds(dus);
      // go quiet, ready for next chirp
      //ctrl.u8[0]= AD9833_FL0_SLP_DAC|AD9833_FL0_SLP_MCLK; // useless
      ctrl.u8[1]= AD9833_FL1_RST;
      writeS1A16BE(b+0);
      endTrans(); // interrupts();
   } // chirp

   void end ()
   {
      ctrl.u8[0]= 0;
      ctrl.u8[1]= AD9833_FL1_B28;
      writeAD9833(b,2);
   }// end

}; // DA_AD9833Chirp

#endif // DA_AD9833_HW_HPP
