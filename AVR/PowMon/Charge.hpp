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
#define INST_TC   3
#define RD_AVG 3

struct PwrState
{
   uint16_t mVbat, mVpnl;
   int16_t diff (void) { return(mVpnl - mVbat); }
};

// Instrumentation using ADC
class CInstrmnt : public CAnReadSync
{
public:
   PwrState state[2];
   uint16_t mVsys;
   int8_t   tCsys;

   CInstrmnt (void) { ; }

   void update (uint8_t m)
   {
      if ((m & 0xF) > 0x1)
      {
         uint16_t s;
         set_sleep_mode(SLEEP_MODE_ADC);
         if (m & 0x2)
         {
            CAnMux::set(INST_BATV);
            s= readSumNDQ(RD_AVG);
            state[m&0x1].mVbat= raw2mV(s/RD_AVG);

            CAnMux::set(INST_PNLV);
            s= readSumNDQ(RD_AVG);
            state[m&0x1].mVpnl= raw2mV(s/RD_AVG);
         }
         if (m & 0x4)
         {
            CAnMux::set(INST_SYSV); 
            s= readSumNDQ(1);
            mVsys= raw2mV(s);
         }
         if (m & 0x8)
         {
            CAnMux::set(INST_TC);
            s= readSumNDQ(1);
            tCsys= convTherm(s);
         }
      set_sleep_mode(SLEEP_MODE_IDLE);
      }
   } // update

   int16_t stateDiff (uint8_t i=0) { return(state[i&0x1].diff()); }

   int8_t dumpS (char str[], int8_t m)
   {
      int8_t n;
      n= snprintf(str, m, " T=%dC", tCsys);
      n+= snprintf(str+n, m-n, " S=%dmV", mVsys);
      for (int8_t i=0; i < 2; i++)
      {
         n+= snprintf(str+n, m-n, " [%d]: %d %d", i, state[i].mVbat, state[i].mVpnl);
      }
      return(n);
   } // dumpS

}; // CInstrmnt

class CCharge : public CInstrmnt
{
public:
   uint16_t mVlim;
   int8_t set, cycle;

   CCharge (uint16_t mvFull=6600) : mVlim{mvFull} { ; }

   uint8_t update (uint8_t ssm)
   {
      int16_t dmV;
      ssm&= 0x1;
      if (--cycle <= 0)
      {
         cycle+= 20;
         ssm|= 0xE; // Full measurement
      }
      else { ssm|= 0x2; }
      CInstrmnt::update(ssm);
      dmV= mVlim - CInstrmnt::state[0].mVbat;
      if (dmV > 0)
      {
         if (dmV >= 170) { set= 19; }
         else { set= dmV / 8; }
         if (stateDiff() > 0) { return(cycle < set); }
      }
      return(0);
   }

   int8_t dumpS (char str[], int8_t m)
   {
      int8_t n;
      n= snprintf(str, m, " C/S%d,%d", cycle, set);
      return CInstrmnt::dumpS(str+n, m-n);
   }

}; // CCharge

#endif // CHARGE_HPP
