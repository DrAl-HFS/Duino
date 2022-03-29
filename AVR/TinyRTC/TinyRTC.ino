// Duino/AVR/TinyRTC/TinyRTC.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#include "Common/AVR/DA_Config.hpp"
#include "Common/CDS1307.hpp"

namespace AT24CHW
{
  enum Device : uint8_t { BASE_ADDR=0x50 };
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
      uint8_t b[32]; // Wire internal buffer size?
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

CDS1307A gRTC;
CAT24C gERM;

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { DEBUG.print(sid); }
  DEBUG.print(" TinyRTC " __DATE__ " ");
  DEBUG.println(__TIME__);
} // bootMsg

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  I2C.begin(); // join i2c bus (address optional for master)
  //uint8_t b[2]={ DS1307HW::DOW, 5 }; gRTC.writeTo(0x68,b,2);
  gRTC.testDays(DEBUG,__DATE__);
  int r= gRTC.setA(__TIME__,NULL,__DATE__);
  DEBUG.print("gRTC.setA() -> "); DEBUG.println(r); 
  gRTC.printDate(DEBUG,' ');
  gRTC.printTime(DEBUG);
  gRTC.dump(DEBUG);
#if 0
  {
    //const char s1[]="Lorem ipsum dolor sit amet";
    //const char s2[]="consectetur adipiscing elit, sed do eiusmod"; // truncated by Wire buffer
    uint8_t b[32];
    int q= sizeof(b);
    for (int i=0; i<q; i++) { b[i]= 1+i; }
    r= gERM.writePage(4,b,q);
    DEBUG.print("gERM.writePage() - q,r= ");
    DEBUG.print(q); DEBUG.print(','); DEBUG.println(r);
    if (r > 0) { gERM.sync(); }
  }
#endif
  //analogReadResolution(8);
  pinMode(A7,INPUT);//ANALOGUE);
} // setup

uint16_t gIter=0;
int ds= 15;

void loop (void)
{
#if 0
  if (0 != ds) 
  { 
    DEBUG.print("ds="); DEBUG.print(ds);
    ds-= gRTC.adjustSec(ds); // duff
    DEBUG.print(" -> "); DEBUG.println(ds);
  }
/*
  char c[4]="00,";
  uint8_t ss[3]={0,0,0};
  gRTC.readTimeBCD(ss,1); 
  for (int i=0; i<3; i++)
  {
    hexChFromU8(c,ss[i]);
    DEBUG.print(c);
  }
  DEBUG.println();
*/
#endif
  int vB= analogRead(A7);
  gRTC.printTime(DEBUG,' '); DEBUG.print(": vB="); DEBUG.println(vB);
  gIter++;
  gERM.dump(DEBUG, gIter & 0x7); // 0x7F 128*32= 4k
  delay(1000);
} // loop
