// Duino/SGAD9833/DA_ad9833Mgt.hpp - Arduino-AVR specific utility classes for
// managing Analog Devices AD9833 signal generator with SPI/3-wire compatible interface.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Dec 2020

#ifndef DA_AD9833_MGT_HPP
#define DA_AD9833_MGT_HPP

#include "DA_ad9833HW.hpp"


/***/

// Cycle mode flags
#define CYCM_CMP_HI  0x08
#define CYCM_CONS_HI 0x04
#define CYCM_CMP_LO  0x02
#define CYCM_CONS_LO 0x01
#define CYCM_MIRR_HI 0x80
#define CYCM_WRAP_HI 0x40
#define CYCM_MIRR_LO 0x20
#define CYCM_WRAP_LO 0x10

// Class implementing rudimentary cyclic animation modes: wrap / mirror
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

// Frequency sweep generator. Linear mode works resonably well, 
// but power law (log/exp) presents computational problem due
// to slow floating point emulation
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
   uint8_t setF (CNumBCDX fv[2])
   {
      uint8_t m=0;
      cycle.lim[0]= toFSR(fv[0]);
      cycle.lim[1]= toFSR(fv[1]);
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

   bool setDT (CNumBCDX v[1])
   {
      uint32_t t= dt;
      dt= v[0].extractScale(); // NB: implicit scaling to ms
      return(dt != t);
   } // setDT

   int8_t setFT (CNumBCDX v[], int8_t n)
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

   int8_t setParam (CNumBCDX v[], int8_t n)
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
   uint16_t hph; // hacky hold phase

   DA_AD9833Control (void) { iFN= 0; rwm= 0; } // reg.setGain(0x80); } // Not reliably cleared by reset (?)

   void setGain (uint8_t v) { reg.setGain(v); }
   
   int8_t waveform (int8_t w=0) // overwrite any sleep/hold setting
   {
static const U8 ctrlB0[]=
{
   0x00,  // sine wave
   AD9833_FL0_TRI, // triangular (symmetric)
   AD9833_FL0_OCLK|AD9833_FL0_SLP_DAC, // half-rate clock output (clock running, DAC off)
   AD9833_FL0_OCLK|AD9833_FL0_FCLK|AD9833_FL0_SLP_DAC // full-rate clock output "
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
         reg.ctrl.u8[0]= scfByte(reg.ctrl.u8[0], AD9833_FL0_SLP_MCLK, scf);
         return((reg.ctrl.u8[0] & AD9833_FL0_SLP_MCLK) > 0);
      }
      return(-1);
   } // mclock

   int8_t hold (int8_t hf=-1)
   {  // hold voltage output - mclock(); broken so zero fsr registers
      if (hf < 0) { hf= reg.isZeroFSR(); }
      if (hf) { reg.setFSR(fsr); reg.setPSR(0); }
      else { reg.setZeroFSR(); } // iFN=-1; reg.setPSR(0xFF); }
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
            if (cs.cmdF[0] & 0x04) { cs.iRes= onOff(); if (cs.iRes >= 0) { cs.cmdR[0]|= 0x08; } }
            if (cs.cmdF[0] & 0x08) { reg.ctrl.u8[1]|=  AD9833_FL1_RST; cs.cmdR[0]|= 0x04; rwm|= 0x81; }
            //if (cs.cmdF[0] & 0x10) { cs.iRes= mclock(); if (cs.iRes >= 0) { cs.cmdR[0]|= 0x10; } }
            //else if (cs.cmdR[0]) { rwm|= 0x10; }
         }
         if (cs.cmdF[1] & 0x0F)
         {
            if (waveform(cs.cmdF[1] & 0x3) >= 0) { cs.cmdR[1]|= cs.cmdF[1] & 0x0F; rwm|= 0x1; }
            //reg.ctrl.u8[1]|= AD9833_FL1_B28; // assume always set
         }
         if ((1 == nV) && cs.v[0].isNum())
         {  // Set backup & shadow HW registers (sweep state remains independant)
            fsr= toFSR(cs.v[0]);
            // if (0 == iFN) ???
            reg.setFSR(fsr, 0); rwm|= FUGM; // avoid phase discontinuity
            iFN= 0; // revert to simple function
         }
         else if (sweep.setParam(cs.v, nV) > 0)
         {
            reg.setFSR(sweep.getFSR(), 0); rwm|= FUGM; // no phase glitch when ctrl written
            if ((0 == iFN) && (0 == (cs.cmdR[0] & 0x01))) { iFN= 2; } // auto-on if off : defaultFN
         }  // Why mode 2 ? mode 1 seems broken...

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
      {  // hacky test...
         reg.setPSR(hph+= nStep); rwm|= 0x8;
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

#endif // DA_AD9833_MGT_HPP
