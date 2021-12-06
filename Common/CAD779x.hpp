// Duino/Common/CAD779x.hpp - AD779x precision ADC with SPI interface utility classes
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Nov 2021

#ifndef CAD779x_HPP
#define CAD779x_HPP

#include <SPI.h>
#include "MBD/mbdDef.h" // UU16/32

#ifndef HSPI
#define HSPI SPI
#endif
#ifndef PIN_NCS
#define PIN_NCS SS // Uno=10,Mega=53
#endif
#ifndef PIN_NRDY
#define PIN_NRDY MISO // Uno=12,Mega=50
#endif

// NB: must follow pin defs
#include "CCommonSPI.hpp"

/*
#define _STR(x) #x
#define STR(x) _STR(x)
#pragma message "PIN_NRDY=" STR(MISO)
*/
#if 0
#ifdef ARDUINO_ARCH_AVR
// NB: cant use pin values due to preprocessor definition problems 
#ifdef ARDUINO_AVR_UNO
#define NRDY_MPB 0x10  // PB4
#endif
#ifdef ARDUINO_AVR_MEGA2560
#define NRDY_MPB 0x08  // PB3
#endif
#endif
#endif

// Namespace useful for wrapping common token names such as <DATA>, <IDLE> etc.
// The resulting verbosity of code is not always convenient but the specificity
// helps avoid tedious debugging of arcane problems.
namespace AD779x  // Caveat: namespace verbosity...
{
   enum Reg : int8_t { COMMSTAT, MODE, CONF, DATA, ID, IO, OFFS, FS };
   enum Stat : uint8_t { NRDY= 0x80, ERR=0x40, IDB=0x08, CHM=0x07  };
   // mode MSB
   enum Mode : int8_t { CONTINUOUS, SINGLE, IDLE, POWER_DOWN, CAL_IZS, CAL_IFS, CAL_XZS, CAL_XFS}; // <<5
   // mode LSB
   enum Clock : int8_t { CLK_INT, CLK_INT_OUT, CLK_EXT, CLK_EXT_DIV2 }; // <<6
   enum Rate : int8_t {
      ROFF,
      R470, // limited to ~18b(?)  - raw (no rejection filter for 50&60Hz noise)
      R242, // fastest 24b           "
      R123, R62, R50, R39, R33_2, // "
      R19_6, // 90dB rejection 60Hz noise
      R16_7A, // 80dB rejection 50Hz noise
      R16_7B, R12_5, R10, R8_33, R6_25, R4_17, RM=0xF }; // 65~74dB rejection 50&60Hz noise
   // conf MSB
   enum Bias : int8_t { BIAS_OFF, BIAS_A1, BIAS_A2 }; // <<6
   enum Flag : uint8_t { BURNOUT=0x20, UNIPOLAR=0x10, BOOST=0x08 };
   enum Gain : int8_t { G1, G2, G4, G8, G16, G32, G64, G128, GAIN_MASK=0x7 };
   // conf LSB
   enum VRef : uint8_t { VREF_EXT=0, BUF=0x10, VREF_INT=0x80 }; // unbuffered
   enum Chan : int8_t { D1, D2, D3, S1, THERM=6, AVDD, CHAN_MASK=0x07 };
   // IO register
   enum EXCD : int8_t { STRAIGHT, SWAP, DUAL1, DUAL2 }; // <<2 Excitation current source destination ("direction" ?!?)
   enum EXCI : int8_t { I0, I10, I210, I1000 }; // Excitation current 10uA ~ 1mA


   struct RateClock
   {
      uint8_t m0;//, mm0;
//
      RateClock (Rate r=R16_7A, Clock c=CLK_INT) { m0= (c<<6)|r; }

      //void set (ShadowReg& sr) { sr.mode.u8[0]= (sr.mode.u8[0] & ~mm0) | (m0 & mm0); }
   };

   struct OperMode
   {
      uint8_t m1;

      OperMode (Mode m=AD779x::CONTINUOUS) { m1= m<<5; }

      //void set (ShadowReg& sr) { sr.mode.u8[1]= m1; }
   };

