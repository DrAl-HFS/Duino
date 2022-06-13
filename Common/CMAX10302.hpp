// Duino/Common/CMAX10302.hpp - MAXIM SpO2 sensor hackery
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors June 2022

#ifndef CMAX10302_HPP
#define CMAX10302_HPP

#include "CCommonI2C.hpp"

namespace MAX10302HW
{
  enum Device : uint8_t { ADDR=0x57 };
  enum Reg : uint8_t {
    INTS1, INTS2, INTE1, INTE2,
    FWRI, FOVC, FRDI, FDAT,
    FCON, MCON, SCON,
    LPA1=0xC, LPA2,
    MLCR1=0x11, MLCR2,
    T1=0x1F, T2, TCON,
    REV=0xFE, ID
  };
}; // namespace MAX10302HW

class CMAX10302 : public CCommonI2C
{
  uint8_t fr[3];
  uint8_t f;
  int8_t tC;
  uint8_t tx4;

public:
   CMAX10302 (void) { f= 0; fr[0]= fr[1]= fr[2]= 0; }

   bool identify (Stream& s)
   {
      uint8_t b[2]={0xFA,0xFA};
      writeTo(MAX10302HW::ADDR, MAX10302HW::REV);
      readFrom(MAX10302HW::ADDR, b, 2);
      s.print("CMAX10302::identify() - ");
      dumpHexByteList(s, b, 2);
      return(0x15 == b[1]);
   } // identify

   uint8_t reset (void)
   {
      uint8_t i=0, b[2]={MAX10302HW::MCON,0x40};
      writeTo(MAX10302HW::ADDR, b, 2);
      do
      {
         delay(1);
         writeTo(MAX10302HW::ADDR, b, 1);
         readFrom(MAX10302HW::ADDR, b+1, 1);

      } while ((++i < 10) && (0x40 == (b[1] & 0x40)));
      return(i);
   } // reset

   void dumpReg (Stream& s, uint8_t r0=MAX10302HW::INTS1, int8_t n=11)
   {
      if (n > 0)
      {
        uint8_t b[11];
        if (n > 11) { n= 11; }
        writeTo(MAX10302HW::ADDR, r0);
        readFrom(MAX10302HW::ADDR, b, n);
        s.print("CMAX10302::dump() - R"); s.print(r0); s.print('['); s.print(n); s.print(']');
        dumpHexByteList(s, b, n);
      }
   } // dumpReg

   void logData (Stream& s) { dump(s, fr, 3,"fr[]=","\n"); }

   void start (void)
   {
      { // FCON, MCON & SPCON
         uint8_t rb[4]= { MAX10302HW::FCON, 0x0<<5, 0x3, (0x1<<5) | (0x1<<2) | 0x1}; // Avg1; SpO2; 15.63pA/4096nA, 100Hz, 118us / 16bit
         writeTo(MAX10302HW::ADDR, rb, 3);
      }
      { // LPA1 & 2
         uint8_t rb[3]= { MAX10302HW::LPA1, 0xF, 0xF}; // 3mA
         writeTo(MAX10302HW::ADDR, rb, 3);
      }
      { // MLCR 1 & 2
         uint8_t rb[3]= { MAX10302HW::MLCR1, 0x21, 0x00}; // RED,IR,X,X
         writeTo(MAX10302HW::ADDR, rb, 3);
      }
   } // start

   int readData (uint8_t b[], int max)
   {
      if (max > 0)
      {
         int n= 0;
         writeTo(MAX10302HW::ADDR, MAX10302HW::FWRI);
         readFrom(MAX10302HW::ADDR, fr, 3);
         //for (int8_t i= 0; i<3; i++) { fr[i]&= 0x1F; }
         if (fr[1] > 0) { n= 32; } // overflow
         else
         {
            n= fr[0] - fr[2]; // write - read
            if (n < 0) { n+= 32; } // wrap
         }
         if (n > 0)
         {
            n*= 3; // samples -> bytes
            if (n > max) { n= max; }
            return readFrom(MAX10302HW::ADDR, b, n);
         }
      }
      return(0);
   } // readData

   void startTemp (bool sync=false)
   {
      if (0 == (f & 0x08))
      {
         uint8_t b[2]={ MAX10302HW::TCON, 0x1 };
         writeTo(MAX10302HW::ADDR, b, 2);
         f|= 0x08;
         if (sync) { delay(1); }
      }
   } // startTemp

   bool temp (bool cont)
   {
      uint8_t b[3]={0,0,0xFF};
      startTemp(true);
      if (f & 0x08)
      {
         writeTo(MAX10302HW::ADDR, MAX10302HW::T1);
         readFrom(MAX10302HW::ADDR, b, 3);
         if (0 == b[2])
         { // reading completed
            f&= ~0x08;
            tC= b[0];
            tx4= b[1];
            if (cont) { startTemp(); }
            return(true);
         }
      }
      return(false);
   } // temp

   void printT (Stream& s, const char *end="\n")
   {
     s.print("tC="); s.print(tC);
     printFrac(s, 625 * (uint32_t)tx4, 999); // 4dig -> (10^4)-1
     if (end) { s.print(end); }
   } // printT

}; // class MAX10302

#endif // CMAX10302_HPP
