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
class CW25Q : public CCommonSPIX1
{
   struct EraseParam { uint16_t nP; int8_t nE; W25Q::Cmd cmd; };

protected:

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

   // hex addr fmt : 0xKKSPYY (blocK, Sector, Page, bYte)
   void cmdAddrFrag (const UU32 addr, const W25Q::Cmd c)
   {
      start();
      HSPI.transfer(c);
      writeRev(addr.u8,3);
   } // cmdAddrFrag

   void startWrite (const UU32 addr, const W25Q::Cmd cmd)
   {
      sync();
      cmd1(W25Q::WR_EN);
      //start();
      cmdAddrFrag(addr,cmd);
   } // startWrite

   // CAVEAT : no checking of address or command!
   void erase (const UU32 addr, const W25Q::Cmd cmd)
   {
      //sync();
      //cmd1(W25Q::WR_EN);
      //start();
      //cmdAddrFrag(cmd, addr);
      startWrite(addr,cmd);
      complete();
   } // erase

   // Require page address and count (sector/block size) to be 4K sector aligned
   bool validErase (EraseParam& m, const uint16_t aP, const uint16_t nP)
   {
      m.nE= 0;
      if ((0 == (aP & 0xF)) && (0 == (nP & 0xF)))
      {  // Faster to erase more with a single command?
         if (nP >= 256)
         {
            m.nP= 256;
            m.nE= (nP >> 8) & 0x7F;
            m.cmd= W25Q::EE_64K; // full block
         }
         else if (nP >= 128)
         {
            m.nP= 128;
            m.nE= nP >> 7; // irrelevant
            m.cmd= W25Q::EE_32K; // half block
         }
         else if (nP >= 16)
         {
            m.nP= 16;
            m.nE= nP >> 4;
            m.cmd= W25Q::EE_4K; // sector
         }
      }
      return(m.nE > 0);
   } // validErase

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

   int dataRead (uint8_t b[], int n, const UU32 addr) // UU32
   {
      if (n > 0)
      {
         //start();
         cmdAddrFrag(addr, W25Q::RD_PG);
         read(b,n);
         complete();
         return(n);
      }
      return(0);
   } // dataRead

   int dataWrite (const uint8_t b[], int n, const UU32 addr) // UU32
   {
      if (n > 0)
      {
         if (n > 256) { n= 256; }
         //sync();
         //cmd1(W25Q::WR_EN);
         //start();
         //cmdAddrFrag(W25Q::WR_PG, addr);
         startWrite(addr, W25Q::WR_PG);
         write(b,n);
         complete();
      }
      return(n);
   } // dataWrite

