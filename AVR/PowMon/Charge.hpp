// Duino/AVR/PowMon/Charge.hpp - Simple battery charge state/control
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors July 2021

#ifndef CHARGE_HPP
#define CHARGE_HPP

#include "Common/AVR/DA_Analogue.hpp"

// ADC max=1023 @ 1.1V ref (caveat: mfr. tol. +-10% aargh!)
// Designed 8k:0.8k divider, seems OK ~1%
// For 5.1V (USB) input, expected: 5.1/11 * 1023/1.1 = 431
// Measured value seems stable around 440 (+2%), so use scale 5100/440 = 11.59
// 4b fraction gives tolerable scale accuracy: 185/16 = 11.5625 (0.3% under)
uint16_t raw2mV (uint32_t raw)
{  
   return((raw * 185) >> 4); // <= 24b intermed. prec.
   //return((raw * 2967) >> 8); // <= 28b
} // raw2mV

#define INST_SYSV 0
#define INST_BATV 1
#define INST_PNLV 2
#define INST_TC		3
#define RD_AVG 3
// Instrumentation using ADC
class CInstrmnt : public CAnalogue
{
public:
	 uint16_t mV[3];
	 uint8_t sysTC;
	 
	 CInstrmnt (void) { ; }
	 
	 void update ()
	 {
		  uint16_t s;
		  set_sleep_mode(SLEEP_MODE_ADC);
		  CAnalogue::set(INST_BATV); readImmedQuiet(); s= 0;
		  for (int8_t i=0; i<RD_AVG; i++) { s+= readImmedQuiet(); }
		  mV[INST_BATV]= raw2mV(s/RD_AVG);
		  CAnalogue::set(INST_PNLV); readImmedQuiet(); s= 0;
		  for (int8_t i=0; i<RD_AVG; i++) { s+= readImmedQuiet(); }
		  mV[INST_PNLV]= raw2mV(s/RD_AVG);
		  CAnalogue::set(INST_SYSV); s= readImmedQuiet(); 
		  mV[INST_SYSV]= raw2mV(s);
		  CAnalogue::set(INST_TC); s= readImmedQuiet();
		  sysTC= convTherm(s);
		  set_sleep_mode(SLEEP_MODE_IDLE);
	 }
	 
	 bool updateAsync (void)
	 {
			uint16_t r;
		  int8_t c, nD=0;
		  if (CAnalogue::avail() > 0)
		  {
				 do
				 {
						c= CAnalogue::get(r);
						if (c >= 0)
						{
							 if (c <= INST_PNLV)
							 {
									r= raw2mV(r);
									if (r != mV[c]) { mV[c]= r; nD++; }
							 }
							 else if (3 == c) { sysTC= convTherm(r); }
						}
				 } while (c >= 0);
		  }
		  return(nD > 0);
	 }
}; // CInstrmnt

class CCharge : public CInstrmnt
{
public:
	
	 CCharge (void) { ; }
}; // CCharge

#endif // CHARGE_HPP
