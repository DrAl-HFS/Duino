// Duino/Common/AVR/DA_Ident.hpp - AVR identity
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef DA_IDENT_HPP
#define DA_IDENT_HPP

#include <EEPROM.h>

class CIdent
{
public:
  CIdent (void) { ; }
  
  int8_t set (const char idzs[])
  {
    int8_t i=0;
    if (EEPROM.read(i) != idzs[i])
    {
      EEPROM.write(i,idzs[i]);
      if (0 != idzs[i++])
      {
        do
        {
          if (EEPROM.read(i) != idzs[i]) { EEPROM.write(i,idzs[i]); }
        } while (0 != idzs[i++]);
      }
    }
    return(i);
  } // set

  int8_t get (char idzs[])
  {
    int8_t i=0;
    do
    {
      idzs[i]= EEPROM.read(i);
    } while (0 != idzs[i++]);
    return(i);
  } // get
  
}; // CIdent

#endif // DA_IDENT_HPP
