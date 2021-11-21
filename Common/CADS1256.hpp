// Duino/Common/CADS1256.hpp - 'Duino high precison ADC
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2021

#ifndef CADS1256_HPP
#define CADS1256_HPP

#include <SPI.h>
#include "MBD/mbdDef.h" // UU16/32

#ifndef HSPI
#define HSPI SPI
#endif
#ifndef PIN_NCS
#define PIN_NCS SS // Uno=10,Mega=53
#endif
#ifndef PIN_NRST
#define PIN_NRST 48 // Mega
#endif
#ifndef PIN_NRDY
#define PIN_NRDY 49
#endif

#include "CCommonSPI.hpp"

namespace ADS1256
{
   typedef enum Cmd : uint8_t
   {  // Simple commands (no args)
      WAKEUP=0x00, SDATAC=0xF,
      SELFCAL=0xF0, SELFOCAL, SELFGCAL,  SYSOCAL, SYSGCAL, 
      SYNC=0xFC, STANDBY, RESET //, WAKEUP=0xFF
   };
   typedef enum RWCmd : int8_t { RDATA=0x1, RDATAC=0x3, READR=0x10, WRITER=0x50 };
   typedef enum Reg : int8_t
   {  // all registers byte addressable
      STATUS, MUX, ADCON, DRATE, IO, 
      OFFC0, OFFC1, OFFC2,  // 0:2 = 24b little-endian
      FSC0, FSC1, FSC2      // "
   };
   
   union ShadowReg
   { 
      struct { uint8_t status, mux, adcon, drate, io; }; // anon
      uint8_t r[5];
   };

}; // namespace ADS1256

class CADS1256Signal
{
protected:
   CADS1256Signal (bool initNow=true)
   {
      pinMode(PIN_NRDY, INPUT); // NB: not 5V tolerant, should be default...
      if (initNow) { init(); }   
   } // CADS1256Signal

   void init (void)
   {
      pinMode(PIN_NCS, OUTPUT);
      digitalWrite(PIN_NCS,1);
      pinMode(PIN_NRST, OUTPUT);
      digitalWrite(PIN_NRST,1);
      //pinMode(PIN_NRDY, INPUT);
   } // init
   
   void syncRead (void) { delayMicroseconds(10); }

   bool reset (const uint16_t t=0)
   {
      digitalWrite(PIN_NRST,0);
      delayMicroseconds(1);
      digitalWrite(PIN_NRST,1);
      if (t > 0) { return syncReady(t); }
   } // reset
   
   bool ready (void) { return(0 == digitalRead(PIN_NRDY)); }
   
   bool syncReady (uint16_t tM= 100, const uint16_t tS= 10)
   {
      do
      {
         if (ready()) { return(true); }
         uint16_t t= min(tS,tM);
         if (t > 0) { delayMicroseconds(t); tM-= t; } 
      } while (tM > 0);
      return(false);
   } // syncReady
   
}; // CADS1256Signal


class CADS1256SPI : CCommonSPI, public CADS1256Signal
{
public:
   CADS1256SPI (uint8_t clkH=0) : CADS1256Signal() { init(clkH); }
   
   // ARG: SPI clock rate in Hecto-Hertz
   void init (uint8_t clkH=19) // NB datasheet requires SCK <= MCLK / 4
   {
      //CADS1256Signal::init();
      if (clkH > 0)
      {
        CCommonSPI::spiSet= SPISettings(clkH*100000, MSBFIRST, SPI_MODE1);
        HSPI.begin();
      }
   } // init

   void close (void) { HSPI.end(); } // hsm= 0x00; }

protected:
   // NB: Delay of 50 CLKIN cycles (CLKIN typically 7.68MHz) required in
   // read operations. Allow 10us (rather than 6.5us) as a safety margin.
    
   int8_t cmd (ADS1256::Cmd c)
   {
      start();
      HSPI.transfer(c); // writeb(c);
      complete();
      return(1);
   } // cmd
   
