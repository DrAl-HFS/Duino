// Duino/Common/CAT24C.hpp - class wrapper for Atmel I2C EEPROM
// https://github.com/DrAl-HFS/Duino.git 
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef CAT24C_HPP
#define CAT24C_HPP

#include "DN_Util.hpp"
#include "CCommonI2C.hpp"


namespace AT24CHW
{
  enum Device : uint8_t { BASE_ADDR=0x50 }; // NB: 7msb -> 0xA0|RNW on wire
}; // namespace AT24CHW

#ifndef UU16
extern "C" { typedef union {uint16_t u16; uint8_t u8[2]; } UU16; }
#endif

class CAT24C : public CCommonI2CX1
{
protected:
   uint8_t devAddr (void) { return(AT24CHW::BASE_ADDR); }
   
   void setAddr (const UU16 a) { writeToRev(devAddr(), a.u8, 2); }
   
   void setPageAddr (uint8_t page) { UU16 a; a.u16= page<<5; setAddr(a); }
   
public:
   CAT24C (void) { ; }
   
   int readPage (const uint8_t page, uint8_t b[], int n)
   {
      setPageAddr(page);
      return readFrom(devAddr(), b, n);
   } // readPage

   int writeAddr (const UU16 a, const uint8_t b[], int n)
   {
      return(writeToRevThenFwd(devAddr(), a.u8, 2, b, n) - 2);
   }
   
   bool sync (void) { delay(10); return(true); }
   
   int writePage (const uint8_t page, const uint8_t b[], int n)
   {
      UU16 a={page<<5};
      return writeAddr(a,b,n);
   } // readPage
   
   void dump (Stream& s, uint8_t page)
   {
      uint8_t b[32]; // Wire internal buffer
      uint16_t t[2];
      int q=sizeof(b), r;
      t[0]= millis();
      r= readPage(page, b, sizeof(b));
      t[1]= millis(); //  
      s.print("CAT24C::dump() - page,q,r= "); s.print(page); s.print(','); s.print(q); s.print(','); s.print(r); s.println(':');
      dumpHexTab(s, b, r);
      s.print("dt="); s.println(t[1]-t[0]); 
   } // dump
   
}; // class CAT24C

#endif // CAT24C_HPP
