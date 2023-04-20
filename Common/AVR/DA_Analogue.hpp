// Duino/Common/AVR/DA_Analogue.hpp - Arduino-AVR interfacing to ADC and experimental PWM-DAC
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#ifndef DA_ANALOGUE_HPP
#define DA_ANALOGUE_HPP

#include <avr/sleep.h>

#ifdef ARDUINO_AVR_MEGA2560
#define NO_IN_THERM
#endif

/***/


/***/

// convT* -> DA_Therm.hpp

// Collect multiplexed
#define ANLG_MUX_SH (3)
#define ANLG_MUX_MAX (1<<ANLG_MUX_SH)
#define ANLG_MUX_MSK  (ANLG_MUX_MAX-1)

class CAnMux
{
protected:
   uint8_t  vmux[ANLG_MUX_MAX];
   uint8_t  chan;

public:
   enum Mux : uint8_t { // 0..7 Analogue pins, 9..13 reserved
      // reference values to measure against
      REF_MASK=0xC0,
      AREF=0x00, VCC=0x40, 
#ifdef NO_IN_THERM // and others...
      IN_MASK=0x01F, IN5_MASK= 0x20,
      IN_BGREF1=0x1F, IN_GND=0x1F,  // special inputs
      BGREF1=0x80, BGREF2=0xC0      // dual bandgap refs
#else // 328P etc.
      IN_MASK=0x0F,
      IN_THERM=0x08, IN_BGREF1=0x0E, IN_GND=0x0F, // special inputs
      INVALIDREF=0x80, BGREF1=0xC0
#endif
   }; 

   void setSequence
   (
      int8_t n,   // number to set
      int8_t i=0, // starting index
      int8_t c=0, // starting mux input id (pin select)
      Mux ref=VCC // reference for all 
   )
   {
      ref&= REF_MASK;
      i&= ANLG_MUX_MSK;
      c&= IN_MASK;
      while (i<ANLG_MUX_MAX)
      { 
         vmux[i++]= ref | c;
         c= (c+1) & IN_MASK;
      }
   } // setSequence
   
   CAnMux (uint8_t profileID)
   {
      if (0 == profileID) { setSequence(ANLG_MUX_MAX); return; } //else
      int8_t i=0; // TODO: factor out application specific hacks...
#ifndef NO_IN_THERM // internal thermistor only on 328P and a few others
      if (0x1 & profileID) { vmux[i++]= BGREF1|IN_THERM; } // internal thermistor
#endif
      if (0x2 & profileID) { vmux[i++]= BGREF1|0; } // UV sensor (transimpedance amp output) / 1.1Vref
      if (0x4 & profileID) { vmux[i++]= VCC|IN_BGREF1; } // assess Vcc
      // VCC|1 : Vcr (return of remote Vcc powering therm dividers etc.)
      // VCC|2 : ext therm 1
      // VCC|3 : ext therm 2
      // others unused...
      setSequence(ANLG_MUX_MAX-i, i, 1, VCC); // fill out remaining defaults
   } // CTOR

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void init (uint8_t id=0)
   {
      set(id);
      PORTC&= 0xF0;
      DDRC&= 0xF0;
      DIDR0= 0x0F; // Disable digital input buffer on A0-A3 ; (A5-A4 -> 0x30)
   } // init

   uint8_t get (uint8_t chanID=0) { return(vmux[chanID & ANLG_MUX_MSK]); }
   void set (uint8_t chanID=0)
   { 
      chan= chanID & ANLG_MUX_MSK;
#ifdef ARDUINO_AVR_MEGA2560 // and others...
      const uint8_t mc= vmux[chan];
      ADMUX= mc & 0xDF; // NB: ADLAR0 = 0x20
      if (mc & IN5_MASK) { ADCSRB|= _BV(MUX5); } else { ADCSRB&= ~_BV(MUX5); }
#else
      ADMUX= vmux[chan];
#endif
   }
   void next (void) { set(chan+1); }
}; // CAnMux

class CAnCommon : public CAnMux
{
public:
   CAnCommon (uint8_t profileID) : CAnMux(profileID) { ; }
   
