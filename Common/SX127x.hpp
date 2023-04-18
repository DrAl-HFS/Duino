// Duino/Common/SX127x.hpp - Semtech LORA/(G)FSK/OOK radio transceiver hackery (HOPERF RFM9xW modules).
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Nov 2021

#ifndef SX127x_HPP
#define SX127x_HPP

#ifndef SPI
#include <SPI.h>
#endif
#include "CCommonSPI.hpp"

namespace SX127x
{
   enum Reg : int8_t
   {
      FIFO, OPM,     // read/write buffer, OPerating Mode
      BRH, BRL,      // Bit Rate (16b, unused in LORA mode)
      FDEVH, FDEVL,  // Frequency DEViation setting for FSK
      CFH, CFM, CFL, // Carrier Frequency (24b)
      PAC, PAR,      // Power Amp (output) Config & Ramp
      OCP, LNA,      // Over Current Protection, Low Noise Amp (input)

      // Mode dependant function, non LORA variant:

      RXC, RSSIC,    // Rx. Config, RSSI (Received Signal Strength Indicator) Config
      RSSICOL, RSSITHR, RSSIVAL, // RSSI COLlision, THReshold, VALue
      RXBW, AFCBW,   // Rx and Auto. Freq. Control BandWidth (setting?)
      OOKP, OOKF, OOKA,    // On/Off Keying Peak, Fix (threshold), Average

      // 0x17,18,19 LORA only

      AFCFEIC=0x1A,  // AFC and FEI control
      AFCVH, AFCVL,  // AFC correction value (16b)
      FEIVH, FEIVL,  // Frequency Error Indicator Value (16b)
      PREDC,         // PREample Detector setting
      TO1, TO2, TO3, // TimeOuts
      RXDEL, OSC,    // Rx Delay, (RC) OSCillator settings
      PRELH, PRELL,  // Preample Length (16b)
      SYNC,          // SYNchronisation (word) Control
      SYNV1, SYNV8=0x2F, // Sync. word value (64b)
      PKTC1, PKTC2,  // PacKeT Config
      PLDLEN,        // PayLoaD LENgth (in bytes?)
      NADRR, BADDR,  // Node and Broadcast (group?) addresses
      FIFOTH,        // FIFO THreshold (for Tx start)
      SEQC1, SEQC2,  // SEQuencer (packet state machine) Control
      TIMRESC,       // TIMer RESolution Control
      T1C, T2C,      // Timer 1 & 2 Control
      IMGCAL,        // IMaGe (?) CALibration
      THERM,         // Temperature
      LOBAT,         // LOw BATtery (voltage ) settings
      IRQS1, IRQS2,  // IRQ Status

      // End of main block mode dependant functionality

      DIOM1, DIOM2,  // Digital IO signal pin Mapping
      VERID,         // VERsion ID (silicon revision number - 0x12)

      // NB: Gaps in register block created by undisclosed test registers: do not overwrite!
      HOPC=0x44,     // "fast" (?) frequency HOPping Control (unused for LORA)
      OSCS=0x4B,     // OSCillator Settings
      PADAC=0x4D,    // Power Amp "boost" level?
      CALT=0x5B,     // stored (IQ?) CALibration Temperature
      BRF=0x5D,      // Bit Rate Fraction (division ratio? - unused for LORA)
      AGCR=0x61, AGCT1, AGCT2, AGCT3, // Auto Gain Control Reference and Thresholds
      PLLBWC=0x70    // PLL BandWidth Control
   }; // enum Reg : int8_t

   enum Flag : uint8_t
   {  // OPM
      LORA=0x80,
      MDL_FSK=0x00, MDL_OOK=0x20, MDL_MASK= 0x60, // Modulation values (2b) and mask
      LOFREQ=0x08,
      SLEEP=0x00, STANDBY, FSTX, TX, FSRX, RX, MODE_MASK= 0x07,   // Principal mode (3b)
      // PAC
      BOOST=0x80, MAXPWR_MASK= 0x70, OUTPWR_MASK= 0x0F,
      // PAR
      MDLSF_MASK= 0x70, // Modulation Shaping Filter (3b) mask (values FSK vs. OOK dependant)
      PARFT_MASK= 0x0F, // Power Amp Rise/Fall Time for FSK (4b) 3.4ms .. 10us
      // OCP
      OCP_ON= 0x10,     // Over Current Protection Enable
      TRIM_MASK= 0x0F,  // 45mA..240mA (100mA default)
      // LNA
      GAIN_MAX=0x20, GAIN_MIN= 0xD0, GAIN_MASK= 0xE0,  // Gain (3b) -0dB .. -48dB (values 0x10 and 0xE0 reserved)
      LFBOOST_MASK=  0x18, // 2b dummy (always zero)
      HFBOOST_ON=0x03,     // 2b 0->off, 3->150% LNA boost
      // RXC
      RESTART_COLL= 0x80,     // auto restart on collision
      RESTART_SAMEF= 0x40,    // constant frequency
      RESTART_RELOCK= 0x20,   // new frequency requires PLL re-lock
      AFC_AUTO= 0x10,         // every RX startup
      AGC_AUTO= 0x08,         // LNA gain set automatically
      RX_TRIGGER_MASK= 0x7,   // values .. ?
      // RSSIC
      OFFS_MASK= 0xF8,     // (5b int) +15..-16
      SMOOTH_MASK= 0x07   // (3b) 2..256 samples

   }; // enum Flag : uint8_t

