// Duino/Common/M0_Util.hpp - ARM Cortex M0+ general utilities (uBitV1, RP2040)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef M0_UTIL
#define M0_UTIL

#if 0 //def TARGET_RP2040
#include "pico/stdlib.h"
#include "hardware/divider.h"
// TODO assess efficiency on RP2040 (hardware divider "coprocessor").
uint32_t divmod32 (uint32_t& q, const uint32_t n, const uint32_t d)
{  // divmod_* unknown ?!?
   divmod_result_t dmr= divmod_u32u32(n,d);
   uint32_t r; q= divmod_u32u32_rem(n,d,&r); return(r);
} // divmod32
#else
// TODO : assess mod&div performance on NRF51 (no HW div on Cortex-M0+)
// Compute numerator / divisor, store quotient & return remainder.
uint32_t divmod32 (uint32_t& q, const uint32_t n, const uint32_t d) { q= n / d; return(n % d); }
#endif

// Packed BCD remains useful as an intermediate format for stream IO
// as it results in very compact and efficient code for the majority
// of simple use cases. General C string handling becomes practical
// for more complex issues (eg. floating point) but incurs significant
// cost...

#if 0 // -> DN_Util.hpp
int bcd4FromU8 (uint8_t bcd[1], uint8_t u)
{
   if (u > 99) { return(-1); }
   if (u <= 9) { bcd[0]= u; return(u>0); }
#if 1
   uint32_t q;
   u= divmod32(q, u, 10); // assume remainder comes efficiently out of division
#else // avr style assumes fast/efficient code synthesis:
   uint8_t q= u / 10;  // 1) division by small constant (?)
   u-= 10 * q; // 2) multiplication to synthesise remainder (multi-cycle on nRF51)
#endif
   bcd[0]= (q << 4) | u;
   return(2);
} // bcd4FromU8
#endif

int bcd4FromU16 (uint8_t bcd[2], uint16_t u)
{
   if (u > 9999) { return(-1); }
   if (u > 99) // likely
   {
      uint32_t q;
      bcd4FromU8(bcd+1, divmod32(q,u,100));
      return(2 + bcd4FromU8(bcd+0,q));
   }//else
   bcd[0]= 0;
   return bcd4FromU8(bcd+1,u);
} // bcd4FromU16

// DEPRECATE ? Packed BCD string of n digits to ASCII
int bcd4ToChar (char ch[], int maxCh, const uint8_t bcd[], int n)
{
   int i= 0, nCh= 0;
   while ((i < n) && (nCh < maxCh))
   {
      nCh+= hexChFromU8(ch+nCh, bcd[i++]);
   }
   return(nCh);
} // bcd4ToChar


uint32_t hmsU8ToSecU32 (uint8_t hms[3]) { return(hms[2] + 60 * (hms[1] + (60 * hms[0]))); }

uint32_t hmsU8FromSecU32 (uint8_t hms[3], uint32_t t)
{
   hms[2]= divmod32(t, t, 60); // sec
   hms[1]= divmod32(t, t, 60); // min
   hms[0]= divmod32(t, t, 24); // hr
   return(t); // days
} // hmsU8FromSecU32

#endif // M0_UTIL