   void init (uint8_t clkPS)
   {
      ADCSRA= (1<<ADEN) | clkPS & 0x07; // Enable, clock prescaler 128 -> 125kHz sampling clock (~12kHz 10b sample rate?)
   }
   void on (void) { ADCSRA|= (1<<ADEN); }
   void off (void) { ADCSRA&= ~(1<<ADEN); }
   void start (void) { ADCSRA|= (1<<ADSC); }
   void stop (void) { ADCSRA&= ~((1<<ADSC) | (1<<ADIE)); }
}; // CAnCommon

// Synchronous analogue reading
class CAnReadSync : public CAnCommon
{
public:
   CAnReadSync (uint8_t profileID) : CAnCommon(profileID) { ; }

   void init (uint8_t c=0, uint8_t clkPS=0x7) { CAnMux::init(c); CAnCommon::init(clkPS); }
      
   uint16_t read (void)
   {
      ADCSRA= (1<<ADEN) | (1<<ADSC) | (ADCSRA & 0x7); // no interrupt
      while (ADCSRA & (1<<ADSC)); // spin
      //uint16_t r= ADCL; return(r | (ADCH<<8));
      return(ADCW);
   } // read
   
   uint16_t readSumND (const int8_t n, const int8_t d=1)
   {
      uint16_t s= 0;
      if ((n > 0) && (n <= 16)) // limit to 14bit sum
      {
         for (int8_t i=0; i<d; i++) { read(); } // discard
         for (int8_t i=0; i<n; i++) { s+= read(); }
      }
      return(s);
   } // readSumND
   
   // Quiet (low noise) versions
   
   // This must be called befor each batch of quiet readings to ensure correct behaviour
   void setSleepQ (void)
   {  // NB: interrupt must be defined (or no wake up)
      // and sleep must be enabled (or no noise reduction)
      set_sleep_mode(SLEEP_MODE_ADC);
      ADCSRA= (1<<ADEN) | (1<<ADIE) | (ADCSRA & 0x7); 
   } // setSleepQ
   
   uint16_t readQ (void)
   {
      start();
      do { sleep_cpu(); } while (ADCSRA & (1<<ADSC)); // spin
      return(ADCW);
   } // readQ
   
   uint16_t readSumNDQ (const int8_t n, const int8_t d=1)
   {
      uint16_t s=0;
      if ((n > 0) && (n <= 16)) // limit to 14bit sum
      {
         for (int8_t i=0; i<d; i++) { readQ(); } // discard
         for (int8_t i=0; i<n; i++) { s+= readQ(); }
      }
      return(s);
   } // readSumNDQ
   
   void event (void) { ; } // dummy ISR endpoint for compatibility
}; // CAnReadSync

// *REMEMBER* declare handler eg. : ISR (ADC_vect) { gADC.event(); }

class CAnReadSyncDbg : public CAnReadSync
{
public:
   CAnReadSyncDbg (uint8_t profileID) : CAnReadSync(profileID) { ; }
   // using everything...

   void dump (Stream& s) 
   {
      s.println("vmux[]=");
      for (int8_t i=0; i<ANLG_MUX_MAX; i++)
      {
         s.print(i); s.print(": 0x"); s.println(vmux[i], HEX); //s.print
      }
   } // dump
   
}; // CAnReadSyncDbg


// Async (interrupt driven)
#define ANLG_VQ_SH (4)
#define ANLG_VQ_MAX (1<<ANLG_VQ_SH)
#define ANLG_VQ_MSK  (ANLG_VQ_MAX-1)

class CAnalogue : public CAnCommon
{
protected:
   volatile uint16_t v[ANLG_VQ_MAX]; // Latest values
   volatile uint8_t nE; // Event (ISR reading) counter
   uint8_t id, nR;

public:
   CAnalogue (uint8_t profileID=0) : CAnCommon(profileID) { ; }

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void init (uint8_t id=0, uint8_t clkPS=0x7)
   {
      nE= nR= 0;
      ADCSRA= (1<<ADEN) | (1<<ADIE) | (clkPS & 0x07); // ADC enable, Interrupt enable, clock prescaler 128 -> 125kHz sampling clock
      // 1<<ADATE; auto trigger enable
      ADCSRB= 0x00; // Free run (when auto-trigger)
      CAnMux::init(id);
   } // init
   
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

// *REMEMBER* declare handler eg. : ISR (ADC_vect) { gADC.event(); }

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

