// Duino/Common/AVR/DA_Analogue_HPP.hpp - Arduino-AVR interfacing to ADC and experimental PWM-DAC
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#ifndef DA_ANALOGUE_HPP
#define DA_ANALOGUE_HPP

//#include <?>


/***/


/***/

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

// Collect multiplexed
#define ANLG_MUX_SH (2)
#define ANLG_MUX_MAX (1<<ANLG_MUX_SH)
#define ANLG_MUX_MSK  (ANLG_MUX_MAX-1)

#define ANLG_VQ_SH (4)
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
      vmux[0]= 0xC0; // A0 / 1.1Vref
      vmux[1]= 0xC1; // A1 / 1.1Vref
      vmux[2]= 0xC2; // A2 / 1.1Vref
      vmux[3]= 0xC8; // thermistor (A8) / 1.1Vref ~0x0161
   } // CTOR

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void init (uint8_t id=0)
   {
      nE= nR= 0;
      set(id);
      ADCSRA= (1<<ADEN) | (1<<ADIE) | 0x07; // ADC enable, Interrupt enable, clock prescaler 128 -> 125kHz sampling clock
      // 1<<ADATE; auto trigger enable
      ADCSRB= 0x00; // Free run (when auto-trigger)
      PORTC&= 0xF0;
      DDRC&= 0xF0;
      DIDR0= 0x0F; // Disable digital input buffer on A0-A3 ; A5-A4 -> 0x3F
   } // init

   void set (uint8_t muxID=0) { id= muxID & ANLG_MUX_MSK; ADMUX= vmux[id]; }
   uint16_t readImmed (void)
   {
      ADCSRA= (1<<ADEN) | (1<<ADSC) | (ADCSRA & 0x7);
      while (ADCSRA & (1<<ADSC)); // spin
      return(ADCW);
   }
   uint16_t readImmedQuiet (void)
   {
      ADCSRA= (1<<ADEN) | (1<<ADIE) | (1<<ADSC) | (ADCSRA & 0x7);
      //set_sleep_mode(SLEEP_MODE_ADC);
      do
      {
         sleep_cpu();
      } while (ADCSRA & (1<<ADSC)); // spin
      return(ADCW);
   }
   void next (void) { set(id+1); }
   void start (void) { ADCSRA|= (1<<ADSC); }
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
      //v[i]= ADCL | (ADCH << 8) | (id << 12);
      v[i]= ADCW | (id << 12);
      ++nE;
   } // event

   uint8_t avail (void)
   {
      int8_t n= nE - nR;
      if (n < 0) { n+= 0xFF; }
      return(n); // min(n,ANLG_VQ_MAX);
   } // avail

   // return channel index or -1 if no data
   int8_t get (uint16_t& r)
   {
      if (avail() <= 0) { return(-1); }
      uint8_t i= nR & ANLG_VQ_MSK;
      uint16_t t= v[i];
      nR++;
      r= t & 0xFFF;     // trust compiler to generate efficient
      return(t >> 12); // byte & nybble handling
   } // get
}; // CAnalogue

class CAnalogueDbg : public CAnalogue
{
public:
   CAnalogueDbg (void) { ; }

   // Debug
   int8_t dump (char s[], int8_t max)
   {
      int8_t n= snprintf(s,max," R%dE%d", nR, nE);
      for (int8_t i=0; i<ANLG_VQ_MAX; i++) { n+= snprintf(s+n, max-n," %X", v[i]); }
      return(n);
   } // dump
}; // CAnalogueDbg

// PWM needs timer/counter - plenty extra on Mega series
// at 1kHz update rate (standard for general purpose task management) a
// 470 Ohm resistor & 10uF capacitor gives reasonable smoothing. 
// At 10kHz : 680 Ohm, 4.7uF is OK
class CFastPulseDAC
{
public:
   CFastPulseDAC (void) { ; }

   int8_t init (uint8_t m)
   {
#ifdef ARDUINO_AVR_MEGA2560 // 640, 1280 ??? 
      // Use timer 3 (provides triple compare channels)
      //pinMode(5, OUTPUT);  pinMode(2, OUTPUT);  pinMode(3, OUTPUT); // Duino pin numbers for OC3 A,B,C
      DDRE|= 0x38; // PORTE 3,4,5 = OC3 A,B,C
      //  - wgm=0101 - fast 8b PWM
      //  - com=10 - f8pwm, com=00 - ignore
      //  - cs=001 - clk/1 (16MHz)
      TCCR3A= 0b10101001;   // com:a,b,c:2,wgm:2
      TCCR3B=    0b01001;   // wgm:2,cs:3
      //TCCR3C= 0;
#else
      // Timer 0/1 ?
#endif
      return(0);
   }
   void set (uint8_t vA, uint8_t vB, uint8_t vC)
   { 
#ifdef ARDUINO_AVR_MEGA2560
      OCR3A= vA;
      OCR3B= vB;
      OCR3C= vC;
#else
#endif
   }
   void set (uint8_t v) { set(v,v,v); }
   //uint16_t get (void) { return(TCNT3); }
}; // CFastPulseDAC

#endif //  DA_ANALOGUE_HPP

