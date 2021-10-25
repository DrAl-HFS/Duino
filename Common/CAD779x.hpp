// Duino/Common/CAD779x.hpp - AD779x precision ADC with SPI interface utility classes
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#include <SPI.h>


#include "MBD/mbdDef.h" // UU16/32

#ifndef NCS
#define NCS SS //=10
#endif

// Namespace useful for wrapping common token names such as <DATA>, <IDLE> etc.
// The resulting verbosity of code is not always convenient but the specificity
// helps avoid tedious debugging of arcane problems.
namespace AD779x  // Caveat: namespace verbosity...
{
   typedef enum Reg : int8_t { COMMSTAT, MODE, CONF, DATA, ID, IO, OFFS, FS };
   enum Stat : uint8_t { NRDY= 0x80, ERR=0x40, IDB=0x08, CHM=0x07  };
   // mode MSB
   enum Mode : int8_t { CONTINUOUS, SINGLE, IDLE, POWER_DOWN, CAL_IZS, CAL_IFS, CAL_XZS, CAL_XFS}; // <<5
   // mode LSB
   enum Clock : int8_t { CLK_INT, CLK_INT_OUT, CLK_EXT, CLK_EXT_DIV2 }; // <<6
   enum Rate : int8_t { ROFF, R470, R242, R123, R62, R50, R39, R33_2, // raw (no rejection filter for 50&60Hz noise)
                  R19_6, // 90dB rejection 60Hz noise
                  R16_7A, // 80dB rejection 50Hz noise
                  R16_7B, R12_5, R10, R8_33, R6_25, R4_17, RM=0xF }; // 65~74dB rejection 50&60Hz noise
   // conf MSB
   enum Bias : int8_t { BIAS_OFF, BIAS_A1, BIAS_A2 }; // <<6
   enum Flag : uint8_t { BURNOUT=0x20, UNIPOLAR=0x10, BOOST=0x08 };
   enum Gain : int8_t { G1, G2, G4, G8, G16, G32, G64, G128, GM=0x7 };
   // conf LSB
   enum VRef : uint8_t { VREF_EXT=0, BUF=0x10, VREF_INT=0x80 }; // unbuffered
   enum Chan : int8_t { D1, D2, D3, S1, THERM=6, AVDD }; // CHM=0x07
   // IO register
   enum EXCD : int8_t { STRAIGHT, SWAP, DUAL1, DUAL2 }; // <<2 Excitation current source destination ("direction" ?!?)
   enum EXCI : int8_t { I0, I10, I210, I1000 }; // Excitation current 10uA ~ 1mA

   // Use shadowing for important registers to
   // reduce communication overhead (& noise)
   typedef struct
   {
      UU16  mode, conf;
      U8    stat;
   } ShadowReg;

   const float internalRefV= 1.17f;
   const long fsr= (long)1<<24; // HACK! not for double ended, AD7792 etc.
}; // namespace AD779x


//using namespace AD779x {

// IO sub-class deals with SPI (pretty simple using 'Duino libraries).
// Note that big-endian conversion is implicit here (AVR & ARM (almost) always little endian).
// Could be replaced with "bare metal" hacking approach if required.
class CAD779xIO
{
   SPISettings set;

public:
   CAD779xIO (uint32_t clk) : m{0x1} { SPI.begin(); set= SPISettings(clk, MSBFIRST, SPI_MODE3); }

   //void init (clk) { if (m & 1) { SPI.end(); } set= SPISettings(clk, MSBFIRST, SPI_MODE3); SPI.begin(); m= 0x1; }
   void close (void) { SPI.end(); m= 0x00; }

   void reset (void) // not reliable.. ?
   {
      start();
      for (int8_t iB=0; iB<4; iB++) { SPI.transfer(0xFF); }
      complete();
      m|= 0x20;
   } // reset

protected:
   uint8_t m; // hacky state variable

   void start (void) { SPI.beginTransaction(set); digitalWrite(NCS,0); }
   void complete (void) { SPI.endTransaction(); digitalWrite(NCS,1); }

