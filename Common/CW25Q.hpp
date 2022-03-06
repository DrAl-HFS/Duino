// Duino/Common/CW25Q.hpp - Simple access to Winbond SPI flash device
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#ifndef CW25Q_HPP
#define CW25Q_HPP

#include "DN_Util.hpp"
#include "CCommonSPI.hpp"


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
      GL_UN=0x98, // global unlock
      EE_4K=0x20, EE_32K=0x52, EE_64K=0xD8, EE_DV=0x60,
      // simple commands (single byte)
      WR_EN=0x06, WR_DIS=0x04,   // write enable/disable
      SLEEP=0xB9, WAKE=0xAB      // power management
      };
   enum FlagS1 : uint8_t {
      BUSY=0x01, WEL=0x02,
      BP0=0x04, BP1=0x08, BP2=0x10,
      TB=0x20, SEC=0x40, SRP=0x80
   };
   enum BitsS1 : uint8_t { BPM=0x1C, BPS=2, BPB=3 };
   enum FlagS2 : uint8_t { // Caveat: not on earlier devices...
      SRL=0x01, QE=0x02, RSV1=0x4,
      LB1=0x08, LB2=0x10, LB3=0x20,
      CMP=0x40, SUS=0x80
   };
   enum FlagS3 : uint8_t { // ditto
      RSV2=0x3, WPS=0x04, RSV3=0x18,
      DRV2=0x20, DRV1=0x40, RSV4=0x80
   };
   const int PAGE_BYTES= 256;
}; // namespace W25Q

// Basic access "toolkit"
class CW25Q : public CCommonSPI
{
protected:
   // hex addr fmt : 0xKKSPYY (blocK, Sector, Page, bYte)
   void cmdAddrFrag (const W25Q::Cmd c, const UU32 addr)
   {
      HSPI.transfer(c);
      writeRev(addr.u8,3);
   } // cmdAddrFrag

   void cmd1 (const W25Q::Cmd c)
   {
      start();
      HSPI.transfer(c);
      complete();
   } // cmd1

   uint8_t cmdRW1 (const W25Q::Cmd c, const uint8_t w=0x00)
   {
      uint8_t r;
      start();
      HSPI.transfer(c);
      r= HSPI.transfer(w);
      complete();
      return(r);
   } // cmdRW1

public:
   CW25Q (const uint8_t clkMHz=SPI_CLOCK_DEFAULT) // NB: full clock rate for (84MHz) STM32F4
   {
      spiSet= SPISettings(clkMHz*1000000, MSBFIRST, SPI_MODE0);
   }

   void init (void)
   {
      CCommonSPI::begin();
      cmd1(W25Q::WAKE);
      delayMicroseconds(3); // delay tRES1 before next NCS low
   } // init

   void unlock (void) // Caveat : global!
   {
      cmd1(W25Q::WR_EN);
      cmd1(W25Q::GL_UN);
   } // unlock

   int16_t dataRead (uint8_t b[], int16_t n, const UU32 addr) // UU32
   {
      if (n > 0)
      {
         start();
         cmdAddrFrag(W25Q::RD_PG, addr);
         read(b,n);
         complete();
         return(n);
      }
      return(0);
   } // dataRead

   int16_t dataWrite (const uint8_t b[], int16_t n, const UU32 addr) // UU32
   {
      if (n > 0)
      {
         if (n > 256) { n= 256; }
         cmd1(W25Q::WR_EN);
         start();
         cmdAddrFrag(W25Q::WR_PG, addr);
         write(b,n);
         complete();
      }
      return(n);
   } // dataWrite

   // Page address and count (sector/block size) should be 4K sector aligned
   // ... or else what??
   uint16_t dataErase (uint16_t aP, const uint16_t nP=16)
   {
      W25Q::Cmd cmd;
      if (aP & 0xF) { return(0); }
      switch(nP)
      {
         case 16  : cmd= W25Q::EE_4K; break;  // sector
         case 128 : cmd= W25Q::EE_32K; break; // half block
         case 256 : cmd= W25Q::EE_64K; break; // block
         default : return(0);
      }
      UU32 addr={(uint32_t)aP << 8};
      cmd1(W25Q::WR_EN);
      start();
      cmdAddrFrag(cmd, addr);
      complete();
      return(nP);
   } // dataErase

   bool sync (uint32_t max=-1)
   {
      uint32_t i=0;
      uint8_t s;
      do
      {
         s= cmdRW1(W25Q::RD_ST1);
         if (0 == (s & W25Q::BUSY)) { return(true); }
      } while (++i < max);
      return(false);
   } // sync

}; // CW25Q

// Extended functionality for common problems
class CW25QUtil : public CW25Q
{
public:
   CW25QUtil (const uint8_t clkMHz=SPI_CLOCK_DEFAULT) : CW25Q(clkMHz) { ; }

   // Check for unused storage (or some other constant value)
   // without using a buffer
   uint32_t scanEqual (const UU32 addr, uint32_t max=1<<16, const uint8_t v=0xFF)
   {
      uint32_t n=0;
      uint8_t b;
      start();
      cmdAddrFrag(W25Q::RD_PG, addr);
      do
      {
         b= HSPI.transfer(0x00);
      } while ((v == b) && (++n < max));
      complete();
      return(n);
   } // scanEqual

