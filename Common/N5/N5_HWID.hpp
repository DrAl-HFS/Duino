// Duino/Common/N5/N5_HWID.hpp - Micro:Bit V1 (nrf51822) hardware ID
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef N5_HWID
#define N5_HWID

#include "N5_Util.hpp"


/***/

#ifdef TARGET_NRF51
void nrf5DumpHWID (Stream &s, const NRF_FICR_Type *pF=NRF_FICR)
{
  s.print("\nUBitV1: "); s.println(0xFFFF & pF->CONFIGID,HEX); // HWID
  s.print("UID= 0x"); s.print(pF->DEVICEID[1],HEX); s.println(pF->DEVICEID[0],HEX); // Unique
  {
    const char apr[]={'P','R'};
    s.print("Addr= ["); s.print(apr[0x1 & pF->DEVICEADDRTYPE]);  s.print("] 0x");
    s.print(0xFFFF & pF->DEVICEADDR[1],HEX); s.println(pF->DEVICEADDR[0],HEX);
  }
  s.print("Flash= "); s.print(pF->CODEPAGESIZE); s.print("byte*"); s.println(pF->CODESIZE);
  s.print("Ram= "); s.print(pF->SIZERAMBLOCKS); s.print("byte*"); s.println(pF->NUMRAMBLOCK);
  s.print("Ovrd= 0x"); s.println(~(pF->OVERRIDEEN),HEX);
} // nrf5DumpHWID
#endif // TARGET_NRF51

#ifdef TARGET_NRF52
void nrf5DumpHWID (Stream &s, const NRF_FICR_Type *pF=NRF_FICR)
{
  s.print("\nUBitV2: P#"); s.print(pF->INFO.PART,HEX); 
  s.print(" V#"); s.print(pF->INFO.VARIANT,HEX); 
  s.print(" K#"); s.println(pF->INFO.PACKAGE,HEX);
  
  s.print("UID= 0x"); s.print(pF->DEVICEID[1],HEX); s.println(pF->DEVICEID[0],HEX); // Unique
  {
    const char apr[]={'P','R'};
    s.print("Addr= ["); s.print(apr[0x1 & pF->DEVICEADDRTYPE]);  s.print("] 0x");
    s.print(0xFFFF & pF->DEVICEADDR[1],HEX); s.println(pF->DEVICEADDR[0],HEX);
  }
  s.print("Flash= "); s.print(pF->CODEPAGESIZE); s.print("byte*"); s.println(pF->CODESIZE);
  s.print("Ram= "); s.println(pF->INFO.RAM); s.print("byte");
}
#endif // TARGET_NRF52

#endif // N5_HWID
