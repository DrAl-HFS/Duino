// Duino/LED/matrix/scroll.hpp - Scrolling text glyph rendering utils.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#include <LEDMatrixDriver.hpp>
#include "Font/font5x7r.hpp"

#ifndef PIN_NCS
#define PIN_NCS SS
#endif

#ifndef LM_SEGMENTS
#define LM_SEGMENTS 4
#endif

#define LM_WIDTH  (8*LM_SEGMENTS)
// LEDMatrixDriver::INVERT_SEGMENT_X | LEDMatrixDriver::INVERT_Y);


class CPrintLED : public LEDMatrixDriver
{
public:
   CPrintLED (uint8_t nSeg=LM_SEGMENTS, uint8_t ncs=PIN_NCS) : LEDMatrixDriver(nSeg, ncs) { ; }

   int8_t printCols (const uint8_t c[], const int8_t n, const uint8_t x)
   {
      for (int8_t i=0; i<n; i++) { setColumn(x+i, c[i]); }
      return(n);
   } // printCols

   int8_t printCh (const char ch, const uint8_t x)
   {
     int16_t iG= glyphIndexASCII(ch);
     if (iG >= 0)
     {
       uint8_t col[6];
       int8_t wG= glyphCopy(col, iG);
       col[wG]= 0x00;
       return printCols(col, wG+1, x);
     }
     return(0);
   } // printCh

   uint8_t printStr (const char s[], uint8_t x=0, const uint8_t w=32)
   {
     uint8_t i=0;
     while (s[i] && (x < w)) { x+= printCh(s[i++],x); }
     return(i);
   } // printStr

}; // CPrintLED
