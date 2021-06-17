// Duino/Common/N5/N5_RHW.hpp - NRF5x Radio hardware bit twiddling
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors June 2021

#ifndef N5_RHW_HPP
#define N5_RHW_HPP

// Compact recoding of physical packet header & footer structure parameters
class PhyFmtN5
{
protected:
   uint8_t ls1, caews0; // length & s1, crc & addr & flags & S0
   
public:
   /*** DEBUG ***/
   uint32_t reg[3];

   void dumpRaw (Stream& s)
   { 
      s.print("PhyFmtN5: "); s.print(ls1,HEX); s.print(','); s.println(caews0,HEX);
      s.print("reg[]: ");s.print(reg[0],HEX); s.print(','); s.print(reg[1],HEX); s.print(','); s.println(reg[2],HEX);
   }
   /***/
   
   PhyFmtN5 (uint8_t cm=0x7) { if (cm) { capture(cm); } }
   
   void commit (const uint8_t cm=0x1, NRF_RADIO_Type *pR=NRF_RADIO)// const
   {
      uint32_t w;
      
      if (cm & 0x1) // Beware 52833 extensions
      {  // length & aux. fields 
         w= ((ls1 & 0xF) << 16) | (ls1 >> 4); // s1, len
         if (caews0 & 0x01) { w|= 1<<8; } // s0
         reg[0]= w; 
         //pR->PCNF0= w;
      }
      if (cm & 0x2)
      {  // Flags
         w= pR->PCNF1;
         if (caews0 & 0x2) { w|= 1<<25; } else { w&= ~(1<<25); } // whiten
         if (caews0 & 0x4) { w|= 1<<24; } else { w&= ~(1<<24); } // endian
         uint8_t t= caews0 & 0x18; // addr base bytes: 1..3 -> 2..4 (+1 prefix)
         if (0 != t)
         {
            w&= ~(0x7 << 16);
            w|= (t+0x08) << (16-3); // +(1<<3)
         }
         reg[1]= w;
         //pR->PCNF1= w;
      }
      if (cm & 0x4)
      {  // CRC
         w= (caews0 & 0x60) >> 5; // crc length: 0..3
         if (caews0 & 0x80) { w|= 1<<8; } // exclude addr from crc
         reg[2]= w;
         //pR->CRCCNF= w;
      }
   } // commit
   
   /*** DEVEL UTIL ***/
   void capture (const uint8_t cm=0x1, const NRF_RADIO_Type *pR=NRF_RADIO)
   {
      uint32_t w;

      if (cm & 0x1)
      {  // Header 
         w= pR->PCNF0;
         ls1= ((w & 0xF) << 4) | ((w >> 16) & 0xF);
         if (w & (1<<8)) { caews0|= 0x1; } else { caews0&= 0xFE; }
         reg[0]= w;
      }
      if (cm & 0x2)
      {  // Flags
         w= pR->PCNF1;
         if (w & (1<<25)) { caews0|= 0x2; } else { caews0&= ~0x2; } // whiten
         if (w & (1<<24)) { caews0|= 0x4; } else { caews0&= ~0x4; } // endian
         uint8_t t= (w >> (16-3)) - 0x08;  // addr base bytes: 2..4 -> 1..3
         caews0= (caews0 & ~0x18) | t;
         reg[1]= w;
      }
      if (cm & 0x4)
      {  // CRC
         w= pR->CRCCNF;
         caews0= (caews0 & ~0x60) | (w & 0x3) << 5; // crc length: 0..3
         if (w & (1<<8)) { caews0|= 0x80; } else { caews0&= ~0x80; } // exclude addr from crc
         reg[2]= w;
      }
   } // capture
   
}; // PhyFmtN5

// System HF clock management (nothing to do with timing)
class HFClockStateN5
{
public:
   bool notXTal (const NRF_CLOCK_Type *pC=NRF_CLOCK)
   { 
      const uint32_t on= (0x1<<16)|0x1; // state & xtal
      return(on != (on & pC->HFCLKSTAT));
   } // notXTal
   
