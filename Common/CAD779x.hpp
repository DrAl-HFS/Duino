// Duino/Common/CAD779x.hpp - AD779x precision ADC with SPI interface utility classes
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#include <SPI.h>


#include "MBD/mbdDef.h" // UU16/32

#ifndef SWAP
#define SWAP(_t, a, b) { _t t= (a); (a)= b; (b)= t; } // where ?
#endif // SWAP

#ifndef NCS
#define NCS SS //=10
#endif

namespace AD779x  // Caveat: namespace verbosity...
{
   enum Reg : int8_t { COMMSTAT, MODE, CONF, DATA, ID, IO, OFFS, FS };
   enum Stat : uint8_t { ERR=0x40, NRDY= 0x80 }; //, ID=0x04 }; CHAN
   // mode[0]
   enum Mode : int8_t { CONTINUOUS, SINGLE, IDLE, POWER_DOWN, CAL_IZS, CAL_IFS, CAL_XZS, CAL_XFS}; // <<5
   // mode[1]
   enum Clock : int8_t { CLK_INT, CLK_INT_OUT, CLK_EXT, CLK_EXT_DIV2 }; // <<6
   enum Rate : int8_t { ROFF, R470, R242, R123, R62, R50, R39, R33_2, // raw (no rejection filter for 50&60Hz noise)
                  R19_6, // 90dB rejection 60Hz noise
                  R16_7A, // 80dB rejection 50Hz noise
                  R16_7B, R12_5, R10, R8_33, R6_25, R4_17 }; // 65~74dB rejection 50&60Hz noise
   // conf[0]
   enum Bias : int8_t { BIAS_OFF, BIAS_A1, BIAS_A2 }; // <<6
   enum Flag : uint8_t { BURNOUT=0x20, UNIPOLAR=0x10, BOOST=0x08 };
   enum Gain : int8_t { G1, G2, G4, G8, G16, G32, G64, G128 };
   // conf[1]
   enum VRef : uint8_t { VREF_EXT=0, VREF_INT=0x80 }; // unbuffered
   enum Chan : int8_t { D1, D2, D3, S1, THERM=6, AVDD };
   // IO register
   enum EXCD : int8_t { STRAIGHT, SWAP, DUAL1, DUAL2 }; // <<2 Excitation current source destination ("direction" ?!?)
   enum EXCI : int8_t { I0, I10, I210, I1000 }; // Excitation current 10uA ~ 1mA
   
   typedef struct
   {
      UU16  mode, conf;
      U8    stat;
   } ShadowReg;
   
}; // namespace AD779x

/*
class CAD779xIO
{
   SPISettings set;
};
*/
class CAD779xRaw
{
   SPISettings set;
   AD779x::ShadowReg sr;
   
public:
   CAD779xRaw (uint32_t clk) : m{0x1} { SPI.begin(); set= SPISettings(clk, MSBFIRST, SPI_MODE3); }

   //void init (clk) { if (m & 1) { SPI.end(); } set= SPISettings(clk, MSBFIRST, SPI_MODE3); SPI.begin(); m= 0x1; }
   void close (void) { SPI.end(); m= 0x00; }

   void reset (void) // not reliable.. ?
   {
      start();
      for (int8_t iB=0; iB<4; iB++) { SPI.transfer(0xFF); }
      end();
      m|= 0x20;
   } // reset

   bool dataReady (void)
   {
      readR(AD779x::COMMSTAT, &(sr.stat), 1);
      return(0 == (sr.stat & AD779x::NRDY));
   }
   
   uint32_t readData (void)
   {
      UU32 v;
      readR(AD779x::DATA, v.u8, 3);
      SWAP(uint8_t, v.u8[0], v.u8[2]); // 24b endian reversal (big->little)
      v.u8[3]= 0;
      return(v.u32);
   } // readData

protected:
   uint8_t m;

   void start (void) { SPI.beginTransaction(set); digitalWrite(NCS,0); }
   void end (void) { SPI.endTransaction(); digitalWrite(NCS,1); }

   int8_t readR (AD779x::Reg r, uint8_t vB[], const uint8_t nB)
   {
      start();
      SPI.transfer( (0x8 | r) << 3); // First byte to COMM reg defines read target register
      for (int8_t iB=0; iB<nB; iB++) { vB[iB]= SPI.transfer(0xFF); }
      end();
      return(nB);
   } // readR
   
