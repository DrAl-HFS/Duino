// Duino/AVR/TinyRTC/TinyRTC.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar-May 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

#define PIN_PWR 13 // Nano D13

#include "Common/DN_Util.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Common/CAT24C.hpp"
#include "Common/CDS1307.hpp"

DNTimer gT(1000);
CDS1307A gRTC;
CAT24C gERM;

void pwrRst (void)
{
  pinMode(PIN_PWR, OUTPUT); // force power transition to reset, prevent sync fail
  digitalWrite(PIN_PWR, LOW); // power off
  delay(50);
  digitalWrite(PIN_PWR, HIGH); // power on
  delay(50);
} // pwrRst

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { s.print(sid); }
  s.print(" TinyRTC " __DATE__ " ");
  s.println(__TIME__);
} // bootMsg

void setRTC (Stream& s)
{
  int8_t r= gRTC.setA(__TIME__,NULL,__DATE__,0);
  DEBUG.print("gRTC.setA() -> "); DEBUG.println(r);
} // setRTC

void setup (void)
{
  int8_t r;

  pwrRst();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  I2C.begin(); // join i2c bus (address optional for master)
  //gRTC.testDays(DEBUG,__DATE__);
  r= gRTC.printTime(DEBUG,' ');
  if (r > 0) { gRTC.printDate(DEBUG); }
  else { setRTC(DEBUG); }
  //analogReadResolution(8);
  pinMode(A7,INPUT);//ANALOGUE);

#ifdef TWI_BUFFER_LENGTH
  DEBUG.print("I2C buffers:"); DEBUG.print(TWI_BUFFER_LENGTH); DEBUG.print(','); DEBUG.println(BUFFER_LENGTH);
#endif
} // setup

void buffTest (void)
{
  //const char s1[]="Lorem ipsum dolor sit amet";
  //const char s2[]="consectetur adipiscing elit, sed do eiusmod"; // truncated by Wire buffer
  uint8_t b[32];
  int r=0, q= sizeof(b);
  //for (int i=0; i<q; i++) { b[i]= 1+i; }
  //r= gERM.writePage(5,b,q);
  DEBUG.print("gERM.writePage() - q,r= ");
  DEBUG.print(q); DEBUG.print(','); DEBUG.println(r);
  if (r > 0) { gERM.sync(); }
} // buffTest

uint16_t gIter=0;
uint8_t mode=0x1; // power cycle
int8_t err=3;

void loop (void)
{
  if (gT.update())
  {
    if (digitalRead(PIN_PWR) < HIGH) { digitalWrite(PIN_PWR, HIGH); }
    int vB= analogRead(A7);
    int r= gRTC.printTime(DEBUG,' ');
    DEBUG.print(": vB="); DEBUG.println(vB);
    if (r < 0)
    {
      gRTC.dump(DEBUG);
      if (err-- <= 0)
      {
        setRTC(DEBUG);
        err= 3;
      }
    }
    gIter++;
    gERM.dump(DEBUG, gIter & 0x7); // 0x7F 128*32= 4k
    if (mode & 0x1) { digitalWrite(PIN_PWR, LOW); }
  }
} // loop