   // Carrier frequency register values for international bands.
   // Unsigned fixed point 24.8 representation is used to provide
   // better accuracy for incremental carrier (channel) changes.
   // NB: 433 / 868 / 915 bands require specific module variants
   // as final RF signal path has to be tuned to band.
   // Caution: transmitting without an appropriate signal path
   // (including antenna) may result in circuit damage.
   // Abuse of enum...
   enum BandU24Q8 : uint32_t { B0= 0,
      // ITU-Region1 "Eurasia" LPD433  (1.74MHz BW) RC, HAM, (69 chan. ea. 25kHz)
      B433_MIN= 0x6C48444C, // 433.05 MHz
      B433_MAX= 0x6CB7A6B3, // 434.79 MHz
      //---
      B_LOFREQ_T= 0x96070500, // 600MHz (?) Hi/lo band threshold for device internal operation
      //---
      // LPD868? EU? 865.0 - 870.0 (5.0MHz) (BW [TxPwr, dwell]) 6 sub-bands:-
      B868_MIN= 0xD84A1EEB, // 865 MHz
      B868_MAX= 0xD98A2DE5, // 870 MHz

      // Sub1 865.0 - 868.0 (3MHz [25mW, 1%]) RFID, LORA ? (.1,3,5,7,9) * 3 = 15 chan.
      B868_S1_MIN= 0xD84A1EEB, // 865.0
      B868_S1_MAX= 0xD90A27E7, // 868.0

      // Sub2 868.0 - 868.6 (600kHz [25mW, 1%])
      B868_S2_MIN= 0xD90A27E7, // 868.0
      B868_S2_MAX= 0xD930901A, // 868.6 (600kHz [25mW, 1%])

      // SigFox (uplink UNB FH BPSK 2k chan.) 868.0 - 868.2  (downlink GFSK 600bd other band?)
      // B868_S3 868.7 - 869.2 : (0.5MHz [25mW, 0.1%]) ? low dwell
      // B868_S4 869.3 - 869.4 : ([10mW, 100%]) ? low power
      // B868_S5 869.4 - 869.65 : ([500mW, 10%]) ? high power (SigFox downlink?)
      // B868_S6 869.7 - 870.0 : (0.3MHz [25mW, 1%]) LORA 869.8 MHz ?
      //---

      // ITU-Region2 "Americas" 902.0 - 928.0 MHz (Mid 915MHz 26.0MHz BW)
      B915_MIN= 0xE18A8E00, // 902.0
      // LORA uplink (125kHz BW) 902.3 - 914.9 @ 0.2 MHz interval (64 channels)
      //  " downlink (500kHz BW) 923.3 - 927.5 @ 0.6 MHz interval (8 channels)
      B915_MAX= 0xE80ADC00, // 928.0 MHz
/*
   }; // enum BandU24Q8 : uint32_t

   enum CarrierU24Q8 : uint32_t
   {
*/
      C_STEP_LORA= 0xCCD66, // 0xCCD.663 = 200 kHz BW (+- 100kHz)

      B868_S1_LCMIN= 0xD850859E, // 865.1MHz  LC1
      B868_S1_LCMAX= 0xD903C134, // 867.9 MHz LC15

      B868_S2_LCMIN= 0xD9108E9A, // 868.1 MHz *SigFox overlap* _S2_LC1
      //B868_S2_LC2= 0xD91D5C00, // 868.3 MHz _S2_LC2
      B868_S2_LCMAX= 0xD92A2967, // 868.5 MHz _S2_LC3

      // S3..S5 not suitable for LORA (power / dwell time)

