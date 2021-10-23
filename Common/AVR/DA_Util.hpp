// Duino/Common/AVR/DA_Util.hpp - Arduino-AVR C++ utility code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#ifndef DA_UTIL_HPP
#define DA_UTIL_HPP

#include "DA_Util.h"

#ifndef DEBUG_BAUD
#define DEBUG_BAUD 115200
#endif

bool beginSync (HardwareSerial& s, const uint32_t bd=DEBUG_BAUD, const uint8_t cfg=SERIAL_8N1, int8_t n=20)
{
  if ((bd > 0) && (cfg > 0))
  {
    uint8_t d= n;
    do
    {
      if (s)
      {
        s.begin(bd,cfg);
        return(true);
      }
      else { delay(d); d= 1 + (d >> 1); }
    } while (--n > 0);
  }
  return(false);
} // beginSync

// Not AVR specific: displace to general Duino util
void dumplb (Stream& s, const char *h, const uint8_t b[], const int8_t n, const char *f=NULL)
{
  if (h) { s.print(h); }
  for (int8_t i=0; i<n; i++)
  { 
    s.print(b[i]);
    if (i < n) { s.print(','); }
  }
  if (f) { s.print(f); }
} // dumplb

#endif //  DA_UTIL_HPP

