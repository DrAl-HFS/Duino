// Duino/Test/DA_RF24.hpp - Arduino-AVR NRF24L01+ wrapper (using RadioHead library)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Jan 2021

#ifndef DA_RF24_HPP
#define DA_RF24_HPP

// Include RadioHead NRF24L01+ "driver" - functional 
// albeit not terribly efficient.
#include <RH_NRF24.h>


/***/
#define PIN_CE 8 // RF Rx/Tx activate pin for Uno / nano
// Other pins SCK,MOSI,MISO,SS all default

// NRF24L01+ ESB packet format (endianess unspecified!) : 
// [preamble: 1byte][addr: 3~5 bytes][hdr: 9bits [len:6][seq:2][nack:1]][payload: 0~32bytes][crc: 1~2bytes]
class TestRF24 : protected RH_NRF24
{
protected:
   uint32_t nPZ;
   uint16_t inSeqM, nRxB, nTxB;
   int8_t outSeq;
   uint8_t nRx, nTx;
   char out[16], in[16];
   
public:
   TestRF24 (uint8_t pinCE=PIN_CE) : RH_NRF24(pinCE,SS) {;}
   
   uint8_t init (Stream& log, const char idsz[])
   {
      //RHMode x= RHModeInitialising;
      // uBit addr 0x75626974 {0x5A,00,00};
      uint8_t addr[]={0x7E,0x18,0x18,0x7E}; // fully (bit&byte) palindromic test address 
      uint8_t rm= 0x0;
      int8_t v=0x38, i= 0;
      if (RH_NRF24::init())
      {
         rm= 0x1;
         if (setNetworkAddress(addr,sizeof(addr))) { rm|= 0x2; }
      }
      if (idsz)
      {
         do { out[i]= idsz[i]; } while ((0 != out[i]) && (++i < sizeof(out)));
      } // else { out[0]= 0; }
      while (++i < sizeof(out))
      {  // pad with "noise"
         out[i]= out[i-1] ^ v;
         v^= (v << 1);
      }
      // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
      rm|= (setChannel(45) > 0) << 1;
      //rm|= (setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm) > 0) << 2;
      if (log.available() >= 0) // junk
      {
         if (idsz) { log.print(idsz); }
         log.print(":initRF() - ");
         log.println(rm,HEX);
         if (rm > 0)
         {
            log.print("Addr");
            for (int i=0; i<4; i++) { log.print(':'); log.print(addr[i],HEX);  }
            log.println();
         }
      }
      return(rm);
   } // init
   
   uint8_t proc (uint8_t event)
   {
      uint8_t n, r=0;
      if ((event & 0x80) && (outSeq <= 0)) { outSeq+= event & 0xF; }
      if (outSeq > 0)
      {
         n= sizeof(out);
         out[n-1]= outSeq;
         if (send(out, n) && waitPacketSent())
         {
            nTxB+= n;
            r|= 0x1;
            nTx++;
            --outSeq;
         }
         setModeRx(); // ???
      }
      n= sizeof(in);
      if (recv(in, &n))
      {
         if (n > 0)
         {
            uint8_t nS= in[n-1];
            if (nS <= 0xF) { inSeqM|= 1 << (nS-1); }
         }
         nRxB+= n;
         r|= 0x10;
         nRx++;
      }
      nPZ+= (0 == r);
      return(r);
   } // proc
   
   void report (Stream& log)
   {
      log.print(in);
      log.print("<-RF["); 
      log.print(nRx); 
      log.print("] "); 
      log.print(nRxB); 
      log.print(" M=x"); 
      log.print(inSeqM,HEX);
      log.print(" ->RF["); 
      log.print(nTx); 
      log.print("] "); 
      log.print(nTxB); 
      log.print(" nPZ="); 
      log.println(nPZ); 
      clear();
   }
   
   void clear (void) { nPZ= 0; in[0]= 0; outSeq= 0; inSeqM= 0; nRx= 0; nTx= 0; nRxB= 0; nTxB= 0;  }
   
}; // class TestRF24


#endif // DA_RF24_HPP
