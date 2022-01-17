// Duino/Common/CW25Q.hpp - Basic test hacks for Winbond SPI flash device
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#ifndef CW25Q_HPP
#define CW25Q_HPP

#include "CCommonSPI.hpp"

namespace W25Q
{
   enum Cmd : uint8_t {
      RD_JID=0x9F, RD_UID=0x4B,  // identity
      // read/write
      RD_ST=0x05, WR_ST=0x01,    // status
      RD_PG=0x03, WR_PG=0x02,    // data page (256 bytes)
      WR_EN=0x06, WR_DIS=0x04,   // write enable/disable
      // erase sector/block/device
      EE_4K=0x20, EE_32K=0x52, EE_64K=0xD8, EE_DV=0x60,
      SLEEP=0xB9, WAKE=0xAB      // power management
      };
}; // namespace W25Q
   
class CW25Q : public CCommonSPI
{
protected:
   
public:
   CW25Q (uint8_t clkMHz=1)
   {
      spiSet= SPISettings(clkMHz*1000000, MSBFIRST, SPI_MODE0);
   }
  
   using CCommonSPI::begin;
   
   int8_t identify (uint8_t b[], int8_t maxB=11)
   {
      int8_t n= 0;
      if (maxB >= 3)
      {
         start();
         writeb(W25Q::RD_JID);
         read(b,3);
         complete();
         n+= 3;
         if (maxB >= (n+8))
         {
            start();
            writeb(W25Q::RD_UID);
            writeb(0x00,4);
            read(b+n,8);
            complete();
            n+= 8;
         }
      }
      return(n);
   } // identify
   
}; // CW25Q

class CW25QDbg : public CW25Q
{
public:
   //CW25QDbg (c) : CW25Q(c) { ; }
   
   void begin (Stream& s)
   {
      CCommonSPI::begin();
      s.print("NCS="); s.println(PIN_NCS);
   }
}; // CW25QDbg

#endif // CW25Q_HPP