   int dataErase (uint16_t aP, const int nP=16)
   {
      EraseParam m;
      int eP=0;
      while ((eP < nP) && validErase(m,aP,nP))
      {
         do
         {
            UU32 addr={ (uint32_t)aP << 8 };

            erase(addr,m.cmd);
            aP+= m.nP;
            eP+= m.nP;
         } while (--m.nE);
      }
      return(eP);
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
/*   friend struct SplitFrag
   {
     const uint8_t *pF; uint8_t h, t;

      SplitFrag (void) : pF{NULL}, h{0}, t{0} {;}

      bool set (const uint8_t *p, const uint8_t l, const int r)
      {
         if ((NULL == pF) && (0 == h) && (0 == t))
         {
            pF= p; h= r; t= l - r;
            return(true);
         }
         return(false);
      } // set

      int writeH (CCommonSPI& dev)
      {
         int w=0;
         if (pF && (h > 0))
         {
            w= dev.write(pF,h);
            if (t > 0) { pF+= h; }
            else { pF= NULL; } // weird?
            h= 0;
         }
         return(w);
      } // writeH

      int writeT (CCommonSPI& dev)
      {
         int w=0;
         if (pF && (0 == h) && (t > 0))
         {
            w= dev.write(pF,t);
            pF= NULL;
            t= 0;
         }
         return(w);
      } // writeT

   }; // struct SplitFrag
*/
   int numFragsWithin (const uint8_t lF[], const int nF, const int max)
   {
      int n=0;
      if (nF > 0)
      {
         int sum= lF[0];
         while ((++n < nF) && ((sum+= lF[n]) <= max)); // { sum+= lF[n]; }
         //if (sum > max) { split.b= sum-max; split.a= lF[n] - split.b; }
      }
      return(n);
   } // numFragsWithin

   // CAVEAT: no address&size checking - beware circular wrap!
   int dataWriteF (const UU32 addr, const uint8_t * const ppF[], const uint8_t lF[], const int nF)
   {
      int t;
      startWrite(addr, W25Q::WR_PG);
      t= writeFrags(ppF, lF, nF);
      complete();
      return(t);
   } // dataWriteF
/*
   int dataWriteFS (const UU32 addr, const uint8_t * const ppF[], const uint8_t lF[], const int nF, Split& s)
   {
      int t;
      startWrite(addr, W25Q::WR_PG);
      t= s.writeT();  // Yech...
      t+= writeFrags(ppF+iF, lF+iF, n);
      t+= s.writeH();
      complete();
      return(t);
   } // dataWriteFS
*/
public:
   CW25QUtil (const uint8_t clkMHz=SPI_CLOCK_DEFAULT) : CW25Q(clkMHz) { ; }

   // Check for unused storage (or some other constant value) without using a buffer
   int dataScan (const UU32 addr, const int max=1<<12, const uint8_t v=0xFF, const bool until=false)
   {
      cmdAddrFrag(addr,W25Q::RD_PG);
      int n= CCommonSPIX1::scan(&v,1,max,until);
      complete();
      return(n);
   } // dataScan

   // Gathered fragment write :
   // Write fragments as continuous sequence, ? over pages ?
   // CAVEAT: device write buffer is *circular* 256Bytes
   int dataWriteFrags (UU32 addr, const uint8_t * const ppF[], const uint8_t lF[], const int nF)
   {
      int n= numFragsWithin(lF, nF, W25Q::PAGE_BYTES - addr.u8[0]);
      if (n == nF) { return dataWriteF(addr,ppF,lF,nF); }
      else { // not properly figured out yet...
      }
      /*
      int t=0, iF=0;
      if (nF > 0)
      {
         //SplitFrag split;
         do
         {
               //{ i-= !split.set(ppF[i],lF[i],rem); }
               int n= i-iF; // NB: already i+1
               iF+= n;
               t+= t0;
               addr.u32+= t0;
         } while (iF < nF);
      }
      return(t);
      */
      return(0);
   } // dataWriteFrags

// DISPLACE -> CW25QUtilA ???
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

   uint16_t dataEraseDirty (uint16_t aP, const uint16_t nP=16)
   {
      UU32 addr={ (uint32_t)aP << 8 };
      uint16_t r= 0;
      if (dataScan(addr,256) < 256)
      {
         r= dataErase(aP,nP);
         sync();
      }
      return(r);
   } // dataEraseDirty

}; // CW25QDbg

// additional test/debug utils
namespace W25Q
{

   class PageScan
   {
      public:
         UU32      base, limit, addr;
         uint32_t i, max, size;

      PageScan (uint32_t aP=0x0000, uint32_t gB=0x100, uint32_t bP=0xFFFF)
      {
         base.u32= aP << 8;
         size= gB;
         limit.u32= bP << 8;
         max= (limit.u32 - base.u32) / size;
         reset();
      }

      void reset (void) { addr= base; i= 0; }

      void next (Stream& s, CW25QUtil& d)
      {
         UU32 a= addr;
         uint32_t t[2], n=16, j=0;
         uint16_t r[16];
         if (i < max)
         {
            if ((i+n) >= max) { n= max - (1+i); }
            t[0]= millis();
            do
            {
               r[j]= d.dataScan(a,size);
               a.u32+= size;
            } while ((r[j] >= size) && (++j < n));
            t[1]= millis();
            n= j; i+= n;
            s.print("0x"); s.print( addr.u32, HEX); s.print(": "); // 24bit address BB:SP:bb = (64k) Block, (4k) Sector, Page, byte
            s.print(r[0]);
            for (j=1; j<n; j++) { s.print(' '); s.print(r[j]); }
            s.print(" dt="); s.println( t[1] - t[0] );
            addr= a;
            if (r[n-1] < size)
            {
               if (n > 1) { s.print("0x"); s.print( a.u32, HEX); s.print(':'); }
               if ((0x100 == size) || (r[n-1] < 0x100))
               {
                  s.println();
                  a.u32-= size;
                  d.pageDump(s, a);
               }
               //else { s.println(r[j]); }
            }
         }
      } // next
   }; // PageScan

}; // namespace W25Q

#endif // CW25Q_HPP
