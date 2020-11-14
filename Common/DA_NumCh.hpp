// Duino/Common/numCh.hpp - Arduino-AVR numeric/char hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Nov 2020

#ifndef DA_NUM_CH_H
#define DA_NUM_CH_H

// TODO : determine any effect of linkage type on AVR code generation
#if 0 //def __cplusplus
extern "C" {
#endif

//#include <?>


/***/

// Processor specific variants

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

int8_t hex2ChU8 (char c[2], uint_fast8_t u)
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(swapHiLo4U8(u));
   return(2);
} // hex2ChU8

// 8 bit value conversion to packed 2 digit BCD without range check
uint_fast8_t conv2BCD4 (uint_fast8_t u) // RENAME toBCD4 ????
{  // if (v > 99) { return(v); }
   uint_fast8_t q= u / 10; // assume u < 100
   u-= 10 * q; // remainder <10
   return(swapHiLo4U8(q)|u);
} // conv2BCD4

uint_fast8_t fromBCD4 (uint_fast8_t bcd, uint_fast8_t n)
{
   uint_fast8_t r= swapHiLo4U8(bcd) & 0xF;
   if (n > 1) { r= 10 * r + (bcd & 0xF); }
   return(r);
} // fromBCD4

#if 0 //def __cplusplus
} // extern "C"
#endif

#endif //  DA_NUM_CH_H

