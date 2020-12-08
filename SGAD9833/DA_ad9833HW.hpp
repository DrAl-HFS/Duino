// Duino/Common/DA_ad9833HW.hpp - Arduino-AVR specific class definitions for 
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Nov 2020

// HW Ref:  http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf

#ifndef DA_AD9833_HW_HPP
#define DA_AD9833_HW_HPP

#include <SPI.h>

// See following include for basic definitions & information
#include "Common/MBD/ad9833Util.h" // Transitional
//#include "Common/DA_Util.h"
#include "Common/DA_Args.hpp"


/***/

#ifdef AVR // 328P specific ?
// Pin number used for SPI select (active low)
// any logic 1|0 output will do...
// Just use SS pin - must be an output to prevent
// SPI HW switching to slave mode (master + slave
// operation requires some extra signalling/arbitration)
#define PIN_SEL SS // (#10 on Uno)
// 9
// Hardware SPI pins used implicitly
//#define PIN_SCK SCK   // (#13 on Uno)
//#define PIN_DAT MOSI  // (#11 on Uno)
#endif


/***/

/*
extern "C" bool isZero (const UU16 fr[2])
{  // not working as expected... ???
   if (0 == (fr[0].u8[0] | fr[1].u8[0])) { return(0 == (AD9833_FSR_MASK_8H & (fr[0].u8[1] | fr[1].u8[1]))); }
   return(false);
} // isZeroFSR
*/
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
//=0
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
   bool isZeroFSR (void) const { return(0 == (AD9833_FSR_MASK & (fr[0].u16 | fr[1].u16))); } // { return isZero(fr); }

   void setPAddr (const uint8_t ia) { assert(ia==ia&1); pr.u8[1]|= ((0x6+ia) << 5); }
}; // class CDA_AD9833FreqPhaseReg

//#include "Common/DA_FastPollTimer.hpp"

//uint16_t rd16BE (const uint8_t b[2]) { return((256*(uint16_t)(b[0])) | b[1]); }

#if (SS == PIN_SEL) // Directly toggle SS (PB2) for performance
#define SET_SEL_LO() PORTB &= ~(1<<2) // should generate CBI / SBI 
#define SET_SEL_HI() PORTB |= (1<<2)  // instructions as below
/* ...arduino.../hardware/tools/avr/avr/include/avr/iom328p.h
#define SET_SEL_LO() { asm("CBI 0x25, 2"); } // IO address 0x20 + 0x5 ?
#define SET_SEL_HI() { asm("SBI 0x25, 2"); }
TODO - properly comprehend gcc assembler arg handling...
#define ASM_CBI(port,bit) { asm("CBI %0, %1" : "=r" (port) : "0" (u)); }
#define SET_SEL_LO() { asm("CBI %0, 2" : "=" PORTB ); } 
#define SET_SEL_HI() { asm("SBI %0, 2" : "=" PORTB ); }
*/
#else   // portable & robust (but slow) versions
#define SET_SEL_LO() digitalWrite(PIN_SEL, LOW)
#define SET_SEL_HI() digitalWrite(PIN_SEL, HIGH)
#endif

