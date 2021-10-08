// Duino/Common/AVR/DA_Therm.hpp - Arduino-AVR interfacing to ADC and experimental PWM-DAC
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Sept 2021

#ifndef DA_THERM_HPP
#define DA_THERM_HPP

#include "DA_Analogue.hpp"


/***/


/***/

// AVR buit-in thermistor hackery
int8_t convT1 (int32_t t)
{
   return(((t-353) * 0x4F) >> 7);
   //return(((t-352) * 13) >> 4);
} // convT1

int8_t convTherm (uint16_t t)
{
   return(25 + convT1(t));
#if 0
   int32_t tC= (int)t - 0x160;
   tC= (tC * 207) / 128; // (reciprocated scale - reduce division)
   tC+= 25;
   // Device signature calibration: TSOFFSET=FF, TSGAIN=4F ???
   // Looks like available docs are totally wrong on this.
   //tC= (tC * 128) / 0x4F;
    //tC= 25+(((int)t-((int)273+80))*13)/16;
   return(tC);
#endif
} // convTherm

// Thermistor MF58-103 (10k nominal)
// Basic 1:1 divider wiring options: 
//  (Vs)-[Rt]-(A#)-[Rd]-(Gnd) - hi side thermistor
//  (Vs)-[Rd]-(A#)-[Rt]-(Gnd) - lo "    "
// where Rt= MF58-103, Rd= 10k resistor, A# analogue pin, Vs=3.3V
// --
// In this configuration Vs is also connected to Va (analogue reference)
// meaning the internal thermistor cannot be used (requires internal 1.1V
// ref with Va floating). A MOSFET switch could be used to disconnect Vs
// from Va as needed (saving ~165uA).
// --
// Alternatively, a 10k resistor added in parallel with Rt and reducing
// Rd to 1k (yielding 5:1 divider) in the hi side configuration allows
// the internal 1.1V ref to be used for the external thermistor.
// A parallel resistor is alleged to improve linearity (to be tested).
// --
// Here we circumvent Voltage, Resistance etc. calculations, relying instead
// upon a simple linear heuristic model (using an observed calibration value).
class ThermNTC : public CAnReadSync
{
private: // 505 10k series, 696 10k series & parallel, 
   // 25C hi side value
   const int16_t cal25= 521; // 1k series with 10k parallel and 1.1Vref
   // high & lo side variants as wiring above
   const int16_t hiRefV= cal25 << 4;
   int16_t dHi (uint16_t vHi_4) { return (int16_t)(vHi_4 - hiRefV); }

   const int16_t loRefV= ((1<<10) - cal25) << 4;
   int16_t dLo (uint16_t vLo_4) { return (int16_t)(loRefV - vLo_4); }

public:
   ThermNTC (void) : CAnReadSync() {;}

   void init (void) { CAnMux::init(2); }

   int8_t conv (uint16_t v_4)
   { // linear approximation to divider response (flattened sigmoid curve)
      return(25 + (dHi(v_4) / 0x64));  // 0x70 for series only
   }

   //uint16_t raw (void) { return CAnReadSync::read(); }
   void log (Stream& s)
   {
      uint16_t r= 0;
      r= readSumND(4); // readSumNDQ(4) >> 2; //
      s.print("Ext:"); s.print(r); s.print('>'); s.print(conv(r<<2)); s.println('C');

      set(3); // internal thermistor
      r= readSumND(4);
      s.print("Int:"); s.print(r); s.print(">"); s.print(convTherm(r>>2)); s.println('C');
      set(2); // external
   }
}; // ThermNTC

#endif // DA_THERM_HPP
