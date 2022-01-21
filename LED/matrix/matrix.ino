// Duino/LED/matrix/matrix.ino - Adapted from
//   "examples/MarqueeText" (author Marko Oette)
// distributed as part of the library package
//   "LEDMatrixDriver" (author Bartosz Bielawski).
// Reorganised font data, rewrote glyph scrolling
// as C++ classes. Factored out to multiple files
// for potential reuse.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2021 - Jan 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#include "Common/AVR/DA_Util.hpp"
#include "Common/AVR/DA_Therm.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Common/CDS18.hpp"
#include "CPrintLED.hpp"
#include "scroll.hpp"  // NB: order dependancy
//define DUMP(s) verifyFontIndex(s)

ThermNTC gTherm;

CDS18Debug gDS;

CPrintLED gLM;

TimedScroll gTS(50);

const char buildID[]= " LED/matrix " __DATE__ " " __TIME__ "\r";
const char asciiCharSet[]= " !#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~";
//char discrim= "0Oo l1Ii";
//char text[]= "!\"20~C\n";
//char motto[]= "Nemo me Impune Lacessit\n" 
//char alpha= "Aa Bb Cc Dd Ee Ff Gg Hh Ii Jj Kk Ll Mm Nn Oo Pp Qq Rr Ss Tt Uu Vv Ww Xx Yy Zz\n";

bool scrollMessage (const char msg[], TimedScroll& ts, CPrintLED& lm)
{
  bool b= ts.update();
  if (b)
  {
    // Shift everything left by one led column.
    lm.scroll(LEDMatrixDriver::scrollDirection::scrollLeft);

    // Write the next column of leds to the right.
    lm.setColumn(LM_WIDTH - 1, ts.nextCBM(msg));
    
    // Write buffered changes.
    lm.display();
  }
  return(b);
} // scrollMessage

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { s.print(sid); }
  s.println(buildID);
} // bootMsg

void setup (void)
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  DUMP(DEBUG);

  gDS.init();

  gLM.clear();
  gLM.printStr("LED/mtx",0);
  gLM.display();

  // LED brightness (0 - 15).
  gLM.setIntensity(3);
  gLM.setEnabled(true);

  gTS.addDelay(500);
} // setup

uint8_t t=0, u=0;

void loop (void)
{
  t+= scrollMessage(buildID, gTS, gLM);
  if (t >= 10)
  {
    t-= 10;
    gTherm.log(DEBUG);
    if (++u >= 10)
    {
      u-= 10;
      gDS.log(DEBUG); gDS.start(); // write(0x44);
    }
  }
} // loop
