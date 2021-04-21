// Duino/ ??? /SerMux.hpp - Serial (software) channel multiplex
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Apr 2021

#ifndef SERMUX_HPP
#define SERMUX_HPP

// ASCII cstring length-until
int8_t lentil (const char s[], const char end=0x00) { int8_t i=0; while (end != s[i]) { ++i; } return(i); }
//int8_t readN (uint8_t b[], const int8_t n, Stream& s) { for (int8_t i=0; i<n; i++) { b[i]= s.read(); } return(n); }

class CSerMux // Presently stateless... so not really a class...
{
protected:
   
public:
   CSerMux (void) { ; }
  
   int8_t send (Stream& s, const uint8_t epid, const uint8_t b[], const int8_t n)
   {
      if (n <= 0) { return(0); }
      uint8_t hdr= 0xC0 | (0x30 & epid) | (0xF & (n-1));
      s.write(hdr);
      s.write(b,n);
      //for (int8_t i=0; i<n; i++) { s.write(b[i]); }
   } // send
  
   int8_t send (Stream& s, const uint8_t epid, const char txt[]) { send(s, epid, (uint8_t*)txt, lentil(txt)); }

   int8_t recv (uint8_t b[], const int8_t nMax, uint8_t& epid, Stream& s)
   {
      int8_t n= s.available();
      if (n > 0)
      {
         uint8_t hdr= s.peek();
         if (0xC0 != (hdr & 0xC0)) { epid= 0xFF; return s.readBytes(b, min(n, nMax)); }
         else
         {
            epid= (hdr >> 4) & 0x3;
            uint8_t l= 1 + (hdr & 0xF);
            if ((nMax >= l) && (n >= (sizeof(hdr)+l)))
            {  // avoid partial read or blocking
               s.read(); // skip the header
               return s.readBytes(b, l);
            }
            else { return(-l); }
         }
      }
      return(0);
   }
}; // CSerMux

#endif // SERMUX_HPP