   // HF clock appears to start in RC mode, whereas XTAL mode is needed for RF
   void startSync (NRF_CLOCK_Type *pC=NRF_CLOCK)
   {  //Serial.print("HFCLKSTAT=0x"); Serial.println(pC->HFCLKSTAT,HEX);
      if (notXTal()) // if (0 == pC->HFCLKRUN)
      {  //Serial.println("start..");
         pC->EVENTS_HFCLKSTARTED= 0;
         pC->TASKS_HFCLKSTART= 1;
         while (0 == pC->EVENTS_HFCLKSTARTED);
      }
   } // startSync
   // void stop (NRF_CLOCK_Type *pC=NRF_CLOCK) { pC->TASKS_HFCLKSTOP= 1; } dangerous ?
}; // ClockStateN5

// Wrapper for hardware state management
class RadioStateN5 : public HFClockStateN5
{
public:
   uint8_t id[16];
   int8_t rssi[16], iR, nR;
   int16_t sumR;
   RadioStateN5 (void) { ; }
   
   bool isOff (NRF_RADIO_Type *pR=NRF_RADIO) { return(RADIO_STATE_STATE_Disabled == pR->STATE); }
   void setup (uint8_t chan=7, NRF_RADIO_Type *pR=NRF_RADIO)
   {
      HFClockStateN5::startSync();
      pR->FREQUENCY= chan;
      pR->MODE= RADIO_MODE_MODE_Nrf_1Mbit;
      pR->TXPOWER= -30; // dBm
      pR->SHORTS|= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
      // 
      pR->CRCCNF=   RADIO_CRCCNF_LEN_Two; // include ADDR in CRC
      pR->CRCINIT=  0xFFFF;
      pR->CRCPOLY=  0x11021; // x^ : [16,12,5,0] same as nRF24 ESB
      // 
      pR->DATAWHITEIV= 0x18;
      //pR->POWER= 1;
   }
   void powerOff (NRF_RADIO_Type *pR=NRF_RADIO) { pR->POWER= 0; }
   
   int8_t recRX (const NRF_RADIO_Type *pR=NRF_RADIO)
   {  int8_t r= -(pR->RSSISAMPLE);
      sumR= sumR + r - rssi[iR];
      rssi[iR]= r;
      id[iR]= pR->RXMATCH;
      iR= (iR + 1) & 0xF;
      nR+= (nR < 0xF);
      return(iR);
   } // recRX
   
   void print (Stream& s, const NRF_RADIO_Type *pR=NRF_RADIO) const
   {
      int16_t mR= sumR;
      if (nR > 1) { mR/= nR; }
      s.print("m_rssi="); s.println(mR);
      /* x= st[]; 
         //if (x & 0x0F) { s.print('R'); }
         //if (x & 0xF0) { s.print('T'); }
         if (0 == (++i & 0xF)) { s.println(); } else { s.print(','); }
      }
      s.println();*/
   }
}; // RadioStateN5

class RadioUtilN5 : public RadioStateN5
{
protected:
   int8_t rssiChan;
   
public: 
   RadioUtilN5 () { ; }
   
   int8_t getRSSI (int8_t chan=-1, NRF_RADIO_Type *pR=NRF_RADIO)
   {  // typically <2us
      int8_t saveChan=-1;
      if ((chan >= 0) && (chan <= 100) && (chan != pR->FREQUENCY))
      {
         saveChan= pR->FREQUENCY;
         pR->FREQUENCY= chan;
      }
      pR->TASKS_RSSISTART= 1;
      while (0 == pR->EVENTS_RSSIEND); // spin
      rssiChan= pR->FREQUENCY;
      if (saveChan >= 0) { pR->FREQUENCY= saveChan; }
      return(-(pR->RSSISAMPLE)); // -> -dB
   } // getRSSI
   
   int8_t dumpRSSI (Stream& s, int8_t chan=-1)
   {
      int8_t r= getRSSI(chan);
      s.print("rssi C"); s.print(rssiChan); s.print("= "); s.println(r);
   }
}; // RadioUtilN5

#endif // N5_RHW_HPP
