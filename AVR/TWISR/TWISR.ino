// Duino/AVR/TWISR/TWISR.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#include "Common/AVR/DA_TWISR.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Common/DN_Util.hpp"

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { s.print(sid); }
  s.print(" TWISR " __DATE__ " ");
  s.println(__TIME__);
} // bootMsg

void setup (void)
{
  delay(100);
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  gTWI.set(TW_CLK_400);
  
  //DEBUG.print("sizeof(fragB)="); DEBUG.println(sizeof(fragB));
} // setup

uint16_t gIter=0;
const char s1[]="* Lorem ipsum dolor sit amet *";
uint8_t gB[40];
const uint8_t q= 32;

void setAddr (uint8_t b[], const uint16_t a)
{
  b[0]= a >> 8; 
  b[1]= a & 0xFF; // form (big endian) page address
} // setAddr

char fillTest (uint8_t b[], uint8_t endSwap=0)
{
  uint8_t t[2];
  
  gTWI.write(0x50,b,2); // set page address
  gTWI.read(0x50,t,2); // retrieve 2 Bytes
  gTWI.sync();
  if ((b[endSwap^1] != t[1]) || (b[endSwap] != t[0])) // Check for stored page address match
  {
    b[2]= b[endSwap]; b[3]= b[endSwap^1]; // page begins with its own address, possibly swapped endian order
    //for (int8_t i=0; i<q; i++) { b[i]= q+1-i; }
    if (b[4] != s1[0]) { memcpy(b+4, s1, sizeof(s1)); } // Fill with text as necessary
    gTWI.write(0x50, b, 2+q); // store 32bytes (prefixed with BE page address)
    return('W');
  }
  return('-');
} // fillTest

void loop (void)
{
  if (gTWI.sync(false))
  {
    setAddr(gB, (gIter & 0x7F) << 5);
    char c= fillTest(gB); // 4kB -> 128pages of 32Bytes
    DEBUG.println(c);
    if (gTWI.sync(false))
    {
      gTWI.write(0x50,gB,2); // reset page
      gTWI.read(0x50,gB+2,q); // retrieve 32 Bytes
      gTWI.sync(); // delay(50);
      dumpHexTab(DEBUG, gB+2, q);
      gIter++;
    }
  }
  DEBUG.println(gIter);
  delay(150);
  //gTWI.dump(DEBUG);
  delay(350);
} // loop
