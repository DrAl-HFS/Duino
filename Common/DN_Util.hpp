// Duino/Common/DN_Util.hpp - 'Duino library jiggery-pokery
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#ifndef DN_UTIL_HPP
#define DN_UTIL_HPP

// Factor to where ?
#define BIT_MASK(n) ((1<<(n))-1)

#if 0
#define SERIAL_TYPE HardwareSerial  // UART
#define SERIAL_TYPE USBSerial       // STM32
#define SERIAL_TYPE usb_serial_class  // Teensy
#endif

#ifndef DEBUG_BAUD
#define DEBUG_BAUD 115200
#endif

#ifndef SERIAL_DELAY_STEP
#define SERIAL_DELAY_STEP 30  // *20=600ms, long delay for Teensy USB-serial sync
#endif

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
      uint16_t d= sdp.getInitialDelay();
      uint8_t i= sdp.getInitialIter();
      do
      {
         if (s)
         {
            s.begin(bd);
            return(true);
         }
         else { delay(d); d= 1 + (d >> 1); }
      } while (--i > 0);
   }
   return(false);
} // beginSync

#endif // DN_UTIL_HPP
