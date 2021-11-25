// Duino/Common/DN_Util.hpp - 'Duino library jiggery-pokery
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#ifndef DN_UTIL_HPP
#define DN_UTIL_HPP

// Factor to where ?
#define BIT_MASK(n) ((1<<(n))-1)

#ifndef SERIAL_TYPE
#define SERIAL_TYPE HardwareSerial  // UART
#endif

#if 0
#define SERIAL_TYPE HardwareSerial  // typically UART, but USB on PICO
#define SERIAL_TYPE USBSerial       // STM32
#define SERIAL_TYPE usb_serial_class  // Teensy
#endif

#ifndef DEBUG_BAUD
#define DEBUG_BAUD 115200
#endif

#ifndef SERIAL_DELAY_STEP
#define SERIAL_DELAY_STEP 30  // *20=600ms, long delay for USB-serial sync
#endif

char hexCh (uint8_t x, const char a='a')
{
   x= '0' + (x & 0xF);
   if (x > '9') { x-= '9' + a; }
   return(x);
} // hexCh

int hex2ChNU8 (char ch[], const int maxCh, const uint8_t u[], const int n)
{
   int i= 0, j= 0;
   while ((i < maxCh) && (j < n))
   {
      const uint8_t x= u[j];
      ch[i++]= hexCh(x >> 4);
      if (i < maxCh) { ch[i++]= hexCh(x); }
   }
   return(i);
} // hex2ChNU8

class SerialDelayParam
{
protected:
   uint8_t m, n;
   
public:
   SerialDelayParam (uint8_t dsms=SERIAL_DELAY_STEP, uint8_t niter=20) : m{dsms}, n{niter} { ; }
   
   uint16_t getInitialDelay (void) const { return((uint16_t)m * n); }
   uint8_t getInitialIter (void) const { return(n); }
}; // SerialDelayParam

// General fail-through start-sync routine for 'Duino serial interfaces
bool beginSync (SERIAL_TYPE& s, const uint32_t bd=DEBUG_BAUD, const SerialDelayParam& sdp=SerialDelayParam())
{
   if (bd >= 0)
   {
      uint16_t t= 0, d= sdp.getInitialDelay();
      uint8_t n= sdp.getInitialIter();
      do
      {
         if (s)
         {
            s.begin(bd);
            if (t < 100) { delay(100 - t); }
            return(true);
         }
         else { delay(d); t+= d; d= 1 + (d >> 1); }
      } while (--n > 0);
   }
   return(false);
} // beginSync

#endif // DN_UTIL_HPP
