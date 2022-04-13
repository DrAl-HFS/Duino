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

uint16_t gIter=0;
const char *gS[]=
{
  "* Lorem ipsum dolor sit amet *",
  "* Nemo enim ipsam voluptatem *"
};
const uint8_t qS= 30;
uint8_t gB[40];
const uint8_t qP= 32;
char gEv=0x00;
uint8_t gFlags=0x01;

void setAddr (uint8_t b[], const uint16_t a)
{
  b[0]= a >> 8; 
  b[1]= a & 0xFF; // form (big endian) page address
} // setAddr

DNTimer gT(500); // 500ms / 2Hz


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
  gTWI.setClk(TWI::CLK_400);
  setAddr(gB, 0);
  //DEBUG.print("sizeof(fragB)="); DEBUG.println(sizeof(fragB));
} // setup


char fillTest (Stream& s, uint8_t b[], uint8_t endSwap=0)
{
  const uint8_t i= ((b[1] & 0x40) > 0);
  uint8_t t[6];
  
  gEv= 0x0;
  if (gTWI.writeSync(0x50,b,2)) // set page address
  {
    for (int8_t i=0; i<sizeof(t); i++) { t[i]= 0xA5; }
    if (gTWI.readSync(0x50,t,sizeof(t))) // retrieve leading Bytes
    {
      s.print("Hdr:"); dumpHexFmt(s, t, 6);
      gEv= '-';
      if ((b[endSwap^1] != t[1]) || (b[endSwap] != t[0]) || (gS[i][3] != t[5])) // Check for stored page content match
      {
        b[2]= b[endSwap]; b[3]= b[endSwap^1]; // page begins with its own address, possibly swapped endian order
        if (gS[i][3] != b[7]) { memcpy(b+4, gS[i], qS); } // Fill with text as necessary
        gTWI.write(0x50, b, 2+qP); // store 32bytes (prefixed with BE page address)
        gEv= 'W';
      }
    }
  }
  s.println(gEv);
  return(gEv);
} // fillTest

void loop (void)
{
  if (gT.update()) { gFlags|= 0x5; }
  if (gFlags & 0x1)
  {
    uint8_t r[2], t;
    if (0x0 != gEv) { gTWI.sync(-1); }
    if ('W' == gEv) { delay(10); }
    r[0]= gTWI.writeSync(0x50,gB,2); // select page
    memset(gB+2, 0xA5, sizeof(gB)-2);
    t= 3;
    do
    {
      r[1]= gTWI.readSync(0x50,gB+2,qP); // retrieve data (32 Bytes)
      if (r[1] <= 0) { delay(1); }
    } while ((r[1] <= 0) && (t-- > 0));
    DEBUG.print("R:");
    for (int8_t i=0; i<2; i++) { DEBUG.print(' '); DEBUG.print(r[i]); }
    DEBUG.print('('); DEBUG.print(t); DEBUG.println(')');
    if (r[1] > 0)
    {
      dumpHexTab(DEBUG, gB+2, qP);
    } else { gTWI.dump(DEBUG); }
    gFlags&= ~0x1;
  }
  if ((gFlags & 0x2) && gTWI.sync())
  {
    fillTest(DEBUG,gB,1); // 4kB -> 128pages of 32Bytes
    gFlags&= ~0x2;
  }
  if (gFlags & 0x4)
  {
    setAddr(gB, (++gIter & 0x7F) << 5);
    DEBUG.print('I'); DEBUG.print(gIter); DEBUG.print(": ");
    gTWI.dump(DEBUG); gTWI.clrEv();
    gFlags&= ~0x4;
  }
} // loop