   uint32_t writeMulti (const UU32 addr, const uint8_t *pp[], const uint8_t b[], const int n)
   {
      if (n <= 0) { return(0); }
      int i=0;
      uint32_t t=0;
      
      start();
      cmdAddrFrag(W25Q::WR_PG, addr);
      do
      {
         uint8_t c= b[i];
         if (t+c > 256) { c= 256 - t; }
         t+= write(pp[i], c);
      } while ((++i < n) && (t < 256));
      complete();
      return(t);
   } // writeMulti

   uint32_t pageDump (Stream& s, const uint16_t aP=0x0000)
   {
      UU32 a={(uint32_t)aP << 8};
      s.print("W25Q:PAGE:"); s.println(aP);
      uint32_t r= pageDump(s,a);
      s.print("bs="); s.println(r);
      return(r);
   } // pageDump

   uint32_t pageDump (Stream& s, const UU32 a)
   {
      uint8_t b[ W25Q::PAGE_BYTES ];
      uint32_t r=0;
      dataRead(b, sizeof(b), a);
      for (uint16_t i= 0; i < sizeof(b); i+= sizeof(uint32_t)) { r+= bitCount32(*(uint32_t*)(b+i)); }
      dumpHexTab(s, b, sizeof(b));
      return(r);
   } // pageDump

}; // CW25QUtil

class CW25QID : public CW25QUtil
{
   // DISPLACE:  where ? not very effective anyway...
   int8_t scanID (const uint8_t id[], const int8_t max)
   {
      int8_t i= 0;
      while ((0x00 != id[i]) && (0xFF != id[i]) && (i < max)) { ++i; }
      return(i);
   } // scanID

public:
   CW25QID (const uint8_t clkMHz=SPI_CLOCK_DEFAULT) : CW25QUtil(clkMHz) { ; }

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
      int8_t n= getMID(id);
      n+= getJID(id+n);
      return(n);
   } // checkID

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
            writeb(0x00,4); // dummy 32clks for sync
            read(b+n,8);
            complete();
            n+= 8;
         }
      }
      return(n);
   } // identify

}; // CW25QID

#include "CMX_Util.hpp" // bitCount32

class CW25QDbg : public CW25QID
{
public:
   uint8_t statf;

   CW25QDbg (const uint8_t clkMHz) : CW25QID(clkMHz), statf{0} { ; }

   void init (Stream& s, uint8_t f=0x1)
   {
      CW25Q::init();
      s.print("W25Q:SPI:NCS="); s.println(PIN_NCS);
      if (identify(s))
      {
         statf= f ;
         dumpStatus(s);
      }
   } // init

   void dumpStatus (Stream& s)
   {
      uint8_t stat[3];

      stat[0]= cmdRW1(W25Q::RD_ST1);
      stat[1]= cmdRW1(W25Q::RD_ST2);
      stat[2]= cmdRW1(W25Q::RD_ST3);
      s.print("W25Q:STAT:");
      dumpHexFmt(s, stat, 3);

      s.print("\nprot: ");
      if (0 == (stat[0] & W25Q::SEC)) { s.print('6'); }
      s.print("4K, ");
      if (0 == (stat[0] & W25Q::TB)) { s.print("top"); } else { s.print("bott"); }
      s.print(", ");
      s.print("BP=0x"); s.print((stat[0] >> W25Q::BPS) & 0x7, HEX);
      s.println();
   } // dumpStatus

   int16_t capacityMb (const uint8_t devID)
   {  // NB: Winbond encoding (not JEDEC)
      if (devID >= 0x14) return(1 << (devID - 0x10));
      if (0x13 == devID) return(80);
      // Older device encoding unknown
      return(-1);
   } // capacityMb

   bool identify (Stream& s)
   {
      int16_t c= -1;
      uint8_t id[16], n;
      n= CW25QID::identify(id,sizeof(id));
      if (n > 0)
      {
         if (0xEF == id[0])
         {
            c= capacityMb(id[1]);
            s.print("W25Q");
         }
         if (c > 0) { s.println(c); } else { s.println('?'); }
         s.print("ID:");
         dumpHexFmt(s, id, n);//, fs);
         s.print('\n');
      }
      return(n > 0);
   } // identify

}; // CW25QDbg

// additional test/debug utils
namespace W25Q
{

   class PageScan
   {
      public:
         UU32      addr;
         uint32_t i, max;

      PageScan (uint32_t n, uint32_t aP=0x0000) { addr.u32= aP << 8; i= 0; max= n; }
      
      void next (Stream& s, CW25QUtil& d)
      {
         const uint32_t size=0x100;
         UU32 a= addr;
         uint32_t t[2], n=16, j=0;
         uint16_t r[16];
         if (i < max)
         {
            t[0]= millis();
            if ((i+n) > max) { n= max-i; }
            do
            {
               r[j]= d.scanEqual(a,size);
               a.u32+= size;
            } while ((r[j] >= size) && (j++ < n));
            t[1]= millis();
            n= j;
            i+= n;
            s.print("0x"); s.print( addr.u32, HEX); s.print(':'); 
            for (j=0; j<n; j++) { s.print(' '); s.print(r[j]); }
            s.println();
            s.print(" dt="); s.println( t[1] - t[0] );
            addr= a;
            if (r[j] < size)
            {
               a.u32-= size;
               d.pageDump(s, a);
            }
         }
      } // next
      
   }; // PageScan
   
}; // namespace W25Q

#endif // CW25Q_HPP
