// Duino/UBit/N5Common/N5_HWID.hpp - Micro:Bit V1 (nrf51822) hardware ID
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef N5_HWID
#define N5_HWID

#include <nrf51.h>
#include <nrf51_bitfields.h>


/***/

void n5DumpHWID (Stream &s, const NRF_FICR_Type *pF=NRF_FICR)
{
  s.print("\nUBit: "); s.println(0xFFFF & pF->CONFIGID,HEX); // HWID
  s.print("UID= 0x"); s.print(pF->DEVICEID[1],HEX); s.println(pF->DEVICEID[0],HEX); // Unique
  {
    const char apr[]={'P','R'};
    s.print("Addr= ["); s.print(apr[0x1 & pF->DEVICEADDRTYPE]);  s.print("] 0x");
    s.print(0xFFFF & pF->DEVICEADDR[1],HEX); s.println(pF->DEVICEADDR[0],HEX);
  }
  s.print("Flash= "); s.print(pF->CODEPAGESIZE); s.print("byte*"); s.println(pF->CODESIZE);
  s.print("Ram= "); s.print(pF->SIZERAMBLOCKS); s.print("byte*"); s.println(pF->NUMRAMBLOCK);
  s.print("Ovrd= 0x"); s.println(~(pF->OVERRIDEEN),HEX);
} // n5DumpHWID

#endif // N5_HWID
