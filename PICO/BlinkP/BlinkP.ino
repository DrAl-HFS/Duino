// Duino/PICO/BlinkP/BlinkP.ino - RP2040 hacking
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors May - Nov 2021

#include "Common/DN_Util.hpp"
//include "Common/PICO/RP2040SDKWrap.hpp"
#include "Common/CAPA102.hpp"

#define LED_PIN LED_BUILTIN //=25

#define DEBUG Serial


CAPA102Pattern gAPA102;

CAPA102State gLED[16];

uint16_t gCyc= 0;


void bootMsg (Stream& s)
{ 
  s.println("\n---");
  s.println("BlinkP " __DATE__ " " __TIME__ );
#if 1
  s.print("LED="); s.println(LED_BUILTIN);
  s.print("MOSI="); s.println(MOSI);
  s.print("SCK="); s.println(SCK);
  s.print("NCS="); s.println(PIN_NCS);
#endif
} // bootMsg

void setup ()
{
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }

  pinMode(LED_PIN, OUTPUT);
  gAPA102.init();
  //pwmSetup();
  for (int i=0; i<16; i++) { gLED[i].setL(i); }
} // setup

void loop ()
{
#if 0
  digitalWrite(LED_PIN, gCyc & 0x1);
  //digitalRead(LED_PIN) ^ 0x1 );
  delay(100);
#else
  uint8_t c=0, v= gCyc;
  
  if (gCyc & 0x100) { v= 255-v; }
  if (0 == (gCyc & 0x200)) { c= v; } // dead zone
  
  analogWrite(LED_PIN, c); // 8b only (clamped [0..255])
#if 0
  gLED[15].setRGB(v,0,0);
  gAPA102.writeRep(gLED,16);
#else  
  uint8_t iC= (gCyc >> 7) & 0xF;
  uint8_t iLc[]={iC, 0x11, (uint8_t)(0xF-iC)};
  gLED[1].setL(v);
  gAPA102.writeInd(gLED,iLc,sizeof(iLc));
#endif
  delay(1); 
#endif
  //pwm_set_gpio_level(PWM_PIN,gLvl);
  //s+= 0x100;
  //delay(1);
  if (0 == (++gCyc & 0xFFF)) { gLED[0].dumpRGBL(DEBUG); DEBUG.println('.'); }
  else if (0 == (gCyc & 0xFF)) { DEBUG.print('.'); }
} // loop
