// Duino/Common/AVR/DA_Analogue_HPP.hpp - Arduino-AVR interfacing to ADC
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020

#ifndef DA_ANALOGUE_HPP
#define DA_ANALOGUE_HPP

//#include <?>


/***/


/***/

#define ANLG_MUX_SH (2)
#define ANLG_MUX_MAX (1<<ANLG_MUX_SH)
#define ANLG_MUX_MSK  (ANLG_MUX_MAX-1)

#define ANLG_VQ_SH (3)
#define ANLG_VQ_MAX (1<<ANLG_VQ_SH)
#define ANLG_VQ_MSK  (ANLG_VQ_MAX-1)

class CAnalogue
{
protected:
   uint8_t  vmux[ANLG_MUX_MAX];
   volatile uint16_t v[ANLG_VQ_MAX]; // Latest values
   volatile uint8_t nE; // Event (ISR reading) counter
   uint8_t id, nR;

public:
   CAnalogue (void)
   {
      vmux[0]= 0x40; // A0 / Vcc
      vmux[1]= 0xC0; // A0 / 1.1Vref
      // vmux[]= 0x4E; // Vcc / 1.1Vref (sanity check)
      vmux[2]= 0xCF; // Gnd / 1.1Vref (sanity check)
      vmux[3]= 0xC8; // thermistor (A8) / 1.1Vref ~0x0161
   } // CTOR

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void init (void)
   {
      nE= nR= 0; id= 0;
      ADMUX=  vmux[id];
      ADCSRA= (1<<ADEN) | (1<<ADIE) | 0x07; // ADC enable, Interrupt enable, clock prescaler 128 -> 125kHz sampling clock
      // 1<<ADATE; auto trigger enable
      ADCSRB= 0x00; // Free run (when auto-trigger)
      //DIDR0=
   } // start

   void set (uint8_t muxID=0) { id= muxID & ANLG_MUX_MSK; ADMUX= vmux[id]; }
   void start (void) { ADCSRA|= 1<<ADSC; }
   void stop (void) { ADCSRA&= ~(1<<ADSC); }
   void startAuto (void) { ADCSRA|= (1<<ADSC) | (1<<ADATE); }
   void stopAuto (void) { ADCSRA&= ~((1<<ADSC) | (1<<ADATE)); }
   // Clean event resolution count
   void flush (void) { nR= nE; }
   uint8_t flush (int8_t retain) // debug adjustment hack
   {
      uint8_t q= nE-retain;
      if (q > nR) { nR= q; }
      return avail();
   } // flush

   void event (void) // ISR
   {
      uint8_t i= nE & ANLG_VQ_MSK;
      v[i]= ADCL | (ADCH << 8) | (id << 12);
      ++nE;
   } // event

   uint8_t avail (void)
   {
      int8_t n= nE - nR;
      if (n < 0) { n+= 0xFF; }
      return(n); // min(n,ANLG_VQ_MAX);
   } // avail

   int8_t get (uint16_t& r)
   {
      if (avail() <= 0) { return(-1); }
      uint8_t i= nR & ANLG_VQ_MSK;
      uint16_t t= v[i];
      nR++;
      r= t & 0xFFF;     // trust compiler to generate efficient
      return(t >> 12); // byte & nybble handling
   } // get

   // Debug
   int8_t dump (char s[], int8_t max)
   {
      int8_t n= snprintf(s,max," R%dE%d", nR, nE);
      for (int8_t i=0; i<ANLG_VQ_MAX; i++) { n+= snprintf(s+n, max-n," %X", v[i]); }
      return(n);
   } // dump

}; // CAnalogue

#endif //  DA_ANALOGUE_HPP

