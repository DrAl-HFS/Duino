// Duino/Common/CMAX72.hpp - Simple hacks for MAX72xx LED controllers with SPI-ish interface
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2021

#ifndef CMAX72_HPP
#define CMAX72_HPP

#include <SPI.h>
#define HSPI SPI

#ifndef PIN_NCS
#define PIN_NCS SS
#endif

#include "CCommonSPI.hpp"


namespace MAX72
{
   enum Reg : int8_t { NOP, DIGIT, DECODE=0x09, INTENSITY, SCAN, SHUTDOWN, TEST=0x0F };
};


class CMAX72State
{
public:
   uint8_t b[8]; // NB: packed array usage requires this data member only

   CMAX72State (void) { for (int8_t i=0; i<8; i++) { b[i]= i; } }
   CMAX72State (uint8_t v) { for (int8_t i=0; i<8; i++) { b[i]= v; } }

//#ifdef DEBUG
   void dump (Stream& s, const char *end="\n")
   {
      s.print(b[0], HEX);
      for (int8_t i=1; i<8; i++) { s.print(' '); s.print(b[i], HEX); }
      if (end) { s.print(end); }
   }

}; // CMAX72State

class CMAX72SPI : public CCommonSPI
{
   uint8_t nR;

public:
   CMAX72SPI (uint8_t n) { nR= n-1; }

   void init (void)
   {
      spiSet= SPISettings(5000000, MSBFIRST, SPI_MODE0);
      CCommonSPI::begin();
   } // init

   void release (void) { HSPI.end(); }

   void writeRegRep (const MAX72::Reg r, const uint8_t v, const uint8_t n)
   {
      uint8_t i= 0;
      CCommonSPI::start();
      do
      {
         HSPI.transfer(r);
         HSPI.transfer(v);
      } while (i++ < n);
      CCommonSPI::complete();

      //if (pS) { pS->print(r, HEX); pS->print(','); pS->print(v, HEX); pS->print('*'); pS->print(1+n); pS->print(';'); }
   } // writeReg

   void power (bool on=true) { writeRegRep(MAX72::SHUTDOWN, on, nR); }
   void bright (const uint8_t v) { writeRegRep(MAX72::INTENSITY, v & 0x7, nR); }
   void reset (void)
   {
      writeRegRep(MAX72::TEST, 0, nR);
      writeRegRep(MAX72::SCAN, 0x7, nR);   // all
      writeRegRep(MAX72::INTENSITY, 0x3, nR); // 25%
      writeRegRep(MAX72::DECODE, 0, nR); // off
   } // reset

   void testPattern (uint8_t k=1)
   {
      for (int8_t i=0; i<8; i++) { writeRegRep(MAX72::DIGIT+i, k+i, nR); }
   } // testPattern

//#ifdef DEBUG
}; // CMAX72SPI
/*
class CMAX72Pattern : public CMAX72SPI
{
private:

   void writeRaw (const uint8_t b[], const uint8_t nB)
   {
      for (uint8_t i=0; i<nB; i++)
      {
         writeReg(MAX72::DIGIT+(0x7&i), b[i]);
      }
   } // writeRaw

public:
   CMAX72Pattern (void) { ; }

   // Repeated write of n LED matrix states
   uint16_t writeRep (const CMAX72State mat[], const uint8_t nM=1, const uint8_t nR=0)
   {
      uint8_t iR=0;
      do
      {
         for (uint8_t i=0; i<nM; i++) { writeRaw(mat[i].b, 8); }
      } while (iR++ < nR);
      return(16*nM*nR);
   } // writeRep

   // Indirect LED patterns with individual repeat count
   uint16_t writeInd (const CMAX72State led[], const uint8_t iLc[], const uint8_t n)
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

}; // CMAX72Pattern
*/
#endif // CMAX72_HPP
