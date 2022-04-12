// Duino/AVR/TWISR/TWISR.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#if 0
#include "TWI/CTWI.hpp"
#else
#include "Common/AVR/DA_TWISR.hpp"
#endif
#include "Common/AVR/DA_Config.hpp"
#include "Common/DN_Util.hpp"
//#include "Common/Timing.hpp"

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
  gTWI.set(); // TW_CLK_400
  
  //DEBUG.print("sizeof(fragB)="); DEBUG.println(sizeof(fragB));
} // setup

DNTimer gT(500);
uint8_t gFlags=0x1;

uint16_t gIter=0;
const char *gS[]=
{
  "* Lorem ipsum dolor sit amet *",
  "* Nemo enim ipsam voluptatem *"
};
const uint8_t qS= 30;
uint8_t gB[40];
const uint8_t qP= 32;
char gEv=0;

void setAddr (uint8_t b[], const uint16_t a)
{
  b[0]= a >> 8; 
  b[1]= a & 0xFF; // form (big endian) page address
} // setAddr

char fillTest (uint8_t b[], uint8_t endSwap=0)
{
  const uint8_t i= ((b[1] & 0x40) > 0);
  uint8_t t[6];
  
  gEv= 0;
  gTWI.write(0x50,b,2); // set page address
  gTWI.sync(-1);
  gTWI.read(0x50,t,sizeof(t)); // retrieve leading Bytes
  gTWI.sync(-1);
  if ((b[endSwap^1] != t[1]) || (b[endSwap] != t[0]) || (gS[i][3] != t[5])) // Check for stored page content match
  {
    b[2]= b[endSwap]; b[3]= b[endSwap^1]; // page begins with its own address, possibly swapped endian order
    if (gS[i][3] != b[7]) { memcpy(b+4, gS[i], qS); } // Fill with text as necessary
    gTWI.write(0x50, b, 2+qP); // store 32bytes (prefixed with BE page address)
    gEv= 'W';
  }
  return(gEv);
} // fillTest

void loop (void)
{
  if ((gFlags & 0x1) && gTWI.sync())
  {
    setAddr(gB, (gIter & 0x7F) << 5);
    fillTest(gB,1); // 4kB -> 128pages of 32Bytes
    gFlags&= ~0x1;
    gFlags|= 0x2;
  }
  if ((gFlags & 0x2) && gTWI.sync(-1))
  {
    if ('W' == gEv) { delay(10); }
    gTWI.write(0x50,gB,2); // reset page
    gTWI.sync(-1);
    gTWI.read(0x50,gB+2,qP); // retrieve page (32 Bytes)
    gTWI.sync(-1);
    dumpHexTab(DEBUG, gB+2, qP);
    gFlags&= ~0x2;
  }
  if (gT.update())
  {
    DEBUG.println(gIter);
    gTWI.dump(DEBUG); gTWI.clrEv();
    gFlags|= 0x1;
    gIter++;
  }
} // loop
