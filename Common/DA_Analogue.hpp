// Duino/Common/DA_Analogue_HPP.hpp - Arduino-AVR interfacing to ADC
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020

#ifndef DA_ANALOGUE_HPP
#define DA_ANALOGUE_HPP

//#include <?>

//#if 0 //def __cplusplus
//extern "C" {
//#endif

//#if 0 //def __cplusplus
//} // extern "C"
//#endif

/***/


/***/

#define ANLG_VQ_SH (1)
#define ANLG_VQ_MAX (1<<ANLG_VQ_SH)
#define ANLG_VQ_MSK  (ANLG_VQ_MAX-1)

class CAnalogue
{
protected:
   volatile uint16_t v[ANLG_VQ_MAX]; // Latest values
   volatile uint8_t nE; // Event (ISR reading) counter
   uint8_t nR;

public:
   CAnalogue (void) {;}

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void init (void)
   {
      ADMUX=  0x48; // Vcc ref, right justify, chan.8 (temp. sensor)
      ADCSRA= (1<<ADEN) | (1<<ADIE) | 0x07; // ADC enable, Interrupt enable, clock prescaler 128 -> 125kHz sampling clock
      ADCSRB= 0x00; // Free run auto-trigger
      //DIDR0=
   } // start
   void start (void) { ADCSRA|= 1<<ADSC; }
   void event (void)
   {
      uint8_t i= nE & ANLG_VQ_MSK;
      v[i]= ADCL | (ADCH << 8);
      v[i]|= (ADMUX & 0xF) << 12;
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
      r= t & 0xFFF;     // trust compiler to generate efficient byte & 
      return(t >> 12); // nybble handling
   } // get

   // Debug
   int8_t dump (char s[], int8_t max) { return snprintf(s,max," E%d v:%d,%d", nE, v[0], v[1]); }
}; // CAnalogue



#endif //  DA_ANALOGUE_HPP