   struct ChanRef
   {
      uint8_t c0;//, cm0;

      ChanRef (Chan c=AVDD, VRef r=VREF_INT) { c0= r|c; }

      //void set (ShadowReg& sr) { sr.conf.u8[0]= (sr.conf.u8[0] & ~cm0) | (c0 & cm0); }
   };

   struct GainFlagBias
   {
      uint8_t c1;//, cm1;

      GainFlagBias (Gain g=G1, Flag f=UNIPOLAR, Bias b=BIAS_OFF) { c1= (b<<6)|f|g; }

      //void set (ShadowReg& sr) { sr.conf.u8[1]= (sr.conf.u8[1] & ~cm1) | (c1 & cm1); }
   };
   
   // Use shadowing for important registers to
   // reduce communication overhead (& noise)
   struct ShadowReg
   {
      UU16  mode, conf;
      U8    stat;
      // unsatisfactory..
      operator = (const RateClock& rc) { mode.u8[0]= rc.m0; }
      operator = (const OperMode& om) { mode.u8[1]= om.m1; }
      operator = (const ChanRef& cr) { conf.u8[0]= cr.c0; }
      operator = (const GainFlagBias& gfb) { conf.u8[1]= gfb.c1; }
   };

   const float internalRefV= 1.17f;
   const long fsr= (long)1<<24; // HACK! not for double ended, AD7792 etc.
}; // namespace AD779x

// TODO: determine suitable "using AD779x::" scheme

// Low-level signalling using MISO line seems unreliable.
// Both HW pull-up (very weak ~100K) and INPUT_PULLUP (AVR) break SPI.
// Try filtering.. ?
class CAD779xSignal
{
public:
   CAD779xSignal (void) { ; }
#ifdef NRDY_MPB
   uint8_t rdy (void) { return(0x0 == (NRDY_MPB & PORTB)); } // digitalReadFast(PIN_NRDY)
#else
   uint8_t rdy (void) { return(0 == digitalRead(PIN_NRDY)); }
#endif

   uint8_t rdy (uint8_t n)
   {
      uint8_t r= rdy();
      while (n-- > 1) { r= (r<<1) | rdy(); } 
      return(r);
   } // rdy

   bool ready (const uint8_t n=3, int8_t t=3)
   {  // noise filter 
      const uint8_t m= (1 << n) - 1;
      uint8_t r= rdy(n);
      while (r > 0)
      {
         if (m == (r & m)) { return(true); }
         if (--t <= 0) { return(false); }
         // another sample
         r= (r << 1) | rdy();
      }
      return(false);
   } // ready

}; // CAD779xSignal

// IO sub-class deals with SPI (pretty simple using 'Duino libraries).
// Note that big-endian conversion is implicit here (AVR & ARM (almost) always little endian).
// Could be replaced with "bare metal" hacking approach if required.
class CAD779xSPI : CCommonSPI, public CAD779xSignal
{

public:
   CAD779xSPI (uint8_t clkM=0) : hsm{0} { init(clkM); }

   void init (uint8_t clkM)
   {
      if (hsm & 1) { close(); }
      if (clkM > 0)
      {
         spiSet= SPISettings(clkM*1000000, MSBFIRST, SPI_MODE3);
         HSPI.begin();
         pinMode(PIN_NCS, OUTPUT);
         digitalWrite(PIN_NCS,1);
         hsm= 0x1;
      }
   } // init

   void close (void) { HSPI.end(); hsm= 0x00; }

   void reset (void) // reliability ?
   {
      start();
      writeb(0xFF,4);
      complete();
      hsm|= 0x20;
   } // reset

protected:
   uint8_t hsm; // hacky state mask

   int8_t readReg (AD779x::Reg r, uint8_t vB[], const uint8_t nB)
   {
      start();
      HSPI.transfer( (0x8 | r) << 3); // First byte to COMM reg defines read target register
      readRev(vB,nB);
      complete();
      return(nB);
   } // readReg