      B868_S6_LCMIN= 0xD97D607F, // 869.8 _S6_C1
      B868_S6_LCMAX= 0xD983C732, // 869.9 *C1 overlap*

   }; // enum CarrierU24Q8 : uint32_t

   typedef BandU24Q8 CarrierU24Q8;

   struct ShadowReg { uint8_t opm; };
}; // namespace SX127x

// Layering classes by inheritance inncreases the potential for code reuse
// or adaptation without introducing runtime overhead and with negligible
// increase in complexity.

class CSX127xSPI : public CCommonSPI
{
public:
   CSX127xSPI (void) { ; }

   void init (void)
   {
      spiSet= SPISettings(1000000, MSBFIRST, SPI_MODE0);
      CCommonSPI::begin();
   } // init

   // void release (void) { CCommonSPI::end(); } ???
   uint8_t readReg (const SX127x::Reg r)
   {
      start();
      HSPI.transfer(r); // & 0x7F
      uint8_t v= HSPI.transfer(0x00);
      complete();
      return(v);
   } // readReg

   uint8_t writeReg (const SX127x::Reg r, uint8_t v)
   {
      start();
      HSPI.transfer(0x80|r);
      v= HSPI.transfer(v);
      complete();
      return(v); // previous value ?
   } // writeReg

   int readReg (const SX127x::Reg r, uint8_t v[], int n) // &0x7F
   {
      if (n < 1) { return(0); }
      start();
      HSPI.transfer(r); // & 0x7F
      n= read(v,n);
      complete();
      return(n);
   } // readReg

   int writeReg (const SX127x::Reg r, const uint8_t v[], int n) // &0x7F
   {
      if (n < 1) { return(0); }
      start();
      HSPI.transfer(0x80|r);
      n= write(v,n);
      complete();
      return(n);
   } // writeReg

}; // class CSX127xSPI
/*
https://www.disk91.com/2017/technology/sigfox/all-what-you-need-to-know-about-regulation-on-rf-868mhz-for-lpwan/

"The first channel 865-688 is a 25mW / 1% channel, it is a large area to add LoRa channels but it is also a zone used by RFIDs.

The 868,7 to 869,2 sub-band is a 25mW area but the duty cycle is 0.1%, this zone can be interesting to communicate when an object is emitting once a day : the risk of collision is really lower and the number of time you will have to re-emit is, as a consequence, lower, so in this sub band you can expect to preserve your energy.

The 869,3 to 869,4 sub-band is not usable for LPWAN as the maximum power is 10mW but you have no duty-cycle so it can be a good area for local object communication.

The 869,4 to 869,65 zone is particularly interesting because you can communicate with 500mW with a 10% duty cycle. An object would ne be able to use a such power when running on battery but in a central network point of view it is a really good channel for downlink communications. The gateway can communicate far away and be listen over the local noise of the object ; the larger duty cycle allows the gateway to communicate with many objects.

The last zone 869,7 to 870 is the last 25mW / 1% zone where you can deploy extra LoRaWan channels."

*/

class CSX127xRaw : public CSX127xSPI, SX127x::ShadowReg
{
protected:
   SX127x::CarrierU24Q8 carrier; // ??? waste of time ???

   uint8_t syncReg (const SX127x::Reg r, const uint8_t mask, int maxIter=10)
   {
      uint8_t v;
      do
      {
         v= readReg(r);
         v&= mask;
      } while ((0 == v) && (--maxIter > 0));
      return(v);
   } // syncReg

public:
   CSX127xRaw (SX127x::CarrierU24Q8 c=SX127x::B868_S1_LCMIN) : carrier{c} { ; }

   // TODO: -> *Util
   bool identify (void)
   {
      uint8_t id=-1;
      readReg(SX127x::VERID, &id, 1);
      return(0x12 == id);
   }
   void setup (void)
   {
      uint8_t rv[4]; // register values

      // re-lock PLL, AFC/AGC off
      writeReg(SX127x::RXC, 0x20);

      // fast hop on
      writeReg(SX127x::HOPC, 0x80);

      uint32_t c= carrier;
      for (int i=2; i>=0; i--) { c>>= 8; rv[i]= c; }
      writeReg(SX127x::CFH, rv, 3);

      // tx amp
      rv[0]= 0x00; // PAC: min power
      rv[1]= 0x17; // PAR: GFSK1, 62us ramp (16.1k/s)
      rv[2]= 0x20; // OCP: limit to min (45mA)
      // rx amp
      rv[3]= 0xC0; // LNA: minimum gain (close range testing)
      writeReg(SX127x::PAC, rv, 4);

      // modulation
      opm= SX127x::MDL_FSK | SX127x::FSRX; // start PLL lock
      if (carrier <= SX127x::B_LOFREQ_T) { opm= SX127x::LOFREQ; }
      writeReg(SX127x::OPM, opm);
   }


}; // class CSX127xRaw

