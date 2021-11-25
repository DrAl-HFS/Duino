// Duino/Common/CCommonSPI.hpp - 'Duino encapsulation of functionality common to many SPI interfaces
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2021

#ifndef CCOMMON_SPI_HPP
#define CCOMMON_SPI_HPP

//include "DN_Util.hpp"
//include <SPI.h>
#ifndef HSPI
#define HSPI SPI
#endif

// General abstraction of device hard/soft states
namespace Device
{
   enum HW : int8_t { FAIL, UNKNOWN, OFF, STANDBY, READY, BUSY, RESET, CALIBRATE }; //, INVALID }; // 4b , LOCK=0x8
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
   Device::HW getHW (void) { return((Device::HW)(hws & 0x7)); }

   void tick (void) { hsc+= (hsc<0xFF); }

   void setHW (Device::HW s)
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

class CCommonSPI
{
protected:
   SPISettings spiSet;

   CCommonSPI (void) { ; }

   void start (void) { HSPI.beginTransaction(spiSet); digitalWrite(PIN_NCS,0); }
   void complete (void) { digitalWrite(PIN_NCS,1); HSPI.endTransaction(); }

   // NB: transfer((void*), int); is read-write in same buffer ...
   // Beware of sending "dummy" bytes for reading: some devices
   // may interpret certain bytes as a command e.g. causing a reset

   uint16_t read (uint8_t b[], const uint16_t n, const uint8_t w=0xAA)
   {
      for (uint16_t i=0; i<n; i++) { b[i]= HSPI.transfer(w); }
      return(n);
   } // write

   // Reverse (endian) order
   uint16_t readRev (uint8_t b[], const uint16_t n, const uint8_t w=0xAA)
   {
      uint16_t i= n;
      while (i-- > 0) { b[i]= HSPI.transfer(w); }
      return(n);
   } // write

   uint16_t writeb (const uint8_t b, const uint16_t n=1)
   {
      for (uint16_t i=0; i<n; i++) { HSPI.transfer(b); }
      return(n);
   } // writeb

   uint16_t write (const uint8_t b[], const uint16_t n)
   {
      for (uint16_t i=0; i<n; i++) { HSPI.transfer(b[i]); }
      return(n);
   } // write

   // Reverse (endian) order
   uint16_t writeRev (const uint8_t b[], const uint16_t n)
   {
      uint16_t i= n;
      while (i-- > 0) { HSPI.transfer(b[i]); }
      return(n);
   } // write

}; // CCommonSPI

#endif // CCOMMON_SPI_HPP
