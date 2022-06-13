// Duino/STM32/TestST/TestST.ino - STM32 test code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb-Mar 2021

#define DEBUG Serial1
//#define TEST_CRC

#if 0
//#include "Common/MBD/mbdDef.h" // UU16/32
#else
typedef union { uint32_t u32; uint16_t u16[2]; uint8_t u8[4]; } UU32;
#endif

#include "Common/DN_Util.hpp"
#include "Common/STM32/ST_HWH.hpp"
#include "Common/STM32/ST_Util.hpp"
#include "Common/STM32/ST_Timing.hpp"
#include <SPI.h>
#include "Common/CW25Q.hpp"
#include "Common/MFDHacks.hpp"



#include "Common/CMAX10302.hpp"


/***/

#define PIN_LED    PC13
#define PIN_PULSE  PB12
#define DEBUG_BAUD 115200


/***/


CMAX10302 gM2;

uint8_t gRawBuff[10*3];
uint32_t gSamples[16];

CClock gClock;
CTimer gT;
void tickFunc (void) { gT.nextIvl(); }

CW25QDbg gW25QDbg(36);
MFDHack mfd;
#ifdef TEST_CRC
HWCRC gCRC;
#endif


void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("TestST " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

void dump (Stream& s)
{ // check packing
  s.print("CFC header size:");
  s.print(sizeof(CFCHdrJ0)); s.print(", ");
  s.print(sizeof(CFCHdrD0)); s.print(", ");
  s.print(sizeof(CFCHdrR0)); s.println();
}

void setup (void)
{
  noInterrupts();

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);

  pinMode(PIN_PULSE, OUTPUT);
  digitalWrite(PIN_PULSE, 0);

  gClock.setA(__DATE__,__TIME__);
  gT.start(tickFunc);

  interrupts();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }

  //gRC522.init();
  gT.dbgPrint(DEBUG);

  gW25QDbg.init(DEBUG);
  gW25QDbg.dataEraseDirty(0x2900,0x1000); // 1MB

  mfd.test(DEBUG,gW25QDbg,gClock);

  Wire.begin(I2C_FAST_MODE);

  if (gM2.identify(DEBUG))
  {
    //DEBUG.print("M2Rst="); DEBUG.println(
    gM2.reset();
    gM2.dumpReg(DEBUG);
    gM2.start();
  }
} // setup

W25Q::PageScan scan(0x0000,1<<12);

volatile uint32_t r=0, v=0xFF0F01;
uint16_t last=0, uic=0, tS=0, lS=0;
uint8_t ivl=50;

uint32_t rbe18 (const uint8_t b[])
{
  uint32_t r= (b[0] & 0x3) << 16;
  r|= b[1] << 8;
  r|= b[2];
  return(r);
} // rbe18

void loop (void)
{
  const uint16_t d= gT.diff();
  if (d >= ivl)
  {
    gT.retire(ivl);
    uic+= ivl;
    if (uic >= 500-ivl) { gM2.startTemp(); }

    int rB= gM2.readData(gRawBuff,30);
    if (rB > 0)
    {
      uint8_t rS= rB / 3;
      DEBUG.print(rS); DEBUG.print(' ');
      for (uint8_t i=0; i<rS; i++)
      {
        gSamples[(tS+i)&0xF]= rbe18(gRawBuff+i*3);
      }
      tS+= rS;
    }
    /*
    gM2.logData(DEBUG);
    if (r > 0)
    {
      DEBUG.print("data["); DEBUG.print(r); DEBUG.print("]=");
      dumpHexTab(DEBUG, gSampleBuff, r,"\n", ' ', 6);
    }
    */
  }
  if (uic > 500)
  {
    digitalWrite(PIN_LED, 1);
    DEBUG.println();
    gClock.print(DEBUG, 0x01);
    if (gM2.temp(true)) { gM2.printT(DEBUG," "); }
    DEBUG.print("tS,dS="); DEBUG.print(tS); DEBUG.print(','); DEBUG.println(tS-lS);
    DEBUG.print(gSamples[0]); DEBUG.print(' '); DEBUG.println(gSamples[1]);
    lS= tS;
    uic-= 500;
    //scan.next(DEBUG,gW25QDbg);
    digitalWrite(PIN_LED, 0);
  }

} // loop

/*
#ifdef TEST_CRC
    testCRC(DEBUG,gCRC);
#else
    CMX::SysTick sysTick;
    r= bitRev32(v);
    r= bitRev32(v);
    r= bitRev32(v);
    r= bitRev32(v);
    uint32_t dt= sysTick.delta();
    DEBUG.print("dt="); DEBUG.println(dt);
    //DEBUG.println(sysTick.rvr());
#endif

  if (d != last)
  {
    c-= (c>0);
    digitalWrite(PIN_LED, c > 0);
    last= d;
#if 0
    uint16_t t[2], j;
    for (int i=0; i<20; i++)
    {
      j= i & 0x1;
      digitalWrite(PIN_PULSE, j);
      //t[j]= hwTickVal();
      //delayMicroseconds(1);
    }
    digitalWrite(PIN_PULSE, 0);
#else
    digitalWrite(PIN_PULSE, gT.swTickVal() & 0x1 );
#endif
  }
*/
