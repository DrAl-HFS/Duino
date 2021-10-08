// Duino/Common/AVR/DA_SPIMHW.hpp - Arduino-AVR SPI Master Hardware wrapper
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#ifndef DA_CONFIG_HPP
#define DA_CONFIG_HPP

#include <EEPROM.h>


/***/

//char hackCh (char ch) { if ((0==ch) || (ch >= ' ')) return(ch); else return('?'); }

// TODO : wrap as ID class
int8_t setID (char idzs[])
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
} // setID

int8_t getID (char idzs[])
{
   int8_t i=0;
   do
   {
      idzs[i]= EEPROM.read(i);
   } while (0 != idzs[i++]);
   return(i);
} // getID

#include <avr/boot.h>
void sig (Stream& s)  // looks like garbage...
{ // avrdude: Device signature = 0x1e950f (probably m328p)
   char lot[7];
   s.print("sig:");
   for (uint8_t i= 0x0E; i < (6+0x0E); i++)
   { // NB - bizarre endian reversal
      lot[(i-0x0E)^0x01]= boot_signature_byte_get(i);
   }
   lot[6]= 0;
   s.print("lot:");
   s.print(lot);
   s.print(" #");
   s.print(boot_signature_byte_get(0x15)); // W
   s.print("(");
   s.print(boot_signature_byte_get(0x17)); // X
   s.print(",");
   s.print(boot_signature_byte_get(0x16)); // Y
   s.println(")");
   s.flush();
   for (uint8_t i=0; i<0xE; i++)
   { // still doesn't appear to capture everything...
      uint8_t b= boot_signature_byte_get(i);
      if (0xFF == b) { s.println(b,HEX); }
      else { s.print(b,HEX); }
      s.flush();
   }
   // Cal. @ 0x01,03,05 -> 9A FF 4F, (RCOSC, TSOFFSET, TSGAIN)
   // 1E 9A 95 FF FC 4F F2 6F
   // F9 FF 17 FF FF 59 36 31
   // 34 38 37 FF 13 1A 13 17
   // 11 28 13 8F FF F
} // sig


#endif // DA_CONFIG_HPP
