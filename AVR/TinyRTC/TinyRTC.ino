// Duino/AVR/TinyRTC/TinyRTC.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#include "Common/AVR/DA_Config.hpp"
#include "Common/CDS1307.hpp"
#include "Common/CAT24C.hpp"

CDS1307A gRTC;
CAT24C gERM;

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { s.print(sid); }
  s.print(" TinyRTC " __DATE__ " ");
  s.println(__TIME__);
} // bootMsg

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  I2C.begin(); // join i2c bus (address optional for master)
  //gRTC.testDays(DEBUG,__DATE__);
  gRTC.printDate(DEBUG,' ');
  gRTC.printTime(DEBUG);
  //analogReadResolution(8);
  pinMode(A7,INPUT);//ANALOGUE);
  
#ifdef TWI_BUFFER_LENGTH
  DEBUG.print("I2C buffers:"); DEBUG.print(TWI_BUFFER_LENGTH); DEBUG.print(','); DEBUG.println(BUFFER_LENGTH);
#endif
} // setup

void buffTest (void)
{
  //const char s1[]="Lorem ipsum dolor sit amet";
  //const char s2[]="consectetur adipiscing elit, sed do eiusmod"; // truncated by Wire buffer
  uint8_t b[32];
  int r, q= sizeof(b);
  for (int i=0; i<q; i++) { b[i]= 1+i; }
  r= gERM.writePage(5,b,q);
  DEBUG.print("gERM.writePage() - q,r= ");
  DEBUG.print(q); DEBUG.print(','); DEBUG.println(r);
  if (r > 0) { gERM.sync(); }
} // buffTest

uint16_t gIter=0;
int ds= 15, err=3;

void loop (void)
{
  int r=0;
  
  if (0 == gIter) { buffTest(); }
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
  r= gRTC.printTime(DEBUG,' '); DEBUG.print(": vB="); DEBUG.println(vB);
  delay(1000);
  if (r < 0) 
  {
    gRTC.dump(DEBUG);
    if (err-- <= 0)
    {
      r= gRTC.setA(__TIME__,NULL,__DATE__,0);
      DEBUG.print("gRTC.setA() -> "); DEBUG.println(r); 
      err= 3;
    }
  }
  gIter++;
  gERM.dump(DEBUG, gIter & 0x7); // 0x7F 128*32= 4k
} // loop
