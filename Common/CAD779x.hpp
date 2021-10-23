// Duino/Common/CAD779x.hpp - AD779x precision ADC with SPI interface utility classes
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#include <SPI.h>

#ifndef NCS
#define NCS SS //=10
#endif

namespace AD779x
{
   enum Reg { COMMSTAT, MODE, CONF, DATA, ID, IO, OFFS, FS };
}; // namespace AD779x

class CAD779xRaw
{
   SPISettings set;
  
public:
   CAD779xRaw (uint32_t clk) : m{0x1} { SPI.begin(); set= SPISettings(clk, MSBFIRST, SPI_MODE3); }

   //void init (clk) { if (m & 1) { SPI.end(); } set= SPISettings(clk, MSBFIRST, SPI_MODE3); SPI.begin(); m= 0x1; }
   //void reset (uint32_t clk=1E6
   void close (void) { SPI.end(); m= 0x00; }
    
protected:
   uint8_t m;

   void start (void) { SPI.beginTransaction(set); digitalWrite(NCS,0); }
   void end (void) { SPI.endTransaction(); digitalWrite(NCS,1); }

   int8_t readR (AD779x::Reg r, uint8_t vB[], const uint8_t nB)
   {
      start();
      SPI.transfer( (0x8 | r) << 3);
      for (int8_t iB=0; iB<nB; iB++) { vB[iB]= SPI.transfer(0xFF); }
      end();
      return(nB);
   } // readR
   
   int8_t writeR (AD779x::Reg r, const uint8_t vB[], const uint8_t nB)
   {
      start();
      SPI.transfer(r << 3);
      for (int8_t iB=0; iB<nB; iB++) { SPI.transfer(vB[iB]); }
      end();
      return(nB);
   } // writeR
   
   char checkID (void)
   {
      uint8_t id=0x5F;
      readR(AD779x::ID, &id, 1);
      uint8_t t= ((id & 0xF) - 0xA);
      if (t <= 1) { m|= 0x80; return('2'+t); }
      return(0);
   }

   void setDefault (void)
   {
      uint8_t r[2]={0,0x80}; // no gain, internal ref.
      
      writeR(AD779x::CONF, r, 2);
      r[0]= 0x1<<5; // single conversion
      r[1]= (0x1<<6) | 0x8; // intl. 16kHz clk, 19.6Hz conv. rate
      writeR(AD779x::MODE, r, 2);
      
      m|= 0x40;
   }
}; // CAD779xRaw

class CAD779xDbg : public CAD779xRaw
{
public:
   CAD779xDbg (uint32_t clk=1E6) : CAD779xRaw(clk) { ; }
   
   bool test (Stream& s)
   {
      if (0x80 == (m & 0xC0)) { setDefault(); }
      if (0 == (m & 0x80)) { logID(s); }
      if (m & 0x80)
      {
         logStat(s);
         logMode(s);
         logConf(s);
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
}; // CAD779xDbg

