// Duino/Common/CCommonSPI.hpp - 'Duino encapsulation of functionality common to many SPI interfaces
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2021 - Jan 2022

#ifndef CCOMMON_SPI_HPP
#define CCOMMON_SPI_HPP

//include <SPI.h>
#ifndef HSPI
#define HSPI SPI
#endif

#ifndef PIN_NCS
#define PIN_NCS   SS
#endif

#ifdef ARDUINO_ARCH_STM32F1
#define SPI_CLOCK_DEFAULT  36 // MHz
#endif

#ifdef ARDUINO_ARCH_STM32F4
#define SPI_CLOCK_DEFAULT  42 // MHz
#endif

#ifndef SPI_CLOCK_DEFAULT
#define SPI_CLOCK_DEFAULT 8
#endif

// General abstraction of device hard/soft states
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
   // may interpret certain bytes as a command e.g. causing a reset

   uint16_t read (uint8_t b[], const int16_t n, const uint8_t w=0xAA)
   {
      int16_t i;
      for (i=0; i<n; i++) { b[i]= HSPI.transfer(w); }
      return(i);
   } // write

   // Reverse (byte-endian) order
   int16_t readRev (uint8_t b[], const int16_t n, const uint8_t w=0xAA)
   {
      if (n <= 0) { return(0); }
      int16_t i= n;
      while (i-- > 0) { b[i]= HSPI.transfer(w); }
      return(n);
   } // write

   int16_t writeb (const uint8_t b, const int16_t n=1)
   {
      int16_t i= 0;
      for (; i<n; i++) { HSPI.transfer(b); }
      return(i);
   } // writeb

   int16_t write (const uint8_t b[], const int16_t n)
   {
      int16_t i= 0;
      for (; i<n; i++) { HSPI.transfer(b[i]); }
      return(i);
   } // write

   // Reverse (endian) order
   int16_t writeRev (const uint8_t b[], const int16_t n)
   {
      if (n <= 0) { return(0); }
      int16_t i= n;
      while (i-- > 0) { HSPI.transfer(b[i]); }
      return(n);
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

#endif // CCOMMON_SPI_HPP
