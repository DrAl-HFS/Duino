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


LEDMatrixDriver led(LM_SEGMENTS, PIN_NCS); //, LEDMatrixDriver::INVERT_SEGMENT_X | LEDMatrixDriver::INVERT_Y);

#define SCROLL_TICK 40
// 40ms -> 25pix/s -> ~5 chars/sec
// 30ms -> 33.3pix/sec -> ~6chars/sec

// No need to alter this.
uint32_t marqueeDelayTimestamp = 0;

//char text[]= "Nemo me Impune Lacessit\n" //!#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
char text[]= "Aa Bb Cc Dd Ee Ff Gg Hh Ii Jj Kk Ll Mm Nn Oo Pp Qq Rr Ss Tt Uu Vv Ww Xx Yy Zz 0Oo 1Iil\n";

TextScroll gTS;

void marquee (void)
{
  uint32_t t= millis();

  if (t < 1) { marqueeDelayTimestamp= 0; }
  if (t < marqueeDelayTimestamp) return;

  marqueeDelayTimestamp= t + SCROLL_TICK;

  // Shift everything left by one led column.
  led.scroll(LEDMatrixDriver::scrollDirection::scrollLeft);

  // Write the next column of leds to the right.
  led.setColumn(LM_WIDTH - 1, gTS.nextCBM(text));
  
  // Write buffered changes.
  led.display();
} // marquee

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { DEBUG.print(sid); }
  DEBUG.print(" matrix " __DATE__ " ");
  DEBUG.println(__TIME__);
} // bootMsg

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  DUMP(DEBUG);
  //DEBUG.print("maxI"); DEBUG.println(sizeof(font5by7Index));
  //gTS.dump(DEBUG);

  gTS.nextGlyph(text[0]);
  
  led.setEnabled(true);

  // LED brightness (0 - 15).
  led.setIntensity(3);
}

void loop (void)
{
  marquee();
}
