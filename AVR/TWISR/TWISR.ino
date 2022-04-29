// Duino/AVR/TWISR/TWISR.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#define PIN_PWR 13 // Nano D13

//#include "TWI/KK/CTWI.hpp"
//#include "TWI/SN/CTWI.hpp"
#include "Common/DN_Util.hpp"
#include "Common/AVR/DA_TWUtil.hpp"
#include "Common/AVR/DA_Config.hpp"
//#include "Common/Timing.hpp"

uint16_t gIter=0;
const char *gS[]=
{
  "* Lorem ipsum dolor sit amet *",
  "* Nemo enim ipsam voluptatem *"
};
const uint8_t qS= 30;
const uint8_t qP= 32;
char gEv=0x00;
uint8_t gFlags=0x01;

void setAddr (uint8_t b[], const uint16_t a)
{
  b[0]= a >> 8;
  b[1]= a & 0xFF; // form (big endian) page address
} // setAddr

DNTimer gT(500); // 500ms / 2Hz

void pwrRst (void)
{
  pinMode(PIN_PWR, OUTPUT); // force power transition to reset, prevent sync fail
  digitalWrite(PIN_PWR, LOW); // power off
  delay(50);
  digitalWrite(PIN_PWR, HIGH); // power on
  delay(50);
} // pwrRst

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { s.print(sid); }
  s.print(" TWISR " __DATE__ " ");
  s.println(__TIME__);
} // bootMsg

void setup (void)
{
  pwrRst();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  gTWI.reset(TWUtil::CLK_400);
  interrupts();
  for (int8_t i=0; i<sizeof(gTWTB); i++) { gTWTB[i]= 0xA5; }
  //uint32_t f= gTWI.set(200000);
  //DEBUG.print("setClk() -> "); DEBUG.println(f);
  setAddr(gTWTB, 0);
  //DEBUG.print("sizeof(fragTWTB)="); DEBUG.println(sizeof(fragTWTB));
} // setup


char fillTest (Stream& s, uint8_t b[], uint8_t endSwap=0)
{
  const uint8_t i= ((b[1] & 0x40) > 0);
  uint8_t t[6];

  gEv= 0x0;
  if (gTWI.writeToSync(0x50,b,2)) // set page address
  {
    for (int8_t i=0; i<sizeof(t); i++) { t[i]= 0xA5; }
    if (gTWI.readFromSync(0x50,t,sizeof(t))) // retrieve leading Bytes
    {
      s.print("Hdr:"); dumpHexFmt(s, t, 6);
      s.println();
      gEv= '-';
      if ((b[endSwap^1] != t[1]) || (b[endSwap] != t[0]) || (gS[i][3] != t[5])) // Check for stored page content match
      {
        b[2]= b[endSwap]; b[3]= b[endSwap^1]; // page begins with its own address, possibly swapped endian order
        if (gS[i][3] != b[7]) { memcpy(b+4, gS[i], qS); } // Fill with text as necessary
        gTWI.writeToSync(0x50, b, 2+qP); // store 32bytes (prefixed with BE page address)
        gEv= 'W';
      }
    }
  }
  s.println(gEv);
  return(gEv);
} // fillTest

void loop (void)
{
  if (gT.update())
  {
    gFlags|= 0x5;
    //if ((gIter > 4) && (gIter & 0x1)) { gFlags|= 0x2; }
  }
  if ((gFlags & 0x2) && gTWI.sync())
  {
    fillTest(DEBUG,gTWTB,1);
    gFlags&= ~0x2;
  }
  if (gFlags & 0x1)
  {
    int r[2]= {0};
    if (0x0 != gEv) { gTWI.sync(-1); }
    if ('W' == gEv) { delay(10); }
    r[0]= gTWI.writeToRevSync(0x50, gTWTB+2, 2);  // select page
    if (r[0] > 0)
    {
      //memset(gTWTB+32, 0xA5, sizeof(gTWTB)-34);
      gTWTB[2]= gTWTB[31]= 0xA5;
      r[1]= gTWI.readFromSync(0x50, gTWTB+2, qP); // retrieve data
    }
    // else { memset(gTWTB+2, 0xA5, sizeof(gTWTB)-2); } // paranoid
    //dump<uint8_t>(DEBUG, ub, 2, "ub", "\n");
    //dump<int>(DEBUG, r, 2, "r", " nEV="); DEBUG.println(gTWI.iQ); //gNISR);
    //dump<uint32_t>(DEBUG, u, 5, "u", "\n");
    if ((r[0] > 0) && (r[1] > 0))
    { gTWI.sync();
      dumpHexTab(DEBUG, gTWTB, sizeof(gTWTB));
    }
    gFlags&= ~0x1;
  }
  if (gFlags & 0x4)
  {
    setAddr(gTWTB, (++gIter & 0x7F) << 5); // 4kB -> 128pages of 32Bytes
    DEBUG.print('I'); DEBUG.print(gIter); DEBUG.println(": ");
    gTWI.logEv(DEBUG); gTWI.clrEv();
    gFlags&= ~0x4;
  }
} // loop
