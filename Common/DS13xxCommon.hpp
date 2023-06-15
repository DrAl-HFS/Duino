// Duino/Common/DS13xxCommon.hpp - common source for Maxim/Dallas RTC
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors June 2023

#ifndef DS13xx_COMMON_HPP
#define DS13xx_COMMON_HPP


namespace DS13xxHW
{
  enum Reg : uint8_t {
    T_SS, T_MM, T_HH   // time
    // order varies between device versions
  };
  enum TDFlag : uint8_t { T_SS_STOP=0x80, T_HH_12H=0x40, T_HH_12H_PM=0x20 };
}; // DS13xxHW

#endif // DS13xx_COMMON_HPP
