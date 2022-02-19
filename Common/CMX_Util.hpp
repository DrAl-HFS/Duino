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

}; // namespace CMX

#endif // CMX_UTIL
