// Duino/Common/AVR/DA_Util.h - Arduino-AVR miscellaneous utility code
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2020 - Jan 2021

#ifndef DA_UTIL_H
#define DA_UTIL_H

// TODO : determine any effect of linkage type on AVR code generation
#if 0 //def __cplusplus
extern "C" {
#endif

#ifdef ARDUINO_ARCH_AVR
#endif

//#include <?>


/***/

// Processor specific variants - leave in as build sanity check...
// TODO: verify use of {u}int_fast8_t is (probably) a waste of time for AVR target...

#ifdef AVR
// GCC extended assembler syntax - "template : output : input {: clobber : goto}"
#define SWAP4U8(u) { asm("swap %0" : "=r" (u) : "0" (u)); }
#endif

#ifdef SAM  // SamD21/51
#endif

/***/

uint_fast8_t swapHiLo4U8 (uint_fast8_t u)
{
   SWAP4U8(u); // asm("swap %0" : "=r" (u) : "0" (u));
   return(u);
} // swapHiLo4U8

// Return ASCII for low 4 bits (nybble)
char hexCharL4 (uint_fast8_t u)
{
   u&= 0xF;
   if (u <= 9) { return(u+'0'); } else { return(u-9+'a'); }
} // hexCharL4

int8_t hex2ChU8 (char c[2], uint8_t u) // hexCharFromU8
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(swapHiLo4U8(u));
   return(2);
} // hex2ChU8

uint8_t char2BCD4 (char ch[], int8_t n) // bcd4FromChar
{
   uint8_t r= 0;
   if (n > 0)
   {
      r= 0xF & (ch[0] - '0');
      if (n > 1)
      {
         r= swapHiLo4U8(r) | (0xF & (ch[1] - '0'));
      }
   }
   return(r);
} // char2BCD4

// 8 bit value conversion to packed 2 digit BCD without range check
uint8_t conv2BCD4 (uint8_t u) // TODO : RENAME bcd4FromU8()
{  // if (v > 99) { return(v); }
   uint8_t q= u / 10; // assume u < 100
   u-= 10 * q; // remainder <10
   return(swapHiLo4U8(q)|u);
} // conv2BCD4

uint8_t fromBCD4 (uint8_t bcd, uint8_t n) // TODO : RENAME bcd4ToU8()
{
   uint8_t r= swapHiLo4U8(bcd) & 0xF;
   if (n > 1) { r= 10 * r + (bcd & 0xF); }
   return(r);
} // fromBCD4

// TODO : assess compiler generated (fast?) division by constant

// Time in minutes to days as byte, returns residue
uint_fast16_t convTimeD (uint8_t d[1], uint_fast16_t t)
{
  d[0]= t / (24 * 60);
  return(t - (d[0] * 24 * 60));
} // convTimeD

// Time in minutes to hours & minutes as bytes
void convTimeHM (uint8_t hm[2], uint_fast16_t t)
{
   hm[0]= t / 60;
   hm[1]= t - (hm[0] * 60);
} // convTimeHM

// Really 16bit -> BCD
// generate specified number of BCD bytes (yielding 2, 4 or 5 digits)
void convMilliBCD (uint8_t bcd[], const int8_t n, uint_fast16_t m)
{
   if (n > 0)
   {  // TODO : investigate feasibility of divmod()
      uint8_t c, s= m / 1000;
      bcd[0]= conv2BCD4(s); // set seconds digits
      if (n > 1)
      {
         m-= s * 1000;  // get remainder
         c= m / 10;
         bcd[1]= conv2BCD4(c);  // centi-seconds
         if (n > 2) { bcd[2]= swapHiLo4U8(m - c * 10); }
      }
   }
} // convMilliBCD

// Class dependancies...

bool beginSync (HardwareSerial& s, const uint32_t bd, const uint8_t cfg=SERIAL_8N1, int8_t n=20)
{
  if ((bd > 0) && (cfg > 0))
  {
    uint8_t d= n;
    do
    {
      if (s)
      {
        s.begin(bd,cfg);
        return(true);
      }
      else { delay(d); d= 1 + (d >> 1); }
    } while (--n > 0);
  }
  return(false);
} // beginSync

// Not AVR specific: displace to general Duino util
void dumplb (Stream& s, const char *h, const uint8_t b[], const int8_t n, const char *f=NULL)
{
  if (h) { s.print(h); }
  for (int8_t i=0; i<n; i++)
  { 
    s.print(b[i]);
    if (i < n) { s.print(','); }
  }
  if (f) { s.print(f); }
} // dumplb

#if 0 //def __cplusplus
} // extern "C"
#endif

#endif //  DA_UTIL_H