   int8_t writeR (AD779x::Reg r, const uint8_t vB[], const uint8_t nB)
   {
      start();
      SPI.transfer(r << 3); // First byte to COMM reg defines write target register
      for (int8_t iB=0; iB<nB; iB++) { SPI.transfer(vB[iB]); }
      end();
      return(nB);
   } // writeR
   
   char checkID (void) //  return ASCII '2' or '3' if identified as AD779x, otherwise 0 (NUL)
   {
      uint8_t id=0x5F;
      readR(AD779x::ID, &id, 1);
      uint8_t t= ((id & 0xF) - 0xA);
      if (t <= 1) { m|= 0x80; return('2'+t); }
      return(0);
   }

   //using AD779x:: ???
   
   void setConf (AD779x::Bias b=AD779x::BIAS_OFF, AD779x::Flag f=AD779x::UNIPOLAR, AD779x::Gain g=AD779x::G1, AD779x::VRef r=AD779x::VREF_INT, AD779x::Chan c=AD779x::AVDD)
   {
      sr.conf.u8[0]= (b<<6)|f|g;
      sr.conf.u8[1]= r|c;
      
      writeR(AD779x::CONF, sr.conf.u8, 2);
   } // setConf

   void setMode (AD779x::Mode m=AD779x::CONTINUOUS, AD779x::Clock c=AD779x::CLK_INT_OUT, AD779x::Rate r=AD779x::R16_7A)
   {
      sr.mode.u8[0]= m<<5;
      sr.mode.u8[1]= (c<<6)|r;
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

class CAD779xDbg : public CAD779xRaw
{
public:
   CAD779xDbg (uint32_t clk=1E6) : CAD779xRaw(clk) { ; }
   
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
   
   bool logID (Stream& s)
   {
      char ch= checkID();
      if (ch)
      {
        s.print("AD779"); s.print(ch); s.println(" - OK");
      } else { s.println("UNKNOWN ID"); } // s.println(id,HEX); }
      return(ch);
   } // logID

   void logStat (Stream& s)
   {
      uint8_t stat;
      readR(AD779x::COMMSTAT, &stat, 1);
      
      char chv='2'; chv+= ((stat & 0x8) > 0);
      s.print("AD779"); s.print(chv); s.print(':');
      s.print("STAT=");
      if (stat & 0x80) { s.print('R'); }
      if (stat & 0x40) { s.print('E'); }
      s.print(" CH"); s.println(stat & 0x7); 
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
      uint8_t mode[2];
      readR(AD779x::MODE, mode, 2);
      s.print("AD779x:");
      s.print("MODE= M"); s.print(mode[0]>>5); 
      s.print(" C"); s.print(mode[1]>>6);
      s.print(" F"); s.println(mode[1] & 0xF);
   } // 
   
   void logConf (Stream& s)
   {
      uint8_t conf[2];
      readR(AD779x::CONF, conf, 2);
      s.print("AD779x:");
      s.print("CONF= VB"); s.print(conf[0] >> 6);
      if (conf[0] & 0x20) { s.print('N'); }
      if (conf[0] & 0x10) { s.print('P'); }
      if (conf[0] & 0x08) { s.print('O'); }
      s.print(" G"); s.print(conf[0] & 0x7);

      s.print(' ');
      if (conf[1] & 0x80) { s.print('I'); } else { s.print('X'); }
      if (conf[1] & 0x20) { s.print('B'); }
      s.print(" CH"); s.println(conf[1] & 0x7);
   }
   
   void logCal (Stream& s)
   {
      UU32 v= { 0 };
      readR(AD779x::OFFS, v.u8, 3);
      s.print("OFFS="); s.println(v.u32);
      v.u32= 0;
      readR(AD779x::FS, v.u8, 3);
      s.print("FS="); s.println(v.u32);
   }

   void logData (Stream& s)
   {
      uint32_t v= readData(); // very noisy ~10 lsb
      s.print("DATA= 0x"); s.print(v,HEX);
      s.print(','); s.print(v); 
      float V= v * ((1.17 * 6) / ((uint32_t)1<<24)); // single ended
      s.print(','); s.println(V);
   }
}; // CAD779xDbg