   int8_t readR (AD779x::Reg r, uint8_t vB[], const uint8_t nB)
   {
      start();
      SPI.transfer( (0x8 | r) << 3); // First byte to COMM reg defines read target register
      //for (int8_t iB=0; iB<nB; iB++) // preserve big endian byte order
      int8_t iB= nB; // big to little endian byte order
      while (iB-- > 0) { vB[iB]= SPI.transfer(0xFF); }
      complete();
      return(nB);
   } // readR
   
   int8_t writeR (AD779x::Reg r, const uint8_t vB[], const uint8_t nB)
   {
      start();
      SPI.transfer(r << 3); // First byte to COMM reg defines write target register
      //for (int8_t iB=0; iB<nB; iB++) // preserve big endian byte order
      int8_t iB= nB; // little to big endian byte order
      while (iB-- > 0) { SPI.transfer(vB[iB]); }
      complete();
      return(nB);
   } // writeR
   
}; // CAD779xIO

// Next the raw utility layer, providing interface to registers, ordinal & flag values
class CAD779xRaw : public CAD779xIO
{
public:
   CAD779xRaw (uint32_t clk) : CAD779xIO(clk) { ; }

   bool dataReady (void)
   {
      readR(AD779x::COMMSTAT, &(sr.stat), 1);
      return(0 == (sr.stat & AD779x::NRDY));
   }

   uint32_t read24b (AD779x::Reg r=AD779x::DATA)
   {
      UU32 v;
      readR(r, v.u8, 3);
      v.u8[3]= 0;
      return(v.u32);
   } // readData

protected:
   AD779x::ShadowReg sr;

   int8_t checkID (void) //  return 0 or 1 if identified as AD779x, otherwise -1
   {
      uint8_t id=0x5F;
      readR(AD779x::ID, &id, 1);
      uint8_t t= ((id & 0xF) - 0xA);
      if (t <= 1) { m|= 0x80; return(t); }
      return(-1);
   }

   //using AD779x:: ???

   void setConf (AD779x::Bias b=AD779x::BIAS_OFF, AD779x::Flag f=AD779x::UNIPOLAR, AD779x::Gain g=AD779x::G1, AD779x::VRef r=AD779x::VREF_INT, AD779x::Chan c=AD779x::THERM)
   {
      sr.conf.u8[1]= (b<<6)|f|g;
      sr.conf.u8[0]= r|c;
      
      writeR(AD779x::CONF, sr.conf.u8, 2);
   } // setConf

   void setMode (AD779x::Mode m=AD779x::CONTINUOUS, AD779x::Clock c=AD779x::CLK_INT_OUT, AD779x::Rate r=AD779x::R16_7A)
   {
      sr.mode.u8[1]= m<<5;
      sr.mode.u8[0]= (c<<6)|r;
      writeR(AD779x::MODE, sr.mode.u8, 2);
   } // setMode

   void setIO (AD779x::EXCD d=AD779x::STRAIGHT,  AD779x::EXCI i=AD779x::I0)
   {
      uint8_t io= (d<<2) | i;
      writeR(AD779x::IO, &io, 1);
   } // setIO

   void setDefault (void)
   {
      setConf();
      setMode();
      m|= 0x40;
   }

}; // CAD779xRaw

// Next layer turns (hardware) encoded data into more practically useful measures & values
class CAD779xUtil : public CAD779xRaw
{
public:
   CAD779xUtil (uint32_t clk=1E6) : CAD779xRaw(clk) { ; }

   uint8_t gain (void) { return(1 << (sr.conf.u8[1] & AD779x::GM)); }
   uint32_t gain (uint32_t v) { return(v << (sr.conf.u8[1] & AD779x::GM)); } // trade mult for shift 
   
   // raw reading to Volts
   float getV (uint32_t v)
   {
      float V= (AD779x::internalRefV / AD779x::fsr) * gain(v); // single ended HACK
      switch(sr.stat & AD779x::CHM)
      { 
         case AD779x::THERM : V*= 100; break;   // norm to Celcius ?
         case AD779x::AVDD :  V*= 6; break;     // 1:5 divider
      }
      return(V);
   }
   
}; // CAD779xUtil

// Data dump layer for debug support
class CAD779xDbg : public CAD779xUtil
{
public:
   CAD779xDbg (uint32_t clk=8E6) : CAD779xUtil(clk) { ; }

   // Displace to intermediate interface
   
