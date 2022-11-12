// Duino/Hack/hack.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2022

#include "Common/DN_Util.hpp"
#include "Common/M0_Util.hpp"
#include "Common/SX127x.hpp"


/***/

//define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG Serial

class HackSPI : public CCommonSPI
{
public:
   HackSPI (void) { ; }

   void init (void)
   {
      spiSet= SPISettings(1000000, MSBFIRST, SPI_MODE0);
      CCommonSPI::begin();
   } // init

   void release (void) { CCommonSPI::end(); }

}; // class HackSPI

// Triangular pulse with 50% dead zone
class TriPulse
{
private:
   uint16_t cycle;

public:
   TriPulse (void) : cycle{0} { ; }

   uint8_t nextV (void)
   {
      uint8_t v= ++cycle;
      if (0x200 & cycle) { return(0); } // dead zone else...
      if (0x100 & cycle) { v= 0xFF-v; }
      return(v);
   } // nextV

   uint16_t cycleM (uint16_t m=-1) { return(cycle & m); }

}; // class TriPulse


/***/

CSX127xDebug gRMD;
DNTimer gTick(1), gSlowTick(1000);
TriPulse gTriPulse;
uint32_t gSTC;

#include "hardware/rtc.h"
#include "pico/stdlib.h"
//include "pico/sleep.h"
#include "pico/util/datetime.h"

void bootMsg (Stream& s)
{
  //char sid[8];
  s.print("\n---\n");
  //if (getID(sid) > 0) { DEBUG.print(sid); }
  s.print(" Hack " __DATE__ " ");
  s.println(__TIME__);
} // bootMsg

void dump (Stream& s)
{
   s.println(gSTC);
   s.print("rtc:"); s.println(rtc_running());
} // dump

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  gRMD.init();
  pinMode(LED_BUILTIN, OUTPUT);
  delay(10); // POR 10ms, MRST 5ms
  if (gRMD.identify(DEBUG)) { gRMD.setup(); }
  dump(DEBUG);
} // setup


void loop (void)
{
   if (gTick.update())
   {
      analogWrite(LED_BUILTIN, gTriPulse.nextV()); // 8b only (clamped [0..255])
      uint16_t v= gTriPulse.cycleM(0xFFF);
      if (0 == (0xFF & v)) { DEBUG.print('.'); }
      if (0 == v)
      {
         gRMD.dump1(DEBUG);
         dump(DEBUG);
      }
   }
} // loop

#ifdef TARGET_RP2040
// 2nd core thread
void loop1 (void) { gSTC+= gSlowTick.update(); } // loop1 rtc_sleep(); ?
#endif // TARGET_RP2040
