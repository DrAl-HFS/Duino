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

class CHackSPI : public CCommonSPI
{
public:
   CHackSPI (void) { ; }

   void init (void)
   {
      spiSet= SPISettings(1000000, MSBFIRST, SPI_MODE0);
      CCommonSPI::begin();
   } // init

   void release (void) { CCommonSPI::end(); }

}; // class CHackSPI

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

#include "hardware/rtc.h" // explicit initialisation needed ?
#include "pico/stdlib.h"
//include "pico/sleep.h"
#include "pico/util/datetime.h"
#include "Common/dateTimeUtil.hpp"

// should be defined elsewhere...
int cmpI (const int a, const int b) { return((a > b) - (a < b)); }
int cmpDT (const datetime_t *pDTA, const datetime_t *pDTB)
{
   int r= cmpI(pDTA->year, pDTB->year);
   if (0 != r) { return(r); }
   r= cmpI(pDTA->month, pDTB->month);
   if (0 != r) { return(r); }
   r= cmpI(pDTA->day, pDTB->day);
   if (0 != r) { return(r); }
   r= cmpI(pDTA->hour, pDTB->hour);
   if (0 != r) { return(r); }
   r= cmpI(pDTA->min, pDTB->min);
   if (0 != r) { return(r); }
   return cmpI(pDTA->sec, pDTB->sec);
} // cmpDT

void datetimeFromA (datetime_t *pDT, const char *date, const char *time)
{  // i16: yyyy, i8: mm,dd,dow,hh,mm,ss
   pDT->year= 0;
   u8FromDateA(((uint8_t*)&(pDT->month))-1, date); // hacky set year (msb in little endian)
   pDT->year= (pDT->year >> 8) + 2000;
   pDT->dotw= 0;
   u8FromTimeA((uint8_t*)&(pDT->hour), time);
} // datetimeFromA

void printDT (Stream& s, const datetime_t *pDT, uint8_t fmtFlags=0)
{
   s.print(pDT->year); s.print('/');
   s.print(pDT->month); s.print('/');
   s.print(pDT->day); s.print(' ');
   s.print(pDT->hour); s.print(':');
   s.print(pDT->min); s.print(':');
   s.print(pDT->sec);
} // printDT

void check (Stream& s, uint32_t *p= (uint32_t*)RTC_BASE, int8_t n=2)
{
   for (int8_t i=0; i<n; i++) { s.print(p[i], HEX); s.print(','); }
} // check

class RTC
{
public:
   RTC (void) { datetime_t dt={2022,11,13,0,21,40,0}; rtc_init(); rtc_set_datetime(&dt); }

   void set (const char *date, const char *time)
   {
      datetime_t dt;
      datetimeFromA(&dt, date, time);
      rtc_set_datetime(&dt);
   } // set

   bool autoSet (const char *date, const char *time)
   {
      int r= 1;
      datetime_t newDT;

      datetimeFromA(&newDT, date, time);
      if (rtc_running())
      {
         datetime_t currentDT;
         rtc_get_datetime(&currentDT);
         r= cmpDT(&newDT, &currentDT);
      }
      else { rtc_init(); r= 1; }
      if (r > 0)
      {
         rtc_set_datetime(&newDT);
         return(true);
      }
      return(false);
   } // autoSet

   void print (Stream& s)
   {
      datetime_t dt={-1,-1,-1,-1,-1,-1,-1};
      rtc_get_datetime(&dt);
      printDT(s,&dt);
   }
}; // RTC

/***/

RTC gRTC;
CSX127xDebug gRMD;
DNTimer gTick(1), gSlowTick(1000);
TriPulse gTriPulse;
uint32_t gSTC;

void bootMsg (Stream& s)
{
   //char sid[8];
   s.print("\n---\n");
   //if (getID(sid) > 0) { DEBUG.print(sid); }
   s.print(" Hack " __DATE__ " ");
   s.println(__TIME__);
} // bootMsg

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  gRMD.init();
  pinMode(LED_BUILTIN, OUTPUT);
  delay(10); // POR 10ms, MRST 5ms
  if (gRMD.identify(DEBUG)) { gRMD.setup(); }
  //gRTC.autoSet(__DATE__,__TIME__);
  //gRTC.print(DEBUG);
} // setup

void dump (Stream& s, uint16_t c)
{
   if (0 == (0xFF & c)) { s.print('.'); }
   if (0 == c)
   {
      s.println();
      gRMD.dump1(s);
#if 0
      datetime_t dt;
      datetimeFromA(&dt,__DATE__,__TIME__);
      printDT(DEBUG, &dt);
//#else
#endif
      //gRTC.print(s);
      check(s);
      s.print("tick="); s.println(gSTC);
      gRTC.print(s);
   }
} // dump

void loop (void)
{
   if (gTick.update())
   {
      analogWrite(LED_BUILTIN, gTriPulse.nextV()); // 8b only (clamped [0..255])
      gRMD.logRSSI();
      dump(DEBUG, gTriPulse.cycleM(0xFFF));
   }
   sleep_us(100);
} // loop

#ifdef TARGET_RP2040
// 2nd core thread
void loop1 (void) { gSTC+= gSlowTick.update(); sleep_ms(100); } // loop1 rtc_sleep(); ?
#endif // TARGET_RP2040
