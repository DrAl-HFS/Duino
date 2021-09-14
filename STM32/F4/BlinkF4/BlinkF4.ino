// Duino/STM32/F4/BlinkF4.ino - STM32F4* basic test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2020 - Sept 2021

#ifdef ARDUINO_ARCH_STM32F4
//define DEBUG Serial // = instance of USBSerial, same old CDC sync problems
#define DEBUG Serial1 // PA9/10 (F1xx compatible)

#ifdef VARIANT_blackpill_f401
#define PIN_LED     PC13  // = LED_BUILTIN
#define PIN_BTN     PA0   // ? BTN_BUILTIN ?
#else
#define PIN_LED     PE0
#define PIN_BTN     PD15
#endif // f401
#endif // STM32F4

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

void bootMsg (Stream& s)
{
    DEBUG.print("BlinkF4 " __DATE__ " ");
    DEBUG.println(__TIME__);
} // bootMsg

void setup (void)
{
  noInterrupts();

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);
  
  pinMode(PIN_BTN, INPUT);

  interrupts();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
} // setup

uint16_t v=0, i=0, d=100;

void loop ()
{
  digitalWrite(PIN_LED, ++i & 0x1);
  v= (v<<1) | digitalRead(PIN_BTN);
  if (0b10 == (v & 0b11)) // inverted input
  {
    d*= 2;
    if (d > 400) { d= 100; }
    DEBUG.println(d);
  } else DEBUG.print('.');
  delay(d);
}