   int8_t writeReg (AD779x::Reg r, const uint8_t vB[], const uint8_t nB)
   {
      start();
      HSPI.transfer(r << 3); // First byte to COMM reg defines write target register
      writeRev(vB,nB);
      complete();
      return(nB);
   } // writeReg

}; // CAD779xSPI


// Next the raw utility layer, providing interface to registers, ordinal & flag values
class CAD779xRaw : public CAD779xSPI
{
public:
   CAD779xRaw (uint8_t clkM) : CAD779xSPI(clkM) { ; }

   bool ready (void) // ~3us @ 8MHz sclk, potentially noisy
   {
      readReg(AD779x::COMMSTAT, &(sr.stat), 1);
      return(0 == (sr.stat & AD779x::NRDY));
   }

   uint32_t read24b (AD779x::Reg r=AD779x::DATA)
   {
      UU32 v;
      readReg(r, v.u8, 3);
      v.u8[3]= 0;
      return(v.u32);
   } // read24b

protected:
   AD779x::ShadowReg sr;

   uint8_t getID (void)
   {
      uint8_t id=0xFF;
      readReg(AD779x::ID, &id, 1);
      return(id);
   } // getID
   
   void setRate (AD779x::RateClock rc)
   {
      sr= rc; // doesn't look right...
      writeReg(AD779x::MODE, sr.mode.u8, 2);
   } // setMode

   void setMode (AD779x::RateClock rc, AD779x::OperMode om)
   {
      sr= rc; // doesn't look right...
      sr= om;
      writeReg(AD779x::MODE, sr.mode.u8, 2);
   } // setMode

   void setChan (AD779x::ChanRef cr)
   {
      sr= cr;
      writeReg(AD779x::CONF, sr.conf.u8, 2);
   }

   void setConf (AD779x::ChanRef cr, AD779x::GainFlagBias gfb)
   {
      sr= cr;
      sr= gfb;

      writeReg(AD779x::CONF, sr.conf.u8, 2);
   } // setConf

   void setDefault (void)
   {
      setConf( AD779x::ChanRef(), AD779x::GainFlagBias() );
      setMode( AD779x::RateClock(), AD779x::OperMode() );
      hsm|= 0x40;
   } // setDefault

}; // CAD779xRaw

// Next layer turns (hardware) encoded data into more practically useful measures & values
class CAD779xUtil : public CAD779xRaw
{
public:
   CAD779xUtil (uint8_t clkM) : CAD779xRaw(clkM) { ; }

   int8_t identify (uint8_t *rawID)
   {
      uint8_t id= getID();
      int8_t i= ((id & 0xF) - 0x8);
      if ((i >= 2) && (i <= 3)) { hsm|= 0x80; } else { i= -1; }
      //CCommonState::setHW(Device::HW::UNKNOWN);
      //if (CCommonState::getHW() < Device::HW::STANDBY) { CCommonState::setHW(Device::HW::STANDBY); }
      if (rawID) { *rawID= id; }
      return(i);
   } // identify

   //void setChan (AD779x::Chan c) { setConf(AD779x::ChanRef(c), AD779x::GainFlagBias()); } //HACK!
   
   AD779x::Chan chan (bool refresh=false)
   {
      if (refresh) { readReg(AD779x::CONF, sr.conf.u8, 2); }
      return(sr.conf.u8[0] & AD779x::CHM);
   }

   AD779x::Rate rate (bool refresh=false)
   {
      if (refresh) { readReg(AD779x::MODE, sr.mode.u8, 2); }
      return(sr.mode.u8[0] & AD779x::RM);
   }

   uint8_t gain (void) { return(1 << (sr.conf.u8[1] & AD779x::GAIN_MASK)); }

   // raw reading to Volts
   float getV (uint32_t v)
   {
      float V= (AD779x::internalRefV / AD779x::fsr) * v; // single ended HACK
      switch(sr.stat & AD779x::CHM)
      {
         case AD779x::THERM : V*= 100; break;   // norm to Celcius ?
         case AD779x::AVDD :  V*= 6; break;     // 1:5 divider
         default :            V /= gain();
      }
      return(V);
   } // getV

}; // CAD779xUtil

