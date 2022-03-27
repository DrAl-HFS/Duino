// Duino/Common/CDS18.hpp - class wrapper for Dallas one wire temperature probe
// https://github.com/DrAl-HFS/Duino.git ?
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#ifndef CDS18_HPP
#define CDS18_HPP

#include <OneWire.h>

#include "DN_Util.hpp"

void dumpHex (Stream& s, uint8_t b[], const int8_t n, const char *end=NULL)
{
  dumpHexFmt(s,b,n);
  if (end) { s.print(end); }
} // dumpHex

namespace DS18
{
   enum Cmd : uint8_t { START_CONV=0x44, READ_SCRATCH=0xBE };
   enum ID : uint8_t { B20=0x28, };
}; // namespace DS18

class CDS18 : public OneWire
{
protected:
  uint8_t id[8], c, f;

  bool rsw (uint8_t cmd)
  {
    f= OneWire::reset();
    OneWire::select(id);
    OneWire::write(cmd);
    return(f > 0);
  } // rsw

public:
  CDS18 (uint8_t pin=8) : OneWire(pin), f{0}, c{0} { ; }

  bool init (void)
  {
    bool r= OneWire::search(id);
    if (r) { r= (OneWire::crc8(id, 7) == id[7]); }
    if (r) switch(id[0])
    {
      case 0x10 :
      case DS18::B20 : // DS18B20
      case 0x22 :
        r= start();
        break;
      default : r= false;
    }
    if (!r) { f= 0; OneWire::reset_search(); }
    return(r);
  }

  bool start (void) { return rsw(DS18::START_CONV); }

  int16_t readRaw (uint8_t v[9])
  {
    rsw(DS18::READ_SCRATCH);
    OneWire::read_bytes(v,9);
    if (OneWire::crc8(v, 8) == v[9]) { f|= 0x2; }
    return((v[1] << 8) | v[0]);
  } // readRaw

}; // CDS18

class CDS18Debug : public CDS18
{
public:
  CDS18Debug (uint8_t pin=8) : CDS18(pin) { ; }

  void clrb (uint8_t b[], const int8_t n) { for (int8_t i=0; i<n; i++) { b[i]= ((1+i)<<4)|i; } }

  void log (Stream&s)
  {
    uint8_t v[12];
    int16_t r=-1;
    uint16_t f;

    clrb(v,12);
    r= readRaw(v);
    s.print("DS["); s.print(f,HEX); s.print(','); s.print(c,HEX); s.print("]:");
    dumpHex(s,id,8,":");//,0x0,-1);
    dumpHex(s,v,12,":");//,0x0,-1);
    f= (r & 0xF) * 625;
    s.print(r>>4); s.print('.'); if (f < 1000) { s.print('0'); } s.println(f);
  }

}; // CDS18Debug

#endif // CDS18_HPP
