// Duino/Common/STM32/ST_Util.hpp - Misc hackery
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2021

#ifndef ST_UTIL_HPP
#define ST_UTIL_HPP

#define BIT_MASK(n) ((1<<(n))-1)

/*
// DEPRECATE: replaced with DN_Util version...
// General fail-through start-sync routine for 'Duino hardware serial ports
bool beginSync (HardwareSerial& s, uint32_t bd=115200, int8_t n=20)//, uint8_t cfg=SERIAL_8N1
{
  uint8_t d= n;
  if (bd > 0)// && (cfg > 0))
  {
    do
    {
      if (s)
      {
        s.begin(bd); //,cfg);
        return(true);
      }
      else { delay(d); d= 1 + (d >> 1); }
    } while (--n > 0);
  }
  return(false);
} // beginSync


// USB version, pretty much useless due to host CDC problems over target reset
bool beginSync (USBSerial& s, int8_t n=20)
{
  uint8_t d= n;
  do
  {
    if (s)
    {
      s.begin();//bd,cfg);
      return(true);
    }
    else { delay(d); d= 1 + (d >> 1); }
  } while (--n > 0);
  return(false);
} // beginSync
*/

/* Hardware ID & calibration stuff */

typedef struct { uint32_t id[3]; } UID;

#ifdef ARDUINO_ARCH_STM32F1
#define FLAKB_BASE ((const uint16_t*)0x1FFFF7E0)
#define UID_BASE   ((const UID*)0x1FFFF7E8)
#endif
#ifdef ARDUINO_ARCH_STM32F4
#define UID_BASE   ((const UID*)0x1FFF7A10)
#define FLAKB_BASE ((const uint16_t*)0x1FFF7A22)
#endif

//typedef struct { const UID *pUID; const uint16_t *pFlashKB; } HWID;
bool dumpID (Stream& s)
{
   //s.print("STM32F1");
   s.print("STM32F4");
   s.print(" UID:");
   s.print(UID_BASE->id[0],HEX);
   for (int i=1; i<3; i++) { s.print(':'); s.print(UID_BASE->id[i],HEX);  }
   s.print("\nFlash:"); s.print(*FLAKB_BASE); s.println("KB");
   return(true);
} // ident

/* Voltage reference calibration values
uint8_t getRef (uint16_t vr[])
{
   vr[0]= VREFINT;
   return(1);
} // getRef
*/

#endif // ST_UTIL_HPP
