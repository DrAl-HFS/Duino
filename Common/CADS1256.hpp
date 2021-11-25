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

#ifdef ARDUINO_AVR_MEGA2560
#ifndef PIN_NRST
#define PIN_NRST 48 // Mega
#endif
#ifndef PIN_NRDY
#define PIN_NRDY 49
#endif
#endif

#ifdef ARDUINO_TEENSY41
#ifndef PIN_NRST
#define PIN_NRST 8
#endif
#ifndef PIN_NDRDY
#define PIN_NDRDY 9
#endif
#endif

// NB: must follow pin defs
#include "CCommonSPI.hpp"

namespace ADS1256
{
   enum Cmd : uint8_t
   {  // Simple commands (no args)
      WAKEUP=0x00, SDATAC=0xF,
      SELFCAL=0xF0, SELFOCAL, SELFGCAL,  SYSOCAL, SYSGCAL, 
      SYNC=0xFC, STANDBY, RESET //, WAKEUP=0xFF
   };
   enum RWCmd : int8_t { RDATA=0x1, RDATAC=0x3, READR=0x10, WRITER=0x50 };
   enum Reg : int8_t
   {  // all registers byte addressable
      STATUS, MUX, ADCON, DRATE, IO, 
      OFFC0, OFFC1, OFFC2,  // 0:2 = 24b little-endian
      FSC0, FSC1, FSC2      // "
   };
   enum StatusFlag : int8_t { NDRDY=0x1, BUFEN=0x2, ACAL=0x4, ORDER=0x8 };
   
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
      pinMode(PIN_NDRDY, INPUT); // NB: not 5V tolerant, should be default...
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
   
   bool ready (void) { return(0 == digitalRead(PIN_NDRDY)); }
   
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
   CADS1256SPI (uint8_t clk_hk=0) : CADS1256Signal() { init(clk_hk); }
   
   // ARG: SPI clock rate in hecto-kilo-Hertz
   void init (uint8_t clk_hk=19) // NB datasheet requires SCK <= MCLK / 4
   {
      //CADS1256Signal::init();
      if (clk_hk > 0)
      {
        CCommonSPI::spiSet= SPISettings(clk_hk*100000, MSBFIRST, SPI_MODE1);
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

   bool ready (void)
   {
      readReg(ADS1256::STATUS, &(sr.status), 1);
      return(0 == (sr.status & ADS1256::NDRDY));
   } // ready

   uint32_t read24b (void)
   {
      UU32 v;
      readData(v.u8);
      v.u8[3]= 0;
      return(v.u32);
   } // read24b

   uint32_t read24b (ADS1256::Reg r)
   {
      UU32 v;
      readReg(r, v.u8, 3); // little-Endian ??
      v.u8[3]= 0;
      return(v.u32);
   } // read24b

protected:
   ADS1256::ShadowReg sr;
   
   int8_t readShadow (uint8_t n=5) { return readReg(ADS1256::Reg::STATUS, sr.r, n); }
   
   uint8_t rawID (void) { return(sr.r[0] >> 4); }
   uint8_t getID (void)
   {
      uint8_t id= rawID();
      if ((0x2|id) != id) { readShadow(); id= rawID(); }
      return(id);
   } // getID

}; // CADS1256Raw

class CADS1256Tables
{
public:
static const uint8_t rl[9];
static const uint16_t rh[7];

   uint16_t lookupRateIdx (const uint8_t i) const
   {
      if (i < 9) { return(rl[i]); } else { return(rh[i-9]); }
   } // lookupRateIdx
   
   // Sanity check
   uint16_t lookupRateCode (const uint8_t c) const
   {
      const uint8_t i= c >> 4;
      if (encodeRateIdx(i) == c)
      {
         return lookupRateIdx(i);
      }
      return(0);
   } // lookupRateCode
   
   // Generate control reg byte from table index
   uint8_t encodeRateIdx (const uint8_t i) const 
   {
      uint8_t c= 0; // (invalid)
      if (i <= 0xF)
      {
         c= i<<4;
         if (i <= 6) { c|= 0x3; }
         else if (i <= 9) { c|= 0x2; }
         else if (i == 10) { c|= 0x1; }
         //else { c|= 0x0; }
      }
      return(c);
   } // encodeRateIdx
   
}; // CADS1256Tables
// Sample rates for 7.68MHz clock
const uint8_t CADS1256Tables::rl[9]= { 2, 5, 10, 15, 25, 30, 50, 60, 100 };
const uint16_t CADS1256Tables::rh[7]= { 500, 1000, 2000, 3750, 7500, 15000, 30000 };
/* periodic times 2.5Hz -> 400ms, 
const uint8_t CADS1256Tables::plms[7]= { 200, 100, 66, 40, 33, 20, 16, 10, 2, 1 }; 
const uint16_t CADS1256Tables::plus[7]= { 500, 266 };
const uint8_t CADS1256Tables::phus[3]={ 133, 66, 33 };
*/

class CADS1256Util : public CADS1256Raw, CADS1256Tables
{

public:
   CADS1256Util (uint8_t clk_hk=0) : CADS1256Raw(clk_hk) { ; }

   int8_t identify (void)
   { 
      uint8_t id= getID();
      int8_t i= id + 0x3;
      if ((i >= 5) && (i <= 6)) { ; } else { i= -1; }
      return(i);
   }

   uint8_t gain (void) const
   {
      uint8_t ge= sr.adcon & 0x7;
      ge-= (ge > 6);
      return(1 << ge);
   } // gain 
   
   uint16_t rate (void) const
   {
      //return CADS1256Tables::lookupRateIdx(sr.drate >> 4);
      return CADS1256Tables::lookupRateCode(sr.drate);
   } // rate

}; // CADS1256Util



class CADS1256Dbg : public CADS1256Util
{
   char chid;

public:
   CADS1256Dbg (uint8_t clk_hk=0) : CADS1256Util(clk_hk), chid{'x'} { ; }

   void init (Stream& s) // clk_hk
   {
      CADS1256SPI::init();
      delay(1);
      logID(s);
   } // init

   void printDevIDS (Stream& s, const char *end=": ") const
   {
      s.print("ADS125"); s.print(chid); if (end) { s.print(end); }
   } // printDevIDS

   bool logID (Stream& s)
   {
      int8_t i= identify();
      if (i >= 0)
      {
        chid='0'+i;
        printDevIDS(s," - OK\n");
        return(true);
      }
      else { s.print("Raw ID: 0x"); s.print(rawID(),HEX); s.println(" - UNKNOWN"); }
      return(false);
   }

   char muxCh (uint8_t m) const
   {
      if (0 == (0x8 & m)) { return('0'+(0x7 & m)); }
      //else
      return('C');
   } // muxCH

   void logSR (Stream& s)
   {
      readShadow();
      if ('x' == chid) { logID(s); } else { printDevIDS(s); }
      s.print("SR[5] 0x{"); s.print(sr.r[0],HEX);
      for (int8_t i=1; i<5; i++) { s.print(','); s.print(sr.r[i],HEX);  }
      s.println('}');
      s.print('\t');
      s.print(" G"); s.print(gain());
      s.print(" R");  s.print(rate());
      s.print(" M+/-"); s.print(muxCh(sr.mux >> 4)); s.print("/"); s.print(muxCh(sr.mux)); 
      s.println('\n');
   } // logSR

}; // CADS1256Util

#endif //  CADS1256_HPP