   int8_t readReg (ADS1256::Reg reg, uint8_t vB[], const uint8_t nB)
   {  // ASSERT(nB<=16);
      int8_t r= 0;
      start();
      HSPI.transfer(ADS1256::READR|reg); // First byte defines initial read target register
      HSPI.transfer(nB-1);
      syncRead();
      r= read(vB,nB);
      complete();
      return(r);
   } // readReg

   int8_t writeReg (ADS1256::Reg reg, const uint8_t vB[], const uint8_t nB)
   {  // ASSERT(nB<=16);
      int8_t r= 0;
      start();
      HSPI.transfer(ADS1256::WRITER|reg); // First byte defines initial write target register
      HSPI.transfer(nB-1);
      r= write(vB,nB);
      complete();
      return(r);
   } // writeReg

   int8_t readData (uint8_t vB[3])
   {
      start();
      HSPI.transfer(ADS1256::RDATA);
      syncRead();
      readRev(vB,3);
      complete();
      return(3);
   } // readData

}; // CADS1256SPI

class CADS1256Raw : public CADS1256SPI
{
public:
   CADS1256Raw (uint8_t clkH=0) : CADS1256SPI(clkH) { for (int8_t i=0; i<5; i++) { sr.r[i]= 0xFF; } }

protected:
   ADS1256::ShadowReg sr;
   
   int8_t readSR (uint8_t n=5) { readReg(ADS1256::Reg::STATUS, sr.r, n); }
   
   uint8_t rawID (void) { return(sr.r[0] >> 4); }
   uint8_t getID (void)
   {
      uint8_t id= rawID();
      if (0x3 != id) { readSR(); id= rawID(); }
      return(id);
   } // getID

}; // CADS1256Raw

class CADS1256Util : public CADS1256Raw
{
static const uint8_t rl[9];  // { 2, 5, 10, 15, 25, 30, 50, 60, 100 };
static const uint16_t rh[7]; // { 500, 1000, 2000, 3750, 7500, 15000, 30000 };

public:
   CADS1256Util (uint8_t clkH=0) : CADS1256Raw(clkH) { ; }

   bool verifyID (void) { return(0x3 == getID()); }

   uint8_t gain (void)
   {
      uint8_t ge= sr.adcon & 0x7;
      ge-= (ge > 6);
      return(1 << ge);
   } // gain 
   
   uint16_t rate (void)
   {
      const uint8_t i= sr.drate >> 4; // rate index in high nybble
      if (i < 9) { return(rl[i]); }
      else { return(rh[i-9]); }
   } // rate

}; // CADS1256Util

static const uint8_t CADS1256Util::rl[9]= { 2, 5, 10, 15, 25, 30, 50, 60, 100 };
static const uint16_t CADS1256Util::rh[7]= { 500, 1000, 2000, 3750, 7500, 15000, 30000 };


class CADS1256Dbg : public CADS1256Util
{
public:
   CADS1256Dbg (uint8_t clkH=0) : CADS1256Util(clkH) { ; }

   void printDevIDS (Stream& s, const char *end=": ")
   {
      s.print("ADS1256"); if (end) { s.print(end); }
   } // printDevIDS

   bool logID (Stream& s)
   {
      if (verifyID()) { printDevIDS(s," - OK\n"); }
      else { s.print("Raw ID: 0x"); s.print(rawID()); s.print(" - UNKNOWN"); }
   }

   void logSR (Stream& s)
   {
      readSR();
      printDevIDS(s);
      s.print("SR[5] 0x{"); s.print(sr.r[0],HEX);
      for (int8_t i=1; i<5; i++) { s.print(','); s.print(sr.r[i],HEX);  }
      s.println('}');
      s.print('\t');
      s.print(" G"); s.print(gain());
      s.print(" R");  s.print(rate());
      s.println('\n');
   } // logSR

}; // CADS1256Util

#endif //  CADS1256_HPP