class CSX127xHelper : public CSX127xRaw
{
protected:
   uint8_t rssiMM[2];

public:

   CSX127xHelper (void) { resetMM(); }

   void resetMM (void) { rssiMM[0]= 0xFF; rssiMM[1]= 0x00; }

   void logRSSI (void)
   {
/*    switch (opm & MODE_MASK))
      {
         case SX127x::FSRX :
            ready= syncReg(SX127x::IRQS1, 0x10); // PLL lock
            break;
         case SX127x::RX :
            ready= syncReg(SX127x::IRQS1, 0x10); // PLL lock
            break;
      }*/
      if (syncReg(SX127x::IRQS1, 0x80)) // mode ready
      {
         uint8_t v= readReg(SX127x::RSSIVAL);
         rssiMM[0]= min(rssiMM[0], v);
         rssiMM[1]= max(rssiMM[1], v);
      }
   }
}; // class CSX127xHelper

// ??? -> ???
uint32_t rdbitsMSB (const uint8_t vB[], const int8_t n)
{
   if (n < 1) { return(0); }
   const int8_t nE= (n & 0x7);
   int8_t nB= (n / 8) + (0 != nE);
   uint32_t v= vB[0];
   for (int8_t i=1; i<nB; i++) { v= (v<<8) | vB[i]; }
   if (0 != nE) { v&= BIT_MASK(n); }
   return(v);
} // rdbitsMSB

class CSX127xDebug : public CSX127xHelper
{
public:
   //const float fStep= 61.024; // (Hz)
   const float scaleMHztoRV= 16386.995; // = 10E6 / 61.024
   const float scaleRVtoMHz= 61.024E-6; // = 61.024 / 10E6


   CSX127xDebug (void) { ; }

   float freqMHztoRV (uint32_t fMHz) { return(fMHz * scaleMHztoRV); }
   float freqRVtoMHz (uint32_t fRV) { return(fRV * scaleRVtoMHz); }
   float freqRVtokHz (uint32_t fRV) { return(1000 * freqRVtoMHz(fRV)); }

   void dump1 (Stream& s)
   {
      int32_t v;
      uint8_t reg[18]; // First dummy byte for FIFO

      readReg(SX127x::OPM, reg+SX127x::OPM, sizeof(reg)-1);

      v= rdbitsMSB(reg+SX127x::BRH, 16);
      s.print("\nbr="); s.print(v); s.print("bit/s");

      v= rdbitsMSB(reg+SX127x::FDEVH, 14);
      s.print(" | fd="); s.print(freqRVtokHz(v)); s.print("kHz");

      v= rdbitsMSB(reg+SX127x::CFH, 24);
      s.print(" | fc=");
      //s.print(freqRVtokHz(rv)); s.print("kHz, ");
      s.print(freqRVtoMHz(v)); s.print("MHz");

      s.print(" | PAC="); s.print(reg[SX127x::PAC], HEX);
      s.print(" | PAR="); s.print(reg[SX127x::PAR], HEX);
      s.print(" | OCP="); s.print(reg[SX127x::OCP], HEX);

      s.print(" | LNA=");
      v= reg[SX127x::LNA] & SX127x::GAIN_MASK;
      if ((v < SX127x::GAIN_MAX) || (v > SX127x::GAIN_MIN)) { s.print("0x"); s.print(v,HEX); s.print("?"); }
      else
      {
         switch(v)
         {
            case SX127x::GAIN_MAX :   v= 0; break;
            case SX127x::GAIN_MAX+1 : v= -6; break;
            default : v= ((v >> 5) - 2) * -12; break;
         }
         s.print(v); s.print("dB");
      }

      float f= -0.5 * reg[SX127x::RSSIVAL];
      s.print(" | RSSI="); s.println(f);
      s.print(" min,max="); s.print(-0.5 * rssiMM[0]);
      s.print(","); s.print(-0.5 * rssiMM[1]);
      s.println();

      readReg(SX127x::RXBW, reg, 16);
      dumpHexTab(s, reg, 16);

      s.println("\n---\n");
   } // dump1

   bool identify (Stream& s)
   {
      bool r= CSX127xRaw::identify();
      if (r) { s.println("sx127x/RFM9x identified"); }
      return(r);
   }
}; // class CSX127xDebug

#endif // SX127x_HPP
