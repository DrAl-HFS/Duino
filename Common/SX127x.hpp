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
      FDEVH, FDEVL,  // Frequency DEViation setting (channel? 16b unused in LORA mode)
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

}; // namespace SX127x

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

   int readReg (const SX127x::Reg r, uint8_t v[], int n) // &0x7F
   {
      if (n < 1) { return(0); }
      start();
      HSPI.transfer(r);
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

class CSX127xRaw : public CSX127xSPI
{
public:
   CSX127xRaw (void) { ; }

   // TODO: -> *Util
   bool identify (void)
   {
      uint8_t id=-1;
      readReg(SX127x::VERID, &id, 1);
      return(0x12 == id);
   }

}; // class CSX127xRaw

class CSX127xHelper : public CSX127xRaw
{
public:
   CSX127xHelper (void) { ; }

   // 0xD90A28 * 61.024Hz -> 868MHz
   // 10E6 / 61.024 = 16386.995
   uint32_t freqMHztoRV (uint32_t fMHz) { return(fMHz * 16387); } // ((fMHz << 16) + fMHz)
   uint32_t freqRVtoMHz (uint32_t fRV) { return(fRV / 16387); }

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
   CSX127xDebug (void) { ; }

   void dump1 (Stream& s)
   {
      uint8_t reg[16];
      readReg(SX127x::OPM,reg,16);
      dumpHexTab(s,reg,16);
      s.print("br="); s.print(rdbitsMSB(reg+1,16));
      s.print(" | fd="); s.print(rdbitsMSB(reg+3,14));
      s.print(" | fc="); s.print(freqRVtoMHz(rdbitsMSB(reg+5,24)));
      s.println();
   }

   bool identify (Stream& s)
   {
      bool r= CSX127xRaw::identify();
      if (r) { s.println("sx127x/RFM9x identified"); }
      return(r);
   }
}; // class CSX127xDebug

#endif // SX127x_HPP