   bool test (Stream& s)
   {
      if (0x10 == (m & 0xF0)) { m^= 0x10; return(false); }
      if (m & 0x20) { m^= 0x20; return(false); }
      if (0x80 == (m & 0xC0)) { setDefault(); }
      if (0 == (m & 0x80))
      { 
         if (!logID(s)) { reset(); m|= 0x10; }
      }
      if (m & 0x80)
      {
         logStat(s);
         logIO(s);
         logMode(s);
         logConf(s);
         logCal(s);
         if (dataReady()) { logData(s); }
         // Hacky...
         return(true);
      }
      //s.print(v[1]>>6); s.print('F'); s.println(v[1] & 0xF);
      //for (int i=0; i<r; i++) { s.print(v[i],HEX); s.print(',');} s.println();
   } // testSPI

   void strID (Stream& s, const uint8_t v)
   {
      char chv='2'; chv+= v & 0x1;
      s.print("AD779"); s.print(chv);      
   } // strID
   
   void strChan (Stream& s, const uint8_t chan)
   {
      s.print(" CH:"); 
      switch(chan)
      {
         case AD779x::THERM:  s.println("THERM"); break;
         case AD779x::AVDD :  s.println("AVDD"); break;
         case AD779x::S1   :  s.println("S1"); break;
         default : s.print('D'); s.println(chan); break;
      }
   } // strChan
 
   bool logID (Stream& s)
   {
      int8_t i= checkID();
      if (i >= 0)
      {
         strID(s, i); s.println(" - OK");
      } else { s.println("UNKNOWN ID"); } // s.println(id,HEX); }
      return(i >= 0);
   } // logID

   void logStat (Stream& s)
   {
      readR(AD779x::COMMSTAT, &(sr.stat), 1);

      strID(s, (sr.stat & AD779x::IDB) > 0); s.print(':');
      s.print("STAT=");
      if (0 == (sr.stat & AD779x::NRDY)) { s.print('R'); }
      if (sr.stat & AD779x::ERR) { s.print('E'); }
      strChan(s, sr.stat & AD779x::CHM); 
   } // logStat

   void logIO (Stream& s)
   {
      uint8_t io;
      readR(AD779x::IO, &io, 1);

      s.print("AD779x:");
      s.print("IO= 0x"); s.println(io,HEX); 
      //if (io & 0x80) { s.print('R'); }
   } // logStat

   void logMode (Stream& s)
   {
      readR(AD779x::MODE, sr.mode.u8, 2);
      s.print("AD779x:");
      s.print("MODE= M#"); s.print(sr.mode.u8[1]>>5); 
      s.print(" C#"); s.print(sr.mode.u8[0]>>6);
      s.print(" R#"); s.println(sr.mode.u8[0] & AD779x::RM);
   } // 

   void logConf (Stream& s)
   {
      readR(AD779x::CONF, sr.conf.u8, 2);
      s.print("AD779x:");
      s.print("CONF= VB#"); s.print(sr.conf.u8[1] >> 6); s.print(' ');
      if (sr.conf.u8[1] & AD779x::BURNOUT) { s.print('R'); }
      if (sr.conf.u8[1] & AD779x::UNIPOLAR) { s.print('U'); }
      if (sr.conf.u8[1] & AD779x::BOOST) { s.print('O'); }
      s.print(" G"); s.print(gain()); s.print(' ');

      if (sr.conf.u8[0] & AD779x::VREF_INT) { s.print('I'); } else { s.print('X'); }
      if (sr.conf.u8[0] & AD779x::BUF) { s.print('B'); }
      strChan(s, sr.conf.u8[0] & AD779x::CHM);
   }
   
   void logCal (Stream& s)
   {
      uint32_t o, f;
      o= read24b(AD779x::OFFS);
      s.print("OFFS="); s.println(o);
      f= read24b(AD779x::FS);
      s.print("FS="); s.println(f);
   }

   void logData (Stream& s)
   {
      uint32_t v= read24b(); // very noisy ~10 lsb
      s.print("DATA= 0x"); s.print(v,HEX);
      s.print(','); s.print(v); 
      s.print(','); s.println(getV(v),4);
   }
}; // CAD779xDbg

// }; // using namespace AD779x
