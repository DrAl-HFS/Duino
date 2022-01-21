// Duino/Common/CW25Q.hpp - Simple access to Winbond SPI flash device
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#ifndef CW25Q_HPP
#define CW25Q_HPP

#include "DN_Util.hpp"

#include "CCommonSPI.hpp"
#if 0
//#include "MBD/mbdDef.h" // UU16/32
#else
typedef union { uint32_t u32; uint16_t u16[2]; uint8_t u8[4]; } UU32;
#endif

namespace W25Q
{
   enum Cmd : uint8_t {
      RD_MID=0x90, RD_JID=0x9F, RD_UID=0x4B,  // identity
   // read/write
       // status bytes
      RD_ST1=0x05, RD_ST2=0x35, RD_ST3=0x15,
      WR_ST1=0x01, WR_ST2=0x31, WR_ST3=0x11,

      RD_PG=0x03, WR_PG=0x02,    // data page (256 bytes)
      RF_PG=0x0B,                // "read fast" allows up to 133MHz SPI clock (requires very high quality signal path)
      // erase sector/block/device
      EE_4K=0x20, EE_32K=0x52, EE_64K=0xD8, EE_DV=0x60,
      // simple commands (single byte)
      WR_EN=0x06, WR_DIS=0x04,   // write enable/disable
      SLEEP=0xB9, WAKE=0xAB      // power management
      };
   const int PAGE_BYTES= 256;
}; // namespace W25Q

class CW25Q : public CCommonSPI
{
protected:
   int8_t scanID (const uint8_t id[], const int8_t max)
   {
      int8_t i= 0;
      while ((0x00 != id[i]) && (0xFF != id[i]) && (i < max)) { ++i; }
      return(i);
   } // scanID

   void cmdAddr (const W25Q::Cmd c, const UU32 addr)
   {
      HSPI.transfer(c);
      writeRev(addr.u8,3);
   } // cmdAddr

   void cmd1 (const W25Q::Cmd c)
   {
      start();
      HSPI.transfer(c);
      complete();
   } // cmd1

   uint8_t cmdRd1 (const W25Q::Cmd c)
   {
      uint8_t r;
      start();
      HSPI.transfer(c);
      r= HSPI.transfer(0x00);
      complete();
      return(r);
   } // cmdRd1

   int8_t getMID (uint8_t mid[2])
   {
      start();
      HSPI.transfer(W25Q::RD_MID);
      writeb(0x00,3); // dummy address
      read(mid,2);
      complete();
      return scanID(mid,2);
   } // getMID

   int8_t getJID (uint8_t jid[3]) // Manufacturer,Type,Capacity
   {
      start();
      HSPI.transfer(W25Q::RD_JID);
      //delayMicroseconds(100);
      read(jid,3);
      complete();
      return scanID(jid,3);
   } // getJID


   int8_t checkID (uint8_t id[5])
   {
      int8_t n= getJID(id);
      if (0xEF == id[0]) { n+= getMID(id+n); }
      return(n);
   } // checkID

public:
   CW25Q (const uint8_t clkMHz=42) // NB: full clock rate for (84MHz) STM32F4
   {
      spiSet= SPISettings(clkMHz*1000000, MSBFIRST, SPI_MODE0);
   }

   void init (void)
   {
      CCommonSPI::begin();
      cmd1(W25Q::WAKE); 
      delayMicroseconds(3); // delay tRES1 before next NCS low
   } // init

   int8_t identify (uint8_t b[], int8_t maxB=13)
   {
      int8_t n= 0;
      if (maxB >= 5)
      {
         n= checkID(b);
         if ((n > 0) && (maxB >= (n+8)))
         {
            start();
            HSPI.transfer(W25Q::RD_UID);
            writeb(0x00,4);
            read(b+n,8);
            complete();
            n+= 8;
         }
      }
      return(n);
   } // identify

   void dataRead (uint8_t b[], int16_t n, UU32 addr) // UU32
   {
      start();
      cmdAddr(W25Q::RD_PG, addr);
      read(b,n);
      complete();
   } // dataRead

}; // CW25Q


class CW25QDbg : public CW25Q
{
public:
   uint8_t stat[3];

   CW25QDbg (const uint8_t clkMHz) : CW25Q(clkMHz) { ; }

   void init (Stream& s)
   {
      CW25Q::init();
      s.print("W25Q:SPI:NCS="); s.println(PIN_NCS);
      identify(s);
   } // init

   void status (Stream& s)
   {
      stat[0]= cmdRd1(W25Q::RD_ST1);
      stat[1]= cmdRd1(W25Q::RD_ST2);
      stat[2]= cmdRd1(W25Q::RD_ST3);
      s.print("W25Q:STAT:");
      dumpHexFmt(s, stat, 3);
   } // status
   
   int16_t capacityMb (const uint8_t devID)
   {
      if (devID >= 0x14) return(1 << (devID - 0x10));
      if (0x13 == devID) return(80);
      return(-1);
   } // capacityMb
   
   void identify (Stream& s)
   {
      uint8_t id[13], n;
      n= CW25Q::identify(id,sizeof(id));
      if (n > 0)
      {
         //char fs[]="   ";
         if (0xEF == id[0])
         {
            uint8_t c= id[2];
            s.print("W25Q");
            if (0xEF == id[3]) { c= id[4]; }
            s.println(capacityMb(c));
         } else { s.println('?'); }
         s.print("ID:");
         dumpHexFmt(s, id, n);//, fs);
         s.print('\n');
      }
   } // identify

   void dumpPage (Stream& s, uint16_t nP)
   {
      uint8_t b[ W25Q::PAGE_BYTES ];
      UU32 a={(uint32_t)nP << 8};
      dataRead(b, sizeof(b), a);
      s.print("W25Q:PAGE:"); s.println(nP);
      dumpHexTab(s, b, sizeof(b));
   } // dumpPage

}; // CW25QDbg

#endif // CW25Q_HPP
