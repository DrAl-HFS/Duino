// Duino/Common/CMAX30102.hpp - MAXIM SpO2 sensor hackery
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors June 2022

#ifndef CMAX10302_HPP
#define CMAX10302_HPP

#include "CCommonI2C.hpp"

namespace MAX30102HW
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
   enum Average : uint8_t { AV1, AV2, AV4, AV8, AV16, AV32 };
   enum Mode : uint8_t { HR=0x2, SPO2=0x3, MULTI=0x7 };
   enum RangeCtrl : uint8_t { RC2K, RC4K, RC8K, RC16K };
   enum SampleRate : uint8_t { SR50, SR100, SR200, SR400, SR800, SR1000, SR1600, SR3200 };
   enum PulseWidth : uint8_t { PW69, PW118, PW215, PW411 };
}; // namespace MAX30102HW

class CMAX30102 : public CCommonI2C
{
   uint8_t fr[3];
   uint8_t f;
   int8_t tC;
   uint8_t tx4;

   uint8_t defSCON (MAX30102HW::RangeCtrl rc, MAX30102HW::SampleRate sr, MAX30102HW::PulseWidth pw)
   {
      return((rc<<5)|(sr<<2)|pw);
   } // defSCON

public:
   CMAX30102 (void) { f= 0; fr[0]= fr[1]= fr[2]= 0; }

   bool identify (Stream& s)
   {
      uint8_t b[2]={0xFA,0xFA};
      writeTo(MAX30102HW::ADDR, MAX30102HW::REV);
      readFrom(MAX30102HW::ADDR, b, 2);
      s.print("CMAX30102::identify() - ");
      dumpHexByteList(s, b, 2);
      return(0x15 == b[1]);
   } // identify

   uint8_t reset (void)
   {
      uint8_t i=0, b[2]={MAX30102HW::MCON,0x40};
      writeTo(MAX30102HW::ADDR, b, 2);
      do
      {
         delay(1);
         writeTo(MAX30102HW::ADDR, b, 1);
         readFrom(MAX30102HW::ADDR, b+1, 1);

      } while ((++i < 10) && (0x40 == (b[1] & 0x40)));
      return(i);
   } // reset

   void dumpReg (Stream& s, uint8_t r0=MAX30102HW::INTS1, int8_t n=11)
   {
      if (n > 0)
      {
        uint8_t b[11];
        if (n > 11) { n= 11; }
        writeTo(MAX30102HW::ADDR, r0);
        readFrom(MAX30102HW::ADDR, b, n);
        s.print("CMAX30102::dump() - R"); s.print(r0); s.print('['); s.print(n); s.print(']');
        dumpHexByteList(s, b, n);
      }
   } // dumpReg

   void logData (Stream& s) { dump(s, fr, 3,"fr[]=","\n"); }

   void start (void)
   {
      { // FCON, MCON & SPCON
         uint8_t rb[4]= { MAX30102HW::FCON, MAX30102HW::AV1<<5, MAX30102HW::SPO2, }; // Avg1; HRM|SpO2 ...
         //(0x1<<5) | (0x1<<2) | 0x1};
         // 15.63pA/4096nA, 100Hz, 118us / 16bit
         rb[3]= defSCON(MAX30102HW::RC4K, MAX30102HW::SR100, MAX30102HW::PW118);
         writeTo(MAX30102HW::ADDR, rb, sizeof(rb));
      }
      { // LPA1 & 2
         uint8_t rb[3]= { MAX30102HW::LPA1, 0xF, 0xF}; // 3mA, 0x1E -> 6mA
         writeTo(MAX30102HW::ADDR, rb, sizeof(rb));
      }
      { // MLCR 1 & 2
         uint8_t rb[3]= { MAX30102HW::MLCR1, 0x21, 0x00}; // 1|2,3|4 -> RED,IR,X,X
         writeTo(MAX30102HW::ADDR, rb, sizeof(rb));
      }
   } // start

   int readData (uint8_t b[], int max)
   {
      if (max > 0)
      {
         int n= 0;
         writeTo(MAX30102HW::ADDR, MAX30102HW::FWRI);
         readFrom(MAX30102HW::ADDR, fr, 3);
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
            return readFrom(MAX30102HW::ADDR, b, n);
         }
      }
      return(0);
   } // readData

   void startTemp (bool sync=false)
   {
      if (0 == (f & 0x08))
      {
         uint8_t b[2]={ MAX30102HW::TCON, 0x1 };
         writeTo(MAX30102HW::ADDR, b, 2);
         f|= 0x08;
         if (sync) { delay(1); }
      }
   } // startTemp

   bool temp (bool cont)
   {
      uint8_t b[3]={0x80,0,0xFF};
      startTemp(true);
      if (f & 0x08)
      {
         writeTo(MAX30102HW::ADDR, MAX30102HW::T1);
         readFrom(MAX30102HW::ADDR, b, 3);
         if ((0 == b[2]) && (b[0] > -40))
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

}; // class MAX30102

#endif // CMAX30102_HPP