class DA_AD9833SPI // : public CFastPollTimer
{
public:
#ifdef DA_FAST_POLL_TIMER_HPP
   uint8_t dbgTransClk, dbgTransBytes;
#endif // DA_FAST_POLL_TIMER_HPP
   DA_AD9833SPI ()
   {
      SPI.begin();
#if (PIN_SEL != SS) // SS is already an output
      pinMode(PIN_SEL, OUTPUT);
#endif // PIN_SEL
      digitalWrite(PIN_SEL, HIGH);
   }
   void writeSeq (const uint8_t b[], const uint8_t n)
   {
#ifdef DA_FAST_POLL_TIMER_HPP
      // Direct twiddling of select-pin helps performance but variable latency
      // ('duino interrupts & setup?) results in throughput of 0.6~0.8 MByte/s
      stamp();
#endif // DA_FAST_POLL_TIMER_HPP
      SPI.beginTransaction(SPISettings(8E6, MSBFIRST, SPI_MODE2));
      for (uint8_t i=0; i<n; i+= 2)
      {
         SET_SEL_LO(); // Falling edge (low level enables input)
         //SPI.transfer16( rd16BE(b+i) ); // Total waste of time - splits into bytes internally
         SPI.transfer(b[i+1]);  // MSB (send in big endian byte order)
         SPI.transfer(b[i+0]);  // LSB
         SET_SEL_HI(); // Rising edge latches to target register
      }
      SPI.endTransaction();
#ifdef DA_FAST_POLL_TIMER_HPP
      dbgTransClk= diff();
      dbgTransBytes= n;
#endif // DA_FAST_POLL_TIMER_HPP
   } // writeSeq
}; // class DA_AD9833SPI

class DA_AD9833Reg : public DA_AD9833SPI
{
public:
   union
   {
      byte b[14]; // compatibility hack
      struct { UU16 ctrl; DA_AD9833FreqPhaseReg fpr[2]; };
   };

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

   void write (uint8_t f, uint8_t c) { return writeSeq(b+(f<<1), c<<1); }

   void write (const uint8_t wm=0x7F)
   {
      if (0x07 == wm) { return writeSeq(b, 6); }
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
         writeSeq(b,2);
      }
   } // write
}; // class DA_AD9833Reg


/*** application level split DA_AD9833UIF? ***/

#define CYCM_CMP_HI  0x08
#define CYCM_CONS_HI 0x04
#define CYCM_CMP_LO  0x02
#define CYCM_CONS_LO 0x01
#define CYCM_WRAP_HI 0x40
#define CYCM_MIRR_HI 0x80
#define CYCM_WRAP_LO 0x10
#define CYCM_MIRR_LO 0x20

class DA_Cycle
{
public :
   uint32_t lim[2];  // NB : implicit order, lim[1] > lim[0]
   uint8_t  mode, state;
   
   DA_Cycle () { ; }
   uint32_t operator () (uint32_t v)
   {
      state= 0; // compare ?
      if ((mode & CYCM_CMP_HI) && (v > lim[1])) { state|= CYCM_CMP_HI; } // hi
      if ((mode & CYCM_CMP_LO) && (v < lim[0])) { state|= CYCM_CMP_LO; } // lo

      // ignore if limits not distinct
      if (CYCM_CMP_HI == state)
      {
         switch (mode & (CYCM_MIRR_HI|CYCM_WRAP_HI))
         {
            case CYCM_WRAP_HI : // wrap hi to lo
               state|= CYCM_WRAP_HI;
               if (mode & CYCM_CONS_HI) { return(lim[0] + ((int32_t)v - lim[1])); } // conserve time
               else { return(lim[0]); } // simple wrap
            case CYCM_MIRR_HI : // mirror
               state|= CYCM_MIRR_HI;
               if (mode & CYCM_CONS_HI) { return(lim[1] + ((int32_t)lim[1] - v)); } // conserve
               //else default
            default : return(lim[1]); // simple mirror / clamp hi
         }
      }
      if (CYCM_CMP_LO == state)
      {  
         switch (mode & (CYCM_MIRR_LO|CYCM_WRAP_LO))
         {
            case CYCM_WRAP_LO : // wrap lo to hi
               state|= CYCM_WRAP_LO;
               if (mode & CYCM_CONS_LO) { return(lim[1] + ((int32_t)v - lim[0])); } // conserve
               else { return(lim[0]); } // simple wrap
            case CYCM_MIRR_LO : // mirror
               state|= CYCM_MIRR_LO;
               if (mode & CYCM_CONS_LO) { return(lim[0] + ((int32_t)lim[0] - v)); } // conserve
               //else default
            default : return(lim[0]); //  simple mirror / clamp lo
         }
      }
      return(v);
   } // operator ()
}; // DA_Cycle

