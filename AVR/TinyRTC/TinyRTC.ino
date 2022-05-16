// Duino/AVR/TinyRTC/TinyRTC.ino
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar-May 2022

#define SERIAL_TYPE HardwareSerial  // UART
#define DEBUG      Serial

// NB: LED_BUILTIN=13
#define PIN_PWR 12
#define PIN_VB  A7
#define PIN_TCK 2

//#include <avr/pci.h>
#include "Common/DN_Util.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Common/CAT24C.hpp"
#include "Common/CDS1307.hpp"

DNTimer gT(1000);
CDS1307A gRTC;
CAT24C gERM;
volatile uint8_t gFlags=0x00; // 0x80 -> power cycle (battery test)
volatile uint16_t gV[3];

//ISR (PCINT2_vect)
//ISR (INT4_vect)
void tick (void) { gV[2]++; gFlags|= 0x02; }

void setupPCI (void)
{
  memset(gV,0,sizeof(gV));
  pinMode(PIN_TCK,INPUT_PULLUP);
#if 1
  attachInterrupt(digitalPinToInterrupt(PIN_TCK), tick, CHANGE); // D2->I0 (virtual for Uno compatibility)
#else // MEGA2560
  EICRB= 0x1; // Any edge on Pin2 -> INT4
  EIMSK|= 0x1<<4; // enable INT4
#endif
} // setupPCI

void power (bool reset=false)
{
  pinMode(PIN_PWR, OUTPUT); // force power transition to reset, prevent sync fail
  if (reset)
  {
    digitalWrite(PIN_PWR, LOW); // power off
    delay(50);
  }
  digitalWrite(PIN_PWR, HIGH); // power on
  //delay(50);
} // power

void i2cPower (bool on)
{ // 0 == INPUT -> high impedance ie. low leakage current (pull up/down ?)
  pinMode(SDA, on);
  pinMode(SCL, on);
} // i2cPower

void bootMsg (Stream& s)
{
  char sid[8];
  if (getID(sid) > 0) { s.print(sid); }
  s.print(" TinyRTC " __DATE__ " ");
  s.println(__TIME__);
} // bootMsg

void setRTC (Stream& s)
{
  int8_t r;
  s.println("setRTC() -");
  r= gRTC.setSqCtrl(DS1307HW::SQ_OUT|DS1307HW::SQWE);
  s.print(" setSqCtrl()->"); s.println(r);
  r= gRTC.clearV(DS1307HW::User::FIRST,0xFF,DS1307HW::User::COUNT);
  s.print(" clearV()->"); s.println(r);
  r= gRTC.setA(__TIME__,NULL,__DATE__,0);
  s.print(" setA() -> "); s.println(r);
} // setRTC

void setup (void)
{
  int r;

  power();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  I2C.begin(); // join i2c bus (address optional for master)
  //gRTC.testDays(DEBUG,__DATE__);
  r= gRTC.printTime(DEBUG,' ');
  if (r > 0) { gRTC.printDate(DEBUG); }
  else { setRTC(DEBUG); }

  //uint8_t b[8]; memset(b, 0xFF, sizeof(b));
  //r= gRTC.writeV(0x8, b, sizeof(b));
  //r= gRTC.readV(0x7, b, sizeof(b));
  //dumpHexTab(DEBUG, b, sizeof(b));
  //dump(DEBUG, b, sizeof(b), " b", "\n");
  // gRTC.clearV(DS1307HW::User::FIRST,0xFF,DS1307HW::User::COUNT); *ERROR* missing last byte..?
  setupPCI();
  //DEBUG.print("LED_BUILTIN="); DEBUG.println(LED_BUILTIN);
  //analogReadResolution(8);  A7 // Nano D13
  pinMode(PIN_VB,INPUT); // ANALOG);
  //DEBUG.print("getSet()->"); DEBUG.println(gRTC.getSet());
  //pinMode(LED_BUILTIN,INPUT); ???
  gRTC.dump(DEBUG);
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
int8_t err=3;

void loop (void)
{
  const uint8_t t= digitalRead(PIN_TCK);
  if (gFlags & 0x02)
  {
    digitalWrite(LED_BUILTIN, t);
    gFlags&= ~0x02;
  }
  gV[t]++;
  if (gT.update())
  {
    i2cPower(true);
    if (digitalRead(PIN_PWR) < HIGH) { digitalWrite(PIN_PWR, HIGH); }
    int vB= analogRead(PIN_VB);
    I2C.setClkT(TWM::CLK_100);
    int r= gRTC.printTime(DEBUG);
    //I2C.logEv(DEBUG);
    DEBUG.print("vB="); DEBUG.print(vB);
    dump(DEBUG, gV, 3, " gV", "\n"); gV[0]= gV[1]= gV[2]= 0;
    if (r < 0)
    {
      gRTC.dump(DEBUG);
      if (err-- <= 0)
      {
        setRTC(DEBUG);
        err= 3;
      }
    }
    //I2C.setClkT(TWM::CLK_400);
    gERM.dump(DEBUG, gIter & 0x7); // 0x7F 128*32= 4k
    if (gFlags & 0x80) { digitalWrite(PIN_PWR, LOW); }
    I2C.clrEv();
    I2C.end();
    i2cPower(false); // ready for power-off
    gIter++;
  }
  sleep_cpu(); // NB: sleep setup inside CSync
} // loop
