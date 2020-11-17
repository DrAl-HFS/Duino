// Duino/Common/DA_ad9833HW.hpp - Arduino-AVR specific class definitions for 
// Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Nov 2020

// HW Ref:  http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf

#ifndef DA_AD9833_HW_H
#define DA_AD9833_HW_H

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

// Classes are used because Arduino has limited support for modularisation and sanitary source reuse.
// Default build settings (minimise size) prevent default class-method-declaration inlining
class DA_AD9833FreqPhaseReg
{
public:
   UU16 fr[2], pr;
   
   DA_AD9833FreqPhaseReg  () {;} // should all be zero on reset
   
   void setFSR (const uint32_t fsr)
   {
      fr[0].u16=  AD_FS_MASK & fsr;
      fr[1].u16=  AD_FS_MASK & (fsr >> 14);
   } // setFSR
//=0
   uint32_t getFSR (uint8_t lsh) const
   {
      switch(lsh)
      {
         case 2 : return( (fr[0].u16 << 2) | (((uint32_t)fr[1].u16 & AD_FS_MASK) << 16) );
         default : return( (fr[0].u16 & AD_FS_MASK) | (((uint32_t)fr[1].u16 & AD_FS_MASK) << 14) );
      }
   } // getFSR

   void setPSR (const uint16_t psr) { pr.u16= AD_PS_MASK & psr; }
   
   // Masking of address bits. NB: assumes all zero! 
   
   void setFAddr (const uint8_t ia) // assert((ia & 0x1) == ia));
   {  // High (MS) bytes - odd numbered in little endian order
      fr[0].u8[1]|= ((ia+0x1)<<6);
      fr[1].u8[1]|= ((ia+0x1)<<6);
   } // setFAddr
   
   void setPAddr (const uint8_t ia) { pr.u8[1]|= ((0x6+ia)<<5); }
}; // class CDA_AD9833FreqPhaseReg

#include "Common/DA_Timing.hpp"

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

class DA_AD9833SPI : public CFastPollTimer
{
public:
   uint8_t dbgTransClk, dbgTransBytes;

