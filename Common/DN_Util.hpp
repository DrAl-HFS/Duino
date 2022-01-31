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

// Low 4 bits (nybble) hex char
char hexChFromL4 (uint8_t x, const char a='a')
{
   x &= 0xF;
   if (x > 9) return(a + x - 0xA);
   //else
   return(x + '0');
} // hexChFromL4

// Full byte hex chars (leading zero)
void hexChFromU8 (char ch[2], uint8_t x, const char a='a')
{
   ch[0]= hexChFromL4(x>>4,a);
   ch[1]= hexChFromL4(x,a);
} // hexChFromU8

uint8_t bcd4Add (const uint8_t a, const uint8_t b, const uint8_t c=0)
{
   uint8_t r= a + b + c;
   if (r >= 0xA) { r= 0x10 + (r-0xA); }
   return(r);
} // bcd4Add

uint8_t bcd8Add (uint8_t r[1], const uint8_t a, const uint8_t b)
{
   uint8_t t[2];
   t[0]= bcd4Add(a&0xF, b&0xF); // lo digits -> sum + carry
   t[1]= bcd4Add(a>>4, b>>4, t[0]>>4); // hi digits -> sum + carry
   r[0]= ((t[1] & 0xF) << 4) | (t[0] & 0xF); // mergi hi lo digits
   return(t[1]>>4); // return hi carry
} // bcd8Add

/*
 DEPRECATE (?)
int hex2ChNU8 (char ch[], const int maxCh, const uint8_t b[], const int n)
{
   int i= 0, j= 0;
   while ((i < maxCh) && (j < n))
   {
      //hexChFromU8(ch+i, u[j++]); i+= 2;
      const uint8_t x= b[j++];
      ch[i++]= hexCh(x >> 4);
      if (i < maxCh) { ch[i++]= hexCh(x); }
   }
   return(i);
} // hex2ChNU8
*/

void dumpHexFmt (Stream& s, const uint8_t b[], const int16_t n, char fs[], const uint8_t ofs=0, const char a='a')
{
   for (int16_t i=0; i<n; i++)
   {
      hexChFromU8(fs+ofs, b[i], a);
      s.print(fs);
   }
} // dumpHexFmt

void dumpHexFmt (Stream& s, const uint8_t b[], const int16_t n)
{
   char fs[]="  "; // safe default
   dumpHexFmt(s,b,n,fs);
} // dumpHexFmt

void dumpCharFmt (Stream& s, const uint8_t b[], const int16_t n, char fs[], const uint8_t ofs=0, const char ng='.')
{
   for (int16_t i=0; i<n; i++)
   {
      fs[ofs]= b[i];
      if (((signed char)b[i] < ' ') || (b[i] >= 0x7F)) { fs[ofs]= ng; } // replace non-glyph characters (except space)
      s.print(fs);
   }
} // dumpCharFmt

void dumpCharFmt (Stream& s, const uint8_t b[], const int16_t n)
{
   char fs[]="."; // safe default
   dumpCharFmt(s,b,n,fs);
} // dumpCharFmt

// Consider: struct { } TabFmt; Move to class ?
void dumpHexTab (Stream& s, const uint8_t b[], const int16_t n, const char *end="\n", const char sep=' ', const int16_t w=16)
{
   if (n > 0)
   {
      int16_t m=n, i=0;
      char sx[4];
      sx[2]= sep;
      sx[3]= 0x00;
      if ((w > 0) && (w < m)) { m= w; }
      do
      {
         dumpHexFmt(s, b+i, m-i, sx);
         s.print('\t');
         dumpCharFmt(s, b+i, m-i);
         i= m;
         if (i < n)
         {
            if (w > 0) { m+= w; }
            if (m > n) { m= n; }
            s.println();
         }
      } while (i < n);
   }
   if (end) { s.print(end); }
} // dumpHexTab

// temporary conflict resolution
#ifndef DA_UTIL_HPP

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

#endif // DA_UTIL
// namespace ???

typedef uint32_t TickCount;

class DNTimer
{
   TickCount nextT, interval;

protected:
   void set (TickCount t) { nextT= t; }
   void next (void) { nextT+= interval; }

public:
   DNTimer (TickCount ivl) : interval{ivl} { nextT= millis(); }

   bool reached (TickCount t) const { return(t >= nextT); }

   void add (TickCount delay) { set(nextT + delay); }

   bool update (void)
   {
      TickCount t= millis();
      bool r= reached(t);
      if (r)
      {
         next();
         if (reached(t)) { set(t); }
      }
      return(r);
   }
}; // DNTimer

#endif // DN_UTIL_HPP