#define SHSB   4
#define NHSB   (1<<SHSB)
#define MHSB   (NHSB-1)
class CHackStat32
{
public:
   uint32_t xt[NHSB], sum, idx, mean;

   CHackStat32 (void) { clear(); }

   void clear (void) { mean= 0; sum= 0; idx= 0; for (int8_t i=0; i<NHSB; i++) { xt[i]= 0; } }
   
   void add (uint32_t x)
   {
      sum+= x - xt[MHSB & idx];
      xt[MHSB & idx]= x;
      if (0 == ++idx) { idx|= NHSB; }
   } // add

   uint32_t getMean (void)
   {
      if (idx >= NHSB) { return(sum >> SHSB); }//else
      if (idx > 1) { return(sum / idx); }//else
      return(sum);
   } // getMean

   uint32_t updateMean (void) { return(mean= getMean()); }

// At 242Hz sample rate reveals oscillation:
// period ~5 samples, peak amplitude ~13b
// clearly suggests 50Hz noise...
   void dumpMDiff (Stream& s)
   {
      //const char sep[]={',','\n'};
      int8_t m; if (idx < NHSB) { m= idx; } else { m= NHSB; } // C++ min() broken?
      s.print("M:"); 
      s.print(mean);
      s.print(" D:"); 
      for (int8_t i=0; i<m; i++)
      {
         s.print(','); s.print((int)(xt[i]-mean));
         // s.print(sep[i==(m-1)]);
      }
      s.println();
   } // dumpMDiff

}; // CHackStat32

// Data dump layer for debug support
class CAD779xDbg : public CAD779xUtil, public CHackStat32
{
   uint16_t rl[4];
   char chid;

public:
   CAD779xDbg (uint8_t clkM=8) : CAD779xUtil(clkM), chid{'x'} { clear(); }

   void setChan (AD779x::Chan c)
   {
      if (c != chan())
      {
         clear();
         CHackStat32::clear();
         CAD779xRaw::setChan(c);
      }
   }
   bool ready (void) { return(0xC0 == (hsm & 0xC0)); }

   bool test (Stream& s)
   {
      if (0x10 == (hsm & 0xF0)) { hsm^= 0x10; return(false); }
      if (hsm & 0x20) { hsm^= 0x20; return(false); }
      if (0x80 == (hsm & 0xC0)) { setDefault(); }
      if (0 == (hsm & 0x80))
      {
         if (!logID(s)) { reset(); hsm|= 0x10; }
      }
      if (hsm & 0x80)
      {
         logStat(s);
         logIO(s);
         logMode(s);
         logConf(s);
         logCal(s);
         if (ready()) { logData(s); }
         // Hacky...
         return(true);
      }
      return(false);
   } // test
   
   void clear (void) { for (int8_t i=0; i<4; i++) { rl[i]= 0; } }
   
   // CAD779xSignal::ready() needed to work ?why?
   void sample (void)
   {
      if (ready())
      {
         uint8_t rm= CAD779xSignal::ready() | (CAD779xRaw::ready() << 1);
         rl[rm]++;
         if (0x3 == rm) { add(read24b()); }
      }
   }
   
   void report (Stream& s)
   {
      //if (ior > 0)
      {
         s.print("rl:"); s.print(rl[0]); for (int8_t i=1; i<4; i++) { s.print(','); s.print(rl[i]); } s.print(';');
         printChan(s,chan()," "); printRate(s,rate()," ");
         s.print("M["); s.print(idx); s.print("]="); s.println(getV(updateMean()),6);
         dumpMDiff(s);
      }
   } // report

   void printDevID (Stream& s, const char *end=": ")
   {
      s.print("AD779"); s.print(chid); if (end) { s.print(end); }
   } // printDevID

   void printChan (Stream& s, const uint8_t chan, const char *end="\n")
   {
      s.print(" CH:");
      switch(chan)
      {
         case AD779x::THERM:  s.print("THERM"); break;
         case AD779x::AVDD :  s.print("AVDD"); break;
         case AD779x::S1   :  s.print("S1"); break;
         default : s.print('D'); s.print(1+chan); break;
      }
      if (end) { s.print(end); }
   } // printChan

