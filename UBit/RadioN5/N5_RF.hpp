// Duino/UBit/RadioN5/N5_RF.hpp - nrf51822 radio hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef N5_RF_HPP
#define N5_RF_HPP

#include "Common/N5/N5_Util.hpp"

//#ifdef TARGET_NRF52
//#define NRF52   doesn't propagate to required build point
//#endif
//#include <RadioHead.h>
#include <RH_NRF51.h>


/***/

int8_t getState (NRF_RADIO_Type *pR=NRF_RADIO) { return(pR->STATE); }

int8_t getRSSI (int8_t chan=-1, NRF_RADIO_Type *pR=NRF_RADIO)
{  // typically <2us
   if ((chan >= 0) && (chan <= 125)) { pR->FREQUENCY= chan; } // (chan<<0) & 0x7f
   pR->TASKS_RSSISTART= 1;
   while (0 == pR->EVENTS_RSSIEND); // spin
   return(-(pR->RSSISAMPLE)); // -> -dB
} // getRSSI

struct ScanDatum
{ 
   int8_t rMax; int16_t rSum; uint8_t n;
   
   ScanDatum (void) { rMax= -128; rSum= 0; n= 0; }
   
   void add (int8_t r)
   {
      if (n > 100)
      {  // reset
         rMax= r; rSum= r; n= 1;
      }
      else
      {
         rMax= max(rMax, r);
         rSum+= r;
         n++;
      }
   }
   void print (Stream& s) const
   {
      s.print(' '); s.print(rMax);
      if (rMax > -70)
      { 
         s.print(' '); s.print(n);
         s.print(' '); if (n > 1) { s.print(rSum / n); } else { s.print(rSum); }
      }
   }
}; // ScanDatum

struct ChanSD : ScanDatum
{
   int8_t chan;

   ChanSD (int8_t c=-1) : ScanDatum() { if (c > 125) { chan= 125; } else { chan= c; } }

   void print (Stream& s) const
   {
      s.print(" Ch"); s.print(chan);
      ScanDatum::print(s);
   }
}; // ChanSD

#define N5_CHAN_MAX (125)

class CRFScan
{
public:
   ChanSD sd[N5_CHAN_MAX];
   
   CRFScan (int8_t chanBase=-1, int8_t n=1, uint8_t s=1)
   {
      if (chanBase >= 0)
      {
         if ((chanBase + n) > N5_CHAN_MAX) { n= N5_CHAN_MAX - chanBase; }
         for (int8_t i=0; i<n; i++) { sd[i]= ChanSD(chanBase+(i*s)); }
      }
   }
   
   void scan (int8_t n=1)
   {
      if (n > N5_CHAN_MAX) { n= N5_CHAN_MAX; }
      for (int8_t i=0; i<n; i++) { sd[i].add( getRSSI( sd[i].chan ) ); }
   }
   
   void print (Stream& s, int8_t n=1, int8_t i0=0) const
   {
      int8_t iM= i0+n;
      if (iM > N5_CHAN_MAX)
      { 
         n= N5_CHAN_MAX - i0;
         iM= i0+n;
      }
      for (int8_t i=i0; i<iM; i++)
      { 
         if (i < iM) { s.println(); } 
         sd[i].print(s);
      }
   }
}; // CRFScan

#endif // N5_RF_HPP
