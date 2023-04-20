// Duino/UVTest/UvTest.ino - test common analogue UV sensor (GUVA-S12SD photo diode via SGM8521 transimpedance amp)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors April 2023

// Uses a sensor head consisting of the UV module plus two MF58-10K NTC
// thermistors (each standard high side divider with 10k,1% resistor).
// One thermistor is exposed to full light (as photodiode) while all 
// other components are well shielded by plastic coated with durable
// metallic tape.

#define UV_TEST

#include "Common/DN_Util.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Common/AVR/DA_Analogue.hpp"

#define DEBUG Serial
#define PULSE 13  // PB5 on 328P
#define POWER 7   // PD7 on 328P

#define PULSE_TICKS  20

DNTimer pulse(500); // 500ms -> 2Hz
CAnReadSyncDbg gADC(0x7);

ISR (ADC_vect) { gADC.event(); } // needed for quiet mode ADC reading

void bootMsg (Stream& s)
{
#ifdef DA_CONFIG_HPP
   char sid[8];
   if (getID(sid) > 0) { s.print(sid); }
#endif
   s.print(" UVTest " __DATE__ " ");
   s.println(__TIME__);
} // bootMsg

void setup (void)
{
   if (beginSync(DEBUG)) { bootMsg(DEBUG); }
   pinMode(PULSE, OUTPUT);
   pinMode(POWER, OUTPUT);
   digitalWrite(POWER,0);
   for (int8_t i=0; i<4; i++) { analogRead(A0+i); } // ensure appropriate input setting
   for (int8_t i=0; i<2; i++) { pinMode(A4+i,INPUT_PULLUP); }
   set_sleep_mode(SLEEP_MODE_IDLE);
   gADC.init();//gADC.dump(DEBUG);
} // setup

//define ANUSLOGREAD
#define ARSUM_SH 2   // 2~8* sampling, improves accuracy, but still not true Vcc ???
#define ARSUM_N (1<<ARSUM_SH)

uint16_t sum (const uint16_t v[], const int8_t n)
{
   if (n <= 0) return(0);

   uint16_t s= v[0]; 
   for (int8_t i=1; i<n; i++) { s+= v[i]; }
   return(s);
} // sum

int8_t prep (void)
{
   uint16_t r[4], s;
   int8_t n=0;
   gADC.setSleepQ(); // gADC.start();
   //gADC.set(0); // ensure internal thermistor selected
   do
   {  // check for sensible average (100 is pretty cold)
      delayMicroseconds(125); // full read time
      r[0x3 & (n++)]= gADC.readQ();
      s= sum(r,min(n,4));
      if (s >= 400) { return(n); }
   } while (n < 32);
   return(-n);
} // prep

void proc (int16_t v[], const int8_t n)
{
   //gADC.set(0); // ensure BG ref (internal thermistor) channel selected
   gADC.setSleepQ(); // low noise
   for (int8_t i=0; i<n; i++)
   {
      //v[i]= analogRead(A0+i); // NB: no channel map
      v[i]= gADC.readSumNDQ(ARSUM_N);
      gADC.next();
   }
   gADC.stop(); // prevent unnecessary interruption
   gADC.set(0); // maintain BG power ready for next measurement?
} // proc
   
void report (Stream& s, const int16_t v[], const int8_t n)
{
   const uint16_t Vbg1= 1090; // Measured BG as 1089 ~ 1092 mV (nominal 1100mV) but assume this is under true value?
   const uint16_t Vbg1RBbiased= 1165; // Bodged variant giving better Vcc estimation with reduced sampling
   int32_t mV, Vcc=5100, Vcr= 4900; // assume short fat USB, long thin sensor wire
   
   s.print("CHAN/MUX");
   for (int8_t i=0; i<n; )
   {  
      const uint8_t mr= gADC.get(i);
#ifdef VERBOSE
      s.print(" ("); s.print(i); s.print(":0x"); s.print(mr,HEX); s.print(')'); 
#endif
      s.print(" 0x"); s.print(v[i],HEX); // raw sum
      if (v[i] <= 0x2000)
      {
         s.print(":");
         const uint8_t c= (mr & CAnMux::IN_MASK);
         switch(mr & CAnMux::REF_MASK)
         {
            case CAnMux::BGREF1 :
               mV= (v[i] * (uint32_t)Vbg1) / (1000 * ARSUM_N);
               if (0 == c) { s.print(" UV="); }
#ifndef NO_IN_THERM
               else if (CAnMux::IN_THERM == c) { s.print(" T="); }
#endif
               s.print(mV);
               break;
            case CAnMux::VCC :
               if (CAnMux::IN_BGREF1 == c)
               {  // BGREF measured against VCC
                  const uint32_t t1= Vbg1RBbiased * (uint32_t) 1000 * ARSUM_N;
                  Vcc= t1 / v[i];
                  //s.print(" (t1="); s.print(t1); s.print(')'); 
                  s.print(" Vcc="); s.print(Vcc); 
                  break;
               }
               else if (1 == c)
               {  // Remote power (check for voltage drop)
                  Vcr= (v[i] * (uint32_t)Vcc) / (1000 * ARSUM_N);
                  s.print(" Vcr="); s.print(Vcr);
                  break;
               }
               //else...
            default : // assume Vcr
               mV= (v[i] * (uint32_t)Vcr) / (1000 * ARSUM_N);
               s.print(" A"); s.print(c); s.print("="); s.print(mV);
               break;
         } // switch
         s.print(" mV");
      }
      if (++i < n) { s.print(", "); } else { s.println(); }
   }
   //s.println();
} // report

uint16_t c=0;
#define NCHAN 6
void loop (void)
{
   if (pulse.update())
   {
      int16_t v[NCHAN];
      int16_t r;

      digitalWrite(POWER,1); // TODO: determine time for sensor to stabilise

      digitalWrite(PULSE, 1); // LED ON
      c= 0x8000; // zero count

      r= prep(); // warm up
      proc(v,NCHAN);
      digitalWrite(POWER,0);
#if 0
      DEBUG.print("c="); DEBUG.print(c & 0x7FFF); DEBUG.print(" ; ");
      DEBUG.print("prep()->"); DEBUG.print(r); DEBUG.print(" ; ");
#endif
      report(DEBUG,v,NCHAN);
      set_sleep_mode(SLEEP_MODE_IDLE);
      sleep_enable();
   }
   sleep_cpu();
   if ((c & 0x8000) && (pulse.ticksSinceLast() >= PULSE_TICKS))
   {
      c&= 0x7FFF;
      digitalWrite(PULSE, 0);
   }
   c++; // count wake events between updates
} // loop
