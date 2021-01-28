// Duino/Common/M0_Util.hpp - ARM Cortex M0+ general utilities (uBitV1)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef M0_UTIL
#define M0_UTIL

// TODO : assess mod&div performance on NRF51 (no HW div on Cortex-M0+)
// Compute numerator / divisor, store quotient & return remainder,.
uint32_t divmod (uint32_t& q, uint32_t n, uint32_t d) { q= n / d; return(n % d); }

int bcd4FromU8 (uint8_t bcd[1], uint8_t u)
{
   if (u > 99) { return(-1); }
   if (u <= 9) { bcd[0]= u; return(u>0); }
   uint8_t q= u / 10; // assume efficient synthesis of division for small constants
   u-= 10 * q; // remainder
   bcd[0]= (q << 4) | u;
   return(2);
} // bcd4FromU8

int bcd4FromU16 (uint8_t bcd[2], uint16_t u)
{
   if (u > 9999) { return(-1); }
   int8_t d= 0;
   if (u > 99)
   {  
      uint32_t q;
      bcd4FromU8(bcd+1, divmod(q,u,100));
      return(2 + bcd4FromU8(bcd+0,q));
   }//else
   bcd[0]= 0;
   return bcd4FromU8(bcd+1,u);
} // bcd4FromU16

uint8_t bcd4ToU8 (uint8_t bcd, int8_t n)
{
   uint8_t r= (bcd >> 4); // & 0xF;
   if (n > 1) { r= 10 * r + (bcd & 0xF); }
   return(r);
} // bcd4ToU8

uint8_t bcd4FromChar (const char ch[2], int8_t n)
{
   uint8_t r= 0;
   if (n > 0)
   {
      r= 0xF & (ch[0] - '0');
      if (n > 1)
      {
         r= (r<<4) | (0xF & (ch[1] - '0'));
      }
   }
   return(r);
} // bcd4FromChar

/*** COMMON ***/
// Return ASCII hex for low 4 bits (nybble)
char hexCharL4 (uint8_t u)
{
   u&= 0xF;
   if (u < 0xa) { return('0' + u); } else { return('a'-9 + u); }
} // hexCharL4
/*** COMMON ***/

int8_t hexCharU8 (char c[2], uint8_t u)
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(u >> 4);
   return(2);
} // hexCharU8

int bcd4ToChar (char ch[], int maxCh, const uint8_t bcd[], int nBCD)
{
   int i= 0, nCh= 0;
   while ((i < nBCD) && (nCh < maxCh))
   {
      nCh+= hexCharU8(ch+nCh, bcd[i++]);
   }
   return(nCh);
} // bcd4ToChar

uint32_t hmsU8ToSecU32 (uint8_t hms[3]) { return(hms[2] + 60 * (hms[1] + (60 * hms[0]))); }

uint32_t hmsU8FromSecU32 (uint8_t hms[3], uint32_t sec)
{
   hms[2]= divmod(sec, sec, 60);
   hms[1]= divmod(sec, sec, 60); 
   hms[0]= divmod(sec, sec, 24);
   return(sec); // days
} // hmsU8FromSecU32

#endif // M0_UTIL
