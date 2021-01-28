// Duino/UBit/Common/M0_Util.hpp - ARM Cortex M0+ general utilities
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef M0_UTIL
#define M0_UTIL

// TODO : assess mod&div performance on NRF51 (no HW div on Cortex-M0+)
// Compute numerator / divisor, store quotient & return remainder,.
uint32_t divmod (uint32_t& q, uint32_t n, uint32_t d) { q= n / d; return(n % d); }

int u8ToBCD4 (uint8_t bcd[1], uint8_t u)
{
   if (u > 99) { return(-1); }
   if (u <= 9) { bcd[0]= u; return(u>0); }
   uint8_t q= u / 10; // assume efficient synthesis of division for small constants
   u-= 10 * q; // remainder
   bcd[0]= (q << 4) | u;
   return(2);
} // u8ToBCD4

int u16ToBCD4 (uint8_t bcd[2], uint16_t u)
{
   if (u > 9999) { return(-1); }
   int8_t d= 0;
   if (u > 99)
   {  
      uint32_t q;
      u8ToBCD4(bcd+1, divmod(q,u,100));
      return(2 + u8ToBCD4(bcd+0,q));
   }//else
   bcd[0]= 0;
   return u8ToBCD4(bcd+1,u);
} // u16ToBCD4

/*** COMMON ***/
// Return ASCII hex for low 4 bits (nybble)
char hexCharL4 (uint_fast8_t u)
{
   u&= 0xF;
   if (u < 0xa) { return('0' + u); } else { return('a'-9 + u); }
} // hexCharL4
/*** COMMON ***/

int8_t hex2ChU8 (char c[2], uint_fast8_t u)
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(u >> 4);
   return(2);
} // hex2ChU8

int bcd4ToChar (char ch[], int maxCh, const uint8_t bcd[], int nBCD)
{
   int i= 0, nCh= 0;
   while ((i < nBCD) && (nCh < maxCh))
   {
      nCh+= hex2ChU8(ch+nCh, bcd[i++]);
   }
   return(nCh);
} // bcd4ToChar

uint32_t sec2HMS (uint8_t hms[3], uint32_t sec)
{
   hms[2]= divmod(sec, sec, 60);
   hms[1]= divmod(sec, sec, 60); 
   hms[0]= divmod(sec, sec, 24);
   return(sec); // days
} // sec2HMS

#endif // M0_UTIL
