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
   x &= 0xF;
   if (x > 9) return(a + x - 0xa);
   //else
   return(x + '0');
} // hexCh

void hexByte (char ch[2], uint8_t x, const char a='a')
{
   ch[0]= hexCh(x>>4,a);
   ch[1]= hexCh(x,a);
} // hexByte

/*
 DEPRECATE (?)
int hex2ChNU8 (char ch[], const int maxCh, const uint8_t b[], const int n)
{
   int i= 0, j= 0;
   while ((i < maxCh) && (j < n))
   {
      //hexByte(ch+i, u[j++]); i+= 2;
      const uint8_t x= b[j++];
      ch[i++]= hexCh(x >> 4);
      if (i < maxCh) { ch[i++]= hexCh(x); }
   }
   return(i);
} // hex2ChNU8
*/

void dumpHexFmt (Stream& s, const uint8_t b[], const int16_t n, char fs[]="00", const uint8_t ofs=0, const char a='a')
{
   for (int16_t i=0; i<n; i++)
   {
      hexByte(fs+ofs, b[i], a); 
      s.print(fs);
   }
} // dumpHexFmt

void dumpCharFmt (Stream& s, const signed char c[], const int16_t n, char fs[]=" ", const uint8_t ofs=0, const char g='.')
{
   for (int16_t i=0; i<n; i++)
   {
      fs[ofs]= c[i];
      if ((c[i] < ' ') || (c[i] >= 0x7F)) { fs[ofs]= g; } // replace non-glyph characters
      s.print(fs);
   }
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
         /*int16_t j= i;
         for (; j<m; j++)
         {
            hexByte(sx, b[j]); 
            s.print(sx);
         }
         */
         dumpHexFmt(s, b+i, m, sx);
         s.print('\t');
         dumpCharFmt(s, b+i, m);
         i= m;
         /*
         for (; i<m; i++)
         {
            signed char ch= b[i];
            if (ch < ' ') { ch= '.'; }
            s.print((char)ch);
         }
         */
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
