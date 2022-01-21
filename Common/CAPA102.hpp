// Duino/Common/CAPA102.hpp - Utils for APA102 RGB LEDs with 2-wire SPI-like interface
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2021 - Jan 2022

#ifndef CAPA102_HPP
#define CAPA102_HPP

#include <SPI.h>
#define HSPI SPI

#ifndef PIN_NCS
#define PIN_NCS 17
#endif

#include "CCommonSPI.hpp"


class CAPA102State
{
public:
   uint8_t lbgr[4]; // NB: packed array usage requires this data member only

   CAPA102State (uint8_t r=0x1F, uint8_t g=0, uint8_t b=0) { setRGB(r,g,b); setL(r+g+b); }

   void setRGB (uint8_t r, uint8_t g, uint8_t b) { lbgr[3]= r; lbgr[2]= g; lbgr[1]= b; }
   void setL (uint8_t l) { lbgr[0]= 0xE0 | min(0x1F, l); }

//#ifdef DEBUG
   void dumpRGBL (Stream& s)
   { 
      int8_t i=4;
      while (--i > 0) { s.print(lbgr[i]); s.print(','); }
      if (0xE0 == (lbgr[0] & 0xE0)) { s.print(lbgr[0] & 0x1F); }
      else { s.print(lbgr[0],HEX); }
   }
}; // CAPA102State

class CAPA102SPI : public CCommonSPI // *Factoring
{
 
public:
   CAPA102SPI (void) { ; }
   // ~CAPA102SPI  (void) { release(); } ???

   void init (void)
   {
      spiSet= SPISettings(4<<20, MSBFIRST, SPI_MODE0);
      //HSPI.setTX(19); HSPI.setSCK(18); // EFP community core required
      CCommonSPI::begin();
   } // init

   void release (void) { HSPI.end(); }

   uint16_t start (void)
   {
      CCommonSPI::start();
      return CCommonSPI::writeb(0x00,4); // SOF / preamble
   } // start

   uint16_t complete (uint16_t n)
   {  // NB: for n LEDs, n/2 trailing clock pulses (i.e. bits) are 
      // required to fully propagate data, i.e. nB / (4*2) bits. So
      // rounding up bits gives the number of bytes to send:
      // ((nB / 8) + 7) / 8 = (nB + 7*8) / 64
      n= CCommonSPI::writeb(0xFF, (n+56)/64); // EOF / footer
      CCommonSPI::complete();
      return(n);
   } // complete

}; // CAPA102SPI

class CAPA102Pattern : public CAPA102SPI
{
private:

   uint16_t writeRep (const uint8_t b[], const uint16_t nB, const uint8_t nR)
   {  // NB always writes at least nB
      uint16_t tB=0;
      uint8_t iR= 0;
      do { tB+= CCommonSPI::write(b, nB); } while (iR++ < nR);
      return(tB);
   } // writeRep

public:
   CAPA102Pattern (void) { ; }

   // Repeated write of n LED states
   uint16_t writeRep (const CAPA102State led[], const uint8_t nL, const uint8_t nR=0)
   {
      uint16_t nB;
      start();
      nB= writeRep(led[0].lbgr, nL<<2, nR);
      return(4 + nB + complete(nB));
   } // writeRep

   // Indirect LED patterns with individual repeat count
   uint16_t writeInd (const CAPA102State led[], const uint8_t iLc[], const uint8_t n)
   {
      uint16_t nB;
      start();
      for (uint8_t i=0; i < n; i++)
      { 
         uint8_t c= iLc[i] & 0x0F;
         if (c > 0)
         {
            uint8_t iL= iLc[i] >> 4;

            nB+= writeRep(led[iL].lbgr, 4, c-1);
         }
      }
      return(4 + nB + complete(nB));
   } // writeInd

}; // CAPA102Pattern

#endif // CAPA102_HPP
