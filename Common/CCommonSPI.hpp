// Duino/Common/CCommonSPI.hpp - 'Duino encapsulation of functionality common to many SPI interfaces
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2021 - Mar 2022

#ifndef CCOMMON_SPI_HPP
#define CCOMMON_SPI_HPP

//include <SPI.h>
#ifndef HSPI
#define HSPI SPI
#endif

#ifndef PIN_NCS
#define PIN_NCS   SS
#endif

// NB: Max SPI clock is typically half the core clock...

#ifdef ARDUINO_ARCH_STM32F1
#define SPI_CLOCK_DEFAULT  36 // MHz
#endif

#ifdef ARDUINO_ARCH_STM32F4
#define SPI_CLOCK_DEFAULT  42 // MHz
#endif

#ifndef SPI_CLOCK_DEFAULT
#define SPI_CLOCK_DEFAULT 8
#endif

// TODO: rethink device hard/soft state sbstraction...
namespace Device
{
   enum HState : int8_t { UNKNOWN, FAIL, ERROR, OFF, SLEEP, STANDBY, READY, WAIT }; // 3b
   enum HAct   : int8_t { NONE, RESET, CALIBRATE, TEST }; // 2b   MEASURE?
   //enum SFlag : uint8_t { IDENT=0x10, CONFIG=0x20, CALIB=0x40, TEST=0x80 };
}; // Device

namespace Bus
{
   enum IF : int8_t { FAIL, UNKNOWN, AVAIL, CONFIGURED }; // 2b
}; // Bus

class CCommonState
{
   uint8_t hsc; // state counter
   uint8_t hws; // current & previous state
   uint8_t ifc; // ?
   uint8_t ifs;

public:
   CCommonState (void) : hsc{0}, hws{0x11} { ; }

//protected:
   uint8_t getSC (void) { return(hsc); }
   Device::HState getHW (void) { return((Device::HState)(hws & 0x7)); }

   void tick (void) { hsc+= (hsc<0xFF); }

   void setHW (Device::HState s)
   {
      if (getHW() != s)
      {
         hws= (hws << 4) | s;
         hsc= 0;
      }
   } // setHW

   Bus::IF getIF (void) { return((Bus::IF)(ifs & 0x3)); }

   void setIF (Bus::IF s) { ifs= (ifs & ~0x3) | s; }

//#ifdef DEBUG
   void dumpChange (Stream& s)
   {
      int8_t i= hws & 0xF;
      if ((hws >> 4) != i)
      {
         static const char *sc="FXOSRBEC";
         if (i < 8) { s.print(sc[i]); }
         else { s.print('['); s.print(i); s.print(']'); }
         s.flush();
      }
   } // dumpChange
   void dump (Stream& s, const char *end="\n")
   {
      s.print("HWS:"); s.print(hws,HEX);
      if (end) { s.print(end); }
   } // dump
//#endif
}; // CCommonState

class CSelect
{
public:
   CSelect (void) { ; } // begin(); }

   void begin (void) { pinMode(PIN_NCS, OUTPUT); digitalWrite(PIN_NCS,1); }
   void start (void) { digitalWrite(PIN_NCS,0); }
   void complete (void) { digitalWrite(PIN_NCS,1); }
}; // CSelect

class CCommonSPI : public CSelect
{
protected:
   SPISettings spiSet;

   CCommonSPI (void) { ; }

   void begin (void) { HSPI.begin(); CSelect::begin(); } // NB: ensure select pin setup follows default interface setup
   void start (void) { HSPI.beginTransaction(spiSet); CSelect::start(); }
   void complete (void) { CSelect::complete(); HSPI.endTransaction(); }
   void end (void) { HSPI.end(); }
   // NB: transfer((void*), int); is read-write in same buffer ...
   // Beware of sending "dummy" bytes for reading: some devices
   // may interpret certain bytes as a command causing eg. a reset

   int read (uint8_t b[], const int n, const uint8_t w=0xAA)
   {
      int i;
      for (i=0; i<n; i++) { b[i]= HSPI.transfer(w); }
      return(i);
   } // write

   int writeb (const uint8_t b, const int n=1)
   {
      int i= 0;
      for (; i<n; i++) { HSPI.transfer(b); }
      return(i);
   } // writeb

   int write (const uint8_t b[], const int n)
   {
      int i= 0;
      for (; i<n; i++) { HSPI.transfer(b[i]); }
      return(i);
   } // write

}; // CCommonSPI

// DEBUG
class CLoopBackSPI : public CCommonSPI
{
public:
   CLoopBackSPI (uint8_t clkMHz=1)
   {
      spiSet= SPISettings(clkMHz*1000000, MSBFIRST, SPI_MODE0);
   }

   void begin (Stream& s)
   {
      CCommonSPI::begin();
      s.print("NCS="); s.println(PIN_NCS);
   }

   bool test (Stream& s, uint8_t i)
   {
      static const uint8_t out[]={0x5A,0xA5};
      uint8_t r;
      i&= 0x1;
      start();
      r= HSPI.transfer(out[i]);
      complete();
      //if (s.?)
      {
         s.print("LB["); s.print(i); s.print("] ");
         s.print(out[i],HEX); s.print("->"); s.println(r,HEX);
      }
      return(r == out[i]);
   } // test

}; // CLoopBackSPI

// Extension set 1 of common SPI functions
class CCommonSPIX1 : public CCommonSPI
{
public:
   //CCommonSPIX1 (void) { ; }

   // Reverse (byte-endian) order read & write
   int readRev (uint8_t b[], const int n, const uint8_t w=0xAA)
   {
      if (n <= 0) { return(0); }
      int i= n;
      while (i-- > 0) { b[i]= HSPI.transfer(w); }
      return(n);
   } // readRev

   int writeRev (const uint8_t b[], const int n)
   {
      if (n <= 0) { return(0); }
      int i= n;
      while (i-- > 0) { HSPI.transfer(b[i]); }
      return(n);
   } // writeRev

   // int readFrags (uint8_t * const ppF[], uint8_t lF[], const int nF)

   int writeFrags (const uint8_t * const ppF[], const uint8_t lF[], const int nF)
   {
      int t= 0;
      for (int iF=0; iF<nF; iF++)
      {
         t+= write(ppF[iF], lF[iF]);
      }
      return(t);
   } // writeFrags

   // Scan while/until a match is found, return count
   int scan (const uint8_t v[], const int nV, const int n, const bool until=false, const uint8_t w=0xAA)
   {
      int i= 0, iV= 0;
      if ((n > 0) && (nV > 0))
      {
         bool test;
         do
         {
            uint8_t b= HSPI.transfer(w);
            test= (b == v[iV]) ^ until;
            if (++iV >= nV) { iV= 0; } // periodic
         } while (test && (++i < n));
      }
      return(i);
   }
}; // CCommonSPIX1

#endif // CCOMMON_SPI_HPP