   void printRate (Stream& s, const uint8_t rate, const char *end="\n")
   {
      s.print(" R:");
      switch(rate) // AD779x::R12_5, AD779x::R50, AD779x::R123, AD779x::R470
      {
         case AD779x::R12_5:  s.print("12.5"); break;
         case AD779x::R50:  s.print("50"); break;
         case AD779x::R123:  s.print("123"); break;
         case AD779x::R470:  s.print("470"); break;
      }
      if (end) { s.print(end); }
   } // printRate
   
   bool logID (Stream& s)
   {
static const char *msg[]= {"UNKNOWN","OK"};
      uint8_t rawID;
      int8_t i= identify(&rawID);
      if (i >= 0) { chid='0'+i; }
      printDevID(s);
      i=  (i >= 0);
      s.print("RAW="); s.print(rawID,HEX); s.print(" - ");
      s.println(msg[i]);
      return(i);
   } // logID

   void logStat (Stream& s)
   {
      readReg(AD779x::COMMSTAT, &(sr.stat), 1);

      printDevID(s);
      s.print("STAT=");
      if (0 == (sr.stat & AD779x::NRDY)) { s.print('R'); }
      if (sr.stat & AD779x::ERR) { s.print('E'); }
      printChan(s, sr.stat & AD779x::CHM);
   } // logStat

   void logIO (Stream& s)
   {
      uint8_t io;
      readReg(AD779x::IO, &io, 1);
      printDevID(s);
      s.print("IO= 0x"); s.println(io,HEX);
      //if (io & 0x80) { s.print('R'); }
   } // logStat

   void logMode (Stream& s)
   {
      readReg(AD779x::MODE, sr.mode.u8, 2);
      printDevID(s);
      s.print("MODE= M#"); s.print(sr.mode.u8[1]>>5);
      s.print(" C#"); s.print(sr.mode.u8[0]>>6);
      s.print(" R#"); s.println(sr.mode.u8[0] & AD779x::RM);
   } // logMode

   void logConf (Stream& s)
   {
      readReg(AD779x::CONF, sr.conf.u8, 2);
      printDevID(s);
      s.print("CONF= VB#"); s.print(sr.conf.u8[1] >> 6); s.print(' ');
      if (sr.conf.u8[1] & AD779x::BURNOUT) { s.print('R'); }
      if (sr.conf.u8[1] & AD779x::UNIPOLAR) { s.print('U'); }
      if (sr.conf.u8[1] & AD779x::BOOST) { s.print('O'); }
      s.print(" G"); s.print(gain()); s.print(' ');

      if (sr.conf.u8[0] & AD779x::VREF_INT) { s.print('I'); } else { s.print('X'); }
      if (sr.conf.u8[0] & AD779x::BUF) { s.print('B'); }
      printChan(s, sr.conf.u8[0] & AD779x::CHM);
   } // logConf

   void logCal (Stream& s)
   {
      uint32_t o, f;
      o= read24b(AD779x::OFFS);
      s.print("OFFS="); s.println(o);
      f= read24b(AD779x::FS);
      s.print("FS="); s.println(f);
   } // logCal

   void logData (Stream& s)
   {
      uint32_t v= read24b(); // very noisy ~10 lsb
      s.print("DATA= 0x"); s.print(v,HEX);
      s.print(','); s.print(v);
      s.print(','); s.println(getV(v),4);
   } // logData

   void debugCycle (uint8_t c)
   {
      const AD779x::Rate rm[]= {AD779x::R12_5, AD779x::R50, AD779x::R123, AD779x::R470};
      const AD779x::Chan cm[]= {AD779x::AVDD, AD779x::THERM, AD779x::S1, AD779x::D1};
      setRate( rm[ c & 0x3 ] );
      setChan( cm[ (c >> 2) & 0x3 ] );
   } // debugCycle

}; // CAD779xDbg

#endif // CAD779x_HPP


