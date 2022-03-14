// Duino/Common/CMX_Util.hpp - ARM Cortex (M3 & M4) utils
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2022

#ifndef CMX_UTIL
#define CMX_UTIL

#if 0
// STM32
//#include <libmaple/bitband.h>
// bb_perip(a,b)
#endif

// [Bit-twiddling hacks]
// Routines suitable for any CPU with fast 32bit shift & mask (& mult) - don't really belong here...
extern "C" {

// Count bits set in a 32bit word (12 ops inc. mult)
uint32_t bitCount32 (uint32_t v)
{
   v= v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
   v= (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
   return((((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24); // bracket + to prevent compiler grumble
} // bitCount32

uint8_t bitRev32 (uint32_t v)
{
   v= ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
   // swap consecutive pairs
   v= ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
   // swap nibbles ...
   v= ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
   // swap bytes
   v= ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
   // swap 2-byte long pairs
   return((v >> 16) | (v << 16));
} // bitRev32

#if 0 // ???
uint8_t bitRev8 (uint8_t b)
{
   return((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
} // bitRev8
#endif

// Read Bytes Little Endian : assemble unaligned bytes LSB to MSB as an unsigned 32bit word
// NB: simple but not efficient due to 4 memory accesses!
// Can be done in 2 accesses but risks accessing
uint32_t rdble (const uint8_t b[], const int n)
{
   uint32_t w= 0;
   if (n > 0)
   {
      w= b[0];
      for (int i=1; i<n; i++) { w|= b[i] << (i<<3); }
   }
   return(w);
} // rdble

}; // extern "C"
// [Bit-twiddling hacks]

namespace CMX {

// Bitband address mapping (Cortex M3 & M4 only)

// TODO: check whether defs are specific to STM32
#define BB_ADRSPC_MASK 0xF0000000   // RAM vs. peripheral
#define BB_OFFSET_MASK 0x000FFFFF   // 20bit offset (1MByte address space)
#define BB_REGION_MASK 0x02000000   // bitband address space designation

// Alternative (general purpose) bitband pointer function
volatile uint32_t *bbp (volatile void *p, const uint8_t b=0)
{
   ASSERT(b == (b & 0x1F)); // 0x7 byte-wise ???
   ASSERT(0 == ((uint32_t)p & 0x0FF00000));
   uint32_t w= (uint32_t)p;
   uint32_t o= w & BB_OFFSET_MASK;
   w&= BB_ADRSPC_MASK;
   w|= BB_REGION_MASK + (o<<5) + (b<<2);
   return((uint32_t*)w);
} // bbp

#define SYST_CVR ((uint32_t*)0xE000E018)
#ifndef BIT_MASK
#define BIT_MASK(b) ((1<<(b))-1)
#endif

class SysTickIvl
{
   uint32_t last;

   uint32_t read (void) { return(BIT_MASK(24) & *SYST_CVR); }

public:
   SysTickIvl (void) { last= read(); }

   uint32_t delta (void)
   {
      uint32_t d, t= read();

      if (last > t) { d= last - t; }  // NB: countdown
      else { d= last + (1 << 24) - t; } // wrap
      last= t;
      return(d);
   } // delta

}; // class SysTickIvl

}; // namespace CMX

#endif // CMX_UTIL
