// Duino/LED/matrix/matrix.ino - Adapted from
//   "examples/MarqueeText" (author Marko Oette)
// distributed as part of the library package
//   "LEDMatrixDriver" (author Bartosz Bielawski).
// Reorganised font data, rewrote glyph scrolling
// as C++ classes. Factored out to multiple files
// for potential reuse.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2021


#include <LEDMatrixDriver.hpp>

#include "Common/AVR/DA_Config.hpp"

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial
#include "Common/AVR/DA_Util.hpp"

#include "Font/font5x7r.hpp" // NB: order dependancy
#include "scroll.hpp"


#define PIN_NCS SS
#define LM_SEGMENTS 4
#define LM_WIDTH  (8*LM_SEGMENTS)


LEDMatrixDriver lmd(LM_SEGMENTS, PIN_NCS); //, LEDMatrixDriver::INVERT_SEGMENT_X | LEDMatrixDriver::INVERT_Y);

//char motto[]= "Nemo me Impune Lacessit\n" 
//char discrim= "0Oo 1Iil"
//char alpha= "Aa Bb Cc Dd Ee Ff Gg Hh Ii Jj Kk Ll Mm Nn Oo Pp Qq Rr Ss Tt Uu Vv Ww Xx Yy Zz\n";
char asciiCharSet[]= "!#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
char text[]= "!\"20~C\n";

TimedScroll gTS(50);

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { DEBUG.print(sid); }
  DEBUG.print(" LED/matrix " __DATE__ " ");
  DEBUG.println(__TIME__);
} // bootMsg

uint8_t printCh (const char ch, const uint8_t x)
{
  int16_t iG= glyphIndexASCII(ch);
  uint8_t iC= 0;
  if (iG >= 0)
  {
    uint8_t wG= glyphWidth(iG);
    
    while (iC < wG) { lmd.setColumn(x+iC, glyphCol(iG, iC)); iC++; }
    lmd.setColumn(x+iC++, 0x00);
  }
  return(iC);
} // printCh

uint8_t printStr (const char s[], uint8_t x=0, const uint8_t w=32)
{
  uint8_t i=0;
  while (s[i] && (x < w))
  {
    x+= printCh(s[i++],x);
  }
  return(i);
} // printStr

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  DUMP(DEBUG);
  //DEBUG.print("maxI"); DEBUG.println(sizeof(font5by7Index));
  //gTS.dump(DEBUG);

  gTS.nextGlyph(text[0]);
  gTS.delay(5000);
  
  lmd.clear();
  // LED brightness (0 - 15).
  lmd.setIntensity(3);
  lmd.setEnabled(true);
  
  //printCh('X',8);
  printStr("matrix",0);
  lmd.display();
} // setup

void loop (void)
{
  if (gTS.update())
  {
    // Shift everything left by one led column.
    lmd.scroll(LEDMatrixDriver::scrollDirection::scrollLeft);

    // Write the next column of leds to the right.
    lmd.setColumn(LM_WIDTH - 1, gTS.nextCBM(asciiCharSet));
    
    // Write buffered changes.
    lmd.display();
  }
} // loop