   DA_AD9833SPI ()
   {
      SPI.begin();
#if (PIN_SEL != SS) // SS is already an output
      pinMode(PIN_SEL, OUTPUT);
#endif
      digitalWrite(PIN_SEL, HIGH);
   }
   void writeSeq (const uint8_t b[], const uint8_t n)
   {
      stamp(); // significant overheads in SPI - throughput 0.3~0.5 MByte/s
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
      dbgTransClk= diff();
      dbgTransBytes= n;
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
   
   DA_AD9833Reg () { ctrl.u8[1]= AD9833_FL1_B28|AD9833_FL1_RST; fpr[0].setPAddr(0); fpr[1].setPAddr(1); }
   
   void setFSR (uint32_t fsr, const uint8_t ia=0) { fpr[ia].setFSR(fsr); fpr[ia].setFAddr(ia); }
   void setPSR (const uint16_t psr, const uint8_t ia=0) { fpr[ia].setPSR(psr); fpr[ia].setPAddr(ia); }
   
   void write (uint8_t f, uint8_t c) { return writeSeq(b+(f<<1), c<<1); }
   
   void write (const uint8_t wm=0x7F)
   {
      uint8_t tm= wm & 0x7F;
      if (0x7F == tm) { writeSeq(b, sizeof(b)); }
      else
      {
         uint8_t first=0, count=0;
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

class DA_AD9833Sweep
{
protected:  // NB: fsr values in AD9833 native 28bit format (fixed point fraction of 25MHz)
   uint32_t fsrLim[2], dt; // sweep start&end (lo&hi?) limits, interval (millisecond ticks)
   int32_t range; // signed difference between end and start
   UU32 fsr;
   int32_t  sLin; // linear step
   uint32_t rPow; // power-law (growth) rate
   UU64 q52; // temp val
   
   // Primitive operations
   uint8_t setF (USciExp fv[2])
   {
      uint32_t t= fsrLim[0];
      uint8_t m=0;
      fsrLim[0]= fv[0].toFSR();
      m= (fsrLim[0] != t);
      fsrLim[1]= fv[1].toFSR();
      t= fsrLim[1];
      m|= (fsrLim[0] != t)<<1;
      range= (fsrLim[1] - fsrLim[0]);
      if (0 == fsr.u32) { fsr.u32= fsrLim[0]; }
      return(m);
   } // setF
   
   bool setDT (USciExp v[1])
   {
      uint32_t t= dt;
      if (-3 == v[0].e) { dt= v[0].u;} // likely
      else if (v[0].e < -3) { return(false); }
      else { dt= v[0].u * uexp10(3+v[0].e); }
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
   DA_AD9833Sweep () { ; }
   
   int8_t setParam (USciExp v[], int8_t n)
   {
      int8_t r= setFT(v,n);
      if ((r > 0) && (0 != range) && (dt > 0))
      {
         sLin= range / dt;
         //rPow= ((uint32_t)1<<24) * exp(log(fsrLim[1]) - log(fsrLim[0]) / dt);
         if (0 == rPow) { rPow= ((uint32_t)1<<24)+65536; } // test default
         logK();
         r|= 0x8;
      }
      return(r);
   } // setParam

   uint8_t stepFSR (uint8_t nStep, uint8_t mode)
   {
      uint8_t actState= 0; // action/state info
      switch(mode & 0xC0)
      {
         case 0x40 : if (1==nStep) { fsr.u32+= sLin; } else { fsr.u32+= nStep * sLin; } break;
         case 0x80 :
         {  uint16_t s= 1 + (uint16_t)(fsr.u8[0]) + fsr.u8[1] + fsr.u8[2]; // super-hacky
            fsr.u32+= s; break;
         }
         default :
         {
            q52.u64= (uint64_t)fsr.u32 * rPow; // scale current by Q24 fraction giving 28.24 (52b) result
            for (uint8_t i=0; i<4; i++) { fsr.u8[i]= q52.u8[i+3]; } // shift down by 3 bytes/24bits
            break;
         }
      }
      if (0 != (mode & 0x3F))
      {
         if ((mode & 0x1) && (fsr.u32 < fsrLim[0])) { actState|= 0x1; }
         if ((mode & 0x2) && (fsr.u32 > fsrLim[1])) { actState|= 0x2; }
         
         if ((actState >= 0x1) && (actState <= 0x2)) // ignore if limits not distinct
         {  // TODO : time-conserving wrap & mirror ?
            if (mode & 0x03)
            {  // wrap
               uint8_t mas= mode & actState;
               if (mas & 0x1) { fsr.u32= fsrLim[1]; actState|= 0x4; }
               if (mas & 0x2) { fsr.u32= fsrLim[0]; actState|= 0x8; }
            }
            if (mode & 0x30)
            {  // clamp
               uint8_t mas= swapHiLo4U8(mode) & actState;
               if (mas & 0x1) { fsr.u32= fsrLim[0]; actState|= 0x10; }
               if (mas & 0x2) { fsr.u32= fsrLim[1]; actState|= 0x20; }
            }
            if (mode & 0x0C)
            {  // mirror
               switch (mode & 0xC0)
               {
                  case 0x40 : sLin= -sLin; actState|= 0x40; break;
                  case 0x80 : rPow= (1<<24) / rPow; actState|= 0x80; break;
               }
            } // mirror
         }
      }
      return(actState);
   } // stepFSR
   
   uint32_t getFSR (void) const { if (fsr.u32 > 0) { return(fsr.u32); } else return(12345); }
   
   void logK (Stream& s=Serial) const
   {
      //s.print("df="); s.print(range); s.print(" dt="); s.print(dt);
      s.print(" sLin="); s.print(sLin); 
      s.print(" rPow="); s.print(rPow,HEX); 
      s.print(" f="); s.println(fsr.u32);
      s.print(" q="); s.print(q52.u32[1],HEX); s.print(":"); s.println(q52.u32[0],HEX);
   }
}; // DA_AD9833Sweep

#define FMMM 0x3

class DA_AD9833Control
{
public:
   DA_AD9833Reg   reg;
   DA_AD9833Sweep sweep;
   uint8_t iFN, rwm; // sweep function state, # 16bit register write mask
   
   DA_AD9833Control () { iFN= 0; } // : DA_AD9833Reg(), DA_AD9833Sweep();
   
   void apply (CmdSeg& cs)
   {
      if (cs())
      {
         uint8_t f= cs.cmdF[0];
         uint8_t nV= cs.getNV();
         
         if (f)
         {  //Serial.print("*f="); Serial.println(f,HEX);
            if (f & 0x10) { ++iFN; }
            if (f & 0x7)
            {
static const U8 ctrlB0[]=
{ 
   0x00,  // sine wave
   AD9833_FL0_TRI, // triangular (symmetric)
   AD9833_FL0_CLK|AD9833_FL0_SLP_DAC, // clock output (clock running, DAC off)
   AD9833_FL0_CLK|AD9833_FL0_DCLK|AD9833_FL0_SLP_DAC // doubled "
};
               uint8_t i= (f & 0x7) - 1; // lazy change
               if (ctrlB0[i] != reg.ctrl.u8[0]) { reg.ctrl.u8[0]= ctrlB0[i]; rwm|= 0x1; }
               //pR-ctrl.u8[1]|= AD9833_FL1_B28; // assume always set
            }
            if (f & 0x20)
            {  // leave DAC off in clock modes
               if (0 == (reg.ctrl.u8[0] & AD9833_FL0_CLK))
               { reg.ctrl.u8[0]^= AD9833_FL0_SLP_DAC; rwm|= 0x1; }
            } 
            if (f & 0x40) // on/off
            {  // toggle MCLK & DAC together
               if (reg.ctrl.u8[0] & (AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_CLK)) // any on -> 
               { reg.ctrl.u8[0] &= ~(AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_CLK); } // all off,
               else { reg.ctrl.u8[0] |= (AD9833_FL0_SLP_DAC|AD9833_SH0_SLP_CLK); } // all on
               rwm|= 0x1;
            }
            if (f & 0x80) { reg.ctrl.u8[1]|=  AD9833_FL1_RST; rwm|= 0x81; }
         }
         if ((1 == nV) && (cs.v[0].u > 0))
         {  // Set shadow HW registers directly, leaving sweep state untouched
            reg.setFSR(cs.v[0].toFSR(), 0); rwm|= 0x7; // no phase glitch when ctrl written
            iFN= 0; // disable sweep functions
         }
         else if (sweep.setParam(cs.v, nV) > 0)
         {
            reg.setFSR(sweep.getFSR(), 0); rwm|= 0x7; // no phase glitch when ctrl written
            if ((0 == iFN) && (0 == (f & 0x10))) { iFN= 1; } // defaultFN
         }

         cs.clean();
         iFN&= FMMM;
         //changeMon(true);
      }
   } // apply
   
   void sweepStep (uint8_t nStep)
   {
      uint8_t m=0;
      switch(iFN)
      {
         case 0 : return;
         case 1 : m= 0x42; break; // linear, wrap hi
         case 2 : m= 0x82; break; // power "
      }
      m= sweep.stepFSR(nStep, m);
      reg.setFSR(sweep.getFSR()); rwm|= 0x7; // Failing to write ctrl before freq causes phase discontinuity (internal reset?)
   } // sweepStep
   
   uint8_t resetPending (void) { return(reg.ctrl.u8[1] & AD9833_FL1_RST); }
   bool resetClear (void)
   {
      if (resetPending())
      {
         reg.ctrl.u8[1]^= AD9833_FL1_RST; rwm|= 0x1;
         return(true);
      }
      return(false);
   } // resetClear
   
   void commit (void) { if (rwm > 0) { reg.write(rwm); rwm= 0; } }
   
   // 10.737418 * 16 = 171.79869
   uint32_t getF () const { return((reg.fpr[0].getFSR(2)<<2) / 172); }

   // Debug stuff
   uint8_t lastFN;
   void changeMon (bool force=false, const char *ids=NULL)
   {
      if (iFN > 0) { sweep.logK(); }
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
      }
   }
   
   int8_t getModeCh (char mc[], int8_t max) const
   {
      if (reg.ctrl.u8[0] & AD9833_FL0_TRI) { mc[0]= 'T'; }
   }
}; // DA_AD9833Control

#endif // DA_AD9833_HW_H