class DA_AD9833Sweep
{
protected:  // NB: fsr values in AD9833 native 28bit format (fixed point fraction of 25MHz)
   DA_Cycle cycle; // uint32_t lim[2];
   uint32_t dt; // sweep start&end (lo&hi?) limits, interval (millisecond ticks)
   int32_t range; // signed (?) difference between end and start
   UU32 fsr;
   int32_t  sLin; // linear step
   uint32_t rPow; // power-law (growth) rate
   UU64 q52; // temp val

   // Primitive operations
   uint8_t setF (USciExp fv[2])
   {
      uint8_t m=0;
      cycle.lim[0]= fv[0].toFSR();
      cycle.lim[1]= fv[1].toFSR();
      if (cycle.lim[0] > cycle.lim[1])
      {  // swap
         uint32_t t= cycle.lim[0];
         cycle.lim[0]= cycle.lim[1];
         cycle.lim[1]= t;
         m|= 0x1;
      }
      range= (cycle.lim[1] - cycle.lim[0]);
      if (0 == fsr.u32) { fsr.u32= cycle.lim[m&0x1]; }
      return(m);
   } // setF

   bool setDT (USciExp v[1])
   {
      uint32_t t= dt;
#ifdef USCI_DEFER
      dt= v[0].extract();
#else
      if (-3 == v[0].e) { dt= v[0].u;} // likely
      else if (v[0].e < -3) { return(false); }
      else { dt= v[0].u * uexp10(3+v[0].e); }
#endif
      return(dt != t);
   } // setDT

   int8_t setFT (USciExp v[], int8_t n)
   {
      int8_t r=0;
      if (n >= 2)
      {
         r= setF(v+0);
         if ((n >= 3) && setDT(v+2)) { r|= 0x4; }
      }
      return(r);
   } // set

public:
   DA_AD9833Sweep (void) { cycle.mode= CYCM_CMP_HI|CYCM_WRAP_HI; } // |CYCM_CONS_HI; }

   int8_t setParam (USciExp v[], int8_t n)
   {
      int8_t r= setFT(v,n);
      if ((r > 0) && (0 != range) && (dt > 0))
      {
         sLin= range / dt;
         //rPow= ((uint32_t)1<<24) * exp(log(lim[1]) - log(lim[0]) / dt);
         if (0 == rPow) { rPow= ((uint32_t)1<<24)+65536; } // test default
         logK();
         r|= 0x8;
      }
      return(r);
   } // setParam

   void stepFSR (uint8_t nStep, uint8_t mode)
   {
      switch(mode)
      {
         case 1 :
            if (1==nStep) { fsr.u32+= sLin; } else { fsr.u32+= nStep * sLin; }
            fsr.u32= cycle(fsr.u32);
            if (cycle.state & 0xA0) { sLin= -sLin; } // mirror
            break;
         case 2 :
         {  uint16_t s= 1 + (uint16_t)(fsr.u8[0]) + fsr.u8[1] + fsr.u8[2]; // super-hacky
            fsr.u32+= s;
            fsr.u32= cycle(fsr.u32);
            if (cycle.state & 0xA0) { rPow= ((uint32_t)1<<24) / rPow; } // mirror
            break;
         }
         default :
         {
            q52.u64= (uint64_t)fsr.u32 * rPow; // scale current by Q24 fraction giving 28.24 (52b) result
            for (uint8_t i=0; i<4; i++) { fsr.u8[i]= q52.u8[i+3]; } // shift down by 3 bytes/24bits
            fsr.u32= cycle(fsr.u32);
            if (cycle.state & 0xA0) { rPow= ((uint32_t)1<<24) / rPow; } // mirror
            break;
         }
      }
#if 0
      if (cycle.state)
      { 
         Serial.print("*SC: M=");
         Serial.print(cycle.mode,HEX);
         Serial.print(" S=");
         Serial.println(cycle.state,HEX);
      }
#endif
   } // stepFSR

   uint32_t getFSR (void) const { if (fsr.u32 > 0) { return(fsr.u32); } else return(12345); }

   void logK (Stream& s=Serial) const
   {
      //s.print("df="); s.print(range); s.print(" dt="); s.print(dt);
      s.print("\tsLin="); s.print(sLin); 
      s.print(" rPow="); s.print(rPow,HEX); 
      s.print(" f="); s.print(fsr.u32);
      s.print(" q="); s.print(q52.u32[1],HEX); s.print(":"); s.println(q52.u32[0],HEX);
   }
}; // DA_AD9833Sweep

#define FMMM 0x3 // function mode mask
#define FUGM 0x7 // frequency update (de-)glitch mask

// set/clear/flip mask byte
uint8_t scfByte (uint8_t b, uint8_t m, int8_t opr)
{
   if (opr < 0) { return(b ^ m); }
   else if (0 == opr) { return(b & ~m); }
   else { return(b | m); }
} // maskByte

// Top level API provides convenience and sanity (?) checking
class DA_AD9833Control
{
//protected:
public:
   DA_AD9833Reg   reg;
   DA_AD9833Sweep sweep;
   uint32_t fsr;  // backup for hold feature
   uint8_t rwm; // sweep function state, write mask for hw reg (16bits per flag)
   int8_t iFN;
   uint16_t hph;
   
   DA_AD9833Control (void) { iFN= 0; rwm= 0; } // Not reliably cleared by reset (?)
   
   int8_t waveform (int8_t w=0) // overwrite any sleep/hold setting
   {
static const U8 ctrlB0[]=
{
   0x00,  // sine wave
   AD9833_FL0_TRI, // triangular (symmetric)
   AD9833_FL0_OCLK|AD9833_FL0_SLP_DAC, // clock output (clock running, DAC off)
   AD9833_FL0_OCLK|AD9833_FL0_DCLK|AD9833_FL0_SLP_DAC // doubled "
};
      if ((w >= 0) && (w < 4))
      {  // lazy change if (ctrlB0[w] != reg.ctrl.u8[0]) { 
         reg.ctrl.u8[0]= ctrlB0[w];
         return(w);
      }
      return(-1);
   } // waveform

   int8_t mclock (int8_t scf=-1) // seems useless (output zero for all waveforms, hardware bug?)
   {  // master clock required in clock modes...
      if (0 == (reg.ctrl.u8[0] & AD9833_FL0_OCLK))
      {
         Serial.println("*K");
         reg.ctrl.u8[0]= scfByte(reg.ctrl.u8[0], AD9833_FL0_SLP_MCLK, scf);
         return((reg.ctrl.u8[0] & AD9833_FL0_SLP_MCLK) > 0);
      }
      return(-1);
   } // mclock

   int8_t hold (int8_t hf=-1)
   {  // hold voltage output - mclock(); broken so zero fsr registers
      if (hf < 0) { hf= reg.isZeroFSR(); }
      if (hf) { reg.setFSR(fsr); reg.setPSR(0); }
      else { reg.setZeroFSR(); reg.setPSR(0xFF); }
#if 0
      Serial.print("*H"); Serial.print(hf);
      Serial.print("fr="); Serial.print(reg.fpr[0].fr[1].u16,HEX);
      Serial.print(":"); Serial.println(reg.fpr[0].fr[0].u16,HEX);
#endif
      return(hf);
   } // hold

   int8_t onOff (int8_t scf=-1)
   {  // toggle MCLK & DAC together
      if (scf < 0)
      {  // any on -> off, all off -> all on
         scf= 0x1 ^ ((reg.ctrl.u8[0] & (AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_MCLK)) > 0);
      }
      reg.ctrl.u8[0]= scfByte(reg.ctrl.u8[0], AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_MCLK, scf);
      //if (0 == stateFlip) { reg.ctrl.u8[0] &= ~(AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_MCLK); } // all off,
      //else { reg.ctrl.u8[0] |= (AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_MCLK); } // all on
      return(scf);
   } // onOff

   void apply (CmdSeg& cs)
   {
      if (cs())
      {
         uint8_t nV= cs.getNV();

         if (cs.cmdF[0])
         {
            if (cs.cmdF[0] & 0x01)
            {
               iFN= ++iFN & FMMM;   // check if function parameters defined ???
               if (0 == iFN)
               {
                  reg.setFSR(fsr, 0);
                  rwm|= FUGM;
               }
               cs.cmdR[0]|= 0x01;
            }
            if (cs.cmdF[0] & 0x02) { cs.iRes= hold(); if (cs.iRes >= 0) { cs.cmdR[0]|= 0x02; rwm|= 0xF; iFN= -1; } } 
            if (cs.cmdF[0] & 0x04) { cs.iRes= mclock(); if (cs.iRes >= 0) { cs.cmdR[0]|= 0x04; } }
            if (cs.cmdF[0] & 0x08) { cs.iRes= onOff(); if (cs.iRes >= 0) { cs.cmdR[0]|= 0x08; } }
            if (cs.cmdF[0] & 0x10) { reg.ctrl.u8[1]|=  AD9833_FL1_RST; cs.cmdR[0]|= 0x10; rwm|= 0x81; }
            else if (cs.cmdR[0]) { rwm|= 0x10; }
         }
         if (cs.cmdF[1] & 0x0F)
         {
            if (waveform(cs.cmdF[1] & 0x3) >= 0) { cs.cmdR[1]|= cs.cmdF[1] & 0x0F; rwm|= 0x1; }
            //reg.ctrl.u8[1]|= AD9833_FL1_B28; // assume always set
         }
         if ((1 == nV) && (cs.v[0].nU > 0))
         {  // Set backup & shadow HW registers (sweep state remains independant)
            fsr= cs.v[0].toFSR();
            // if (0 == iFN) ???
            reg.setFSR(fsr, 0); rwm|= FUGM; // avoid phase discontinuity
            iFN= 0; // revert to simple function
         }
         else if (sweep.setParam(cs.v, nV) > 0)
         {
            reg.setFSR(sweep.getFSR(), 0); rwm|= FUGM; // no phase glitch when ctrl written
            if ((0 == iFN) && (0 == (cs.cmdR[0] & 0x01))) { iFN= 1; } // auto-on if off : defaultFN
         }

         if (iFN > 0) { iFN&= FMMM; }
         //changeMon(true);
      }
   } // apply

   void update (uint8_t nStep)
   {
      if (iFN > 0)
      {
         sweep.stepFSR(nStep, iFN);
         reg.setFSR(sweep.getFSR()); rwm|= FUGM; // Failing to write ctrl before freq causes phase discontinuity (internal reset?)
      }
      else if (iFN < 0)
      {
         reg.setPSR(++hph); rwm|= 0x8;
      }
   } // update

   void commit (void) { if (rwm > 0) { reg.write(rwm); rwm= 0; } }

   // 10.737418 * 16 = 171.79869
   uint32_t getF () const { return((reg.fpr[0].getFSR(2)<<2) / 172); }

   // Debug stuff
   uint8_t lastFN;
   void changeMon (bool force=false, const char *ids=NULL)
   {
      //if (iFN > 0) { sweep.logK(); }
      if (force || (iFN != lastFN))
      {
         if (ids) { Serial.print(ids); }
         Serial.print("*iFN="); 
         if (iFN == lastFN) { Serial.println(lastFN); }
         else
         {
            Serial.print(lastFN); Serial.print("->"); Serial.println(iFN);
            lastFN= iFN;
         }
         sweep.logK();
      }
   } // changeMon

   int8_t getModeCh (char mc[], int8_t max) const
   {
      if (reg.ctrl.u8[0] & AD9833_FL0_TRI) { mc[0]= 'T'; }
   } // getModeCh

}; // DA_AD9833Control

#endif // DA_AD9833_HW_HPP
