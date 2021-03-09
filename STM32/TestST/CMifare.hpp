// Duino/STM32/TestST/CMifare.hpp - Mifare RC522 test hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2021

#ifndef CMIFARE_HPP
#define CMIFARE_HPP

#include <SPI.h>
//#include <MFRC522.h>

#ifndef NSS1
#define NSS1 PA4  // STM32 - SPI1 - NSS
#endif

// Register read/write command construction
#define CMD_RD_RN(r) (0x80|((r&0x3F)<<1))
#define CMD_WR_RN(r) (0x00|((r&0x3F)<<1))


/***/

class CMifareSPI
{
public:
   CMifareSPI (void) { ; }

   void init (void)
   {
      pinMode(NSS1, OUTPUT);
      digitalWrite(NSS1, 1);
      SPI.begin(); // SPI1 default
      //SPI.setClockDivider(SPI_CLOCK_DIV8); // 48/8= 6MHz
   }
  
   // void cleanup (void) { SPI.end(); }

   int readN (uint8_t regVal[], const uint8_t regNum[], const int8_t n)
   {
      if (n <= 0) { return(0); }
      transBegin();
      int r= transReadN(regVal, regNum, n);
      transEnd();
      return(r);
   } // readN

  int writeN (const uint8_t regVal[], const uint8_t regNum[], const int8_t n)
  {
      if (n <= 0) { return(0); }
      transBegin();
      int r= transWriteN(regVal, regNum, n);
      transEnd();
      return(r);
  } // writeN

protected:

   void transBegin (void)
   {
      // SPI clock edge behaviour: latch on rising cpol=0, change on trailing cpha=0.
      SPI.beginTransaction(SPISettings(6E6, MSBFIRST, SPI_MODE0));
      digitalWrite(NSS1, 0);
   } //  transBegin

   void transEnd (void)
   {
      digitalWrite(NSS1, 1);
      SPI.endTransaction();
   } // transEnd
  
   int transReadN (uint8_t regVal[], const uint8_t regNum[], const int8_t n)
   {
      SPI.transfer( CMD_RD_RN(regNum[0]) );
      for (int8_t i=1; i<n; i++) { regVal[i-1]= SPI.transfer( CMD_RD_RN(regNum[i]) ); }
      regVal[n-1]= SPI.transfer( 0x00 );
      return(n);
   } // transReadN

   int transWriteN (const uint8_t regVal[], const uint8_t regNum[], const int8_t n)
   {
      for (int8_t i=0; i<n; i++)
      {
         SPI.transfer( CMD_WR_RN(regNum[i]) );
         SPI.transfer( regVal[i] );
      }
      return(n);
   } // transWriteN

}; // CMifareSPI

#define HACK_MAXB 32
class CHackMFRC522 : public CMifareSPI
{
public:

   CHackMFRC522 () { ; }
   
   int8_t identify (Stream& s)
   {
      uint8_t iob= 0x37;
      int8_t r= readN(&iob, &iob, 1);
      if ((r > 0) && (0x90 == (iob & 0xF0)))
      {
         r= iob & 0x0F;
         s.print("Mifare RC522 V");
         s.println(r);
         return(r);
      }
      return(0);
   } // identify
   
   int8_t hack (Stream& s)
   {
      int8_t r=0;
      if (identify(s))
      {
         uint8_t in[HACK_MAXB], out[HACK_MAXB];
         int8_t n=0;
         char st[2]={',','\n'};

         out[n++]= 0x07; // Status1
         out[n++]= 0x08; // Status2
         out[n++]= 0x0C; // Misc ? ctrl
         
         out[n++]= 0x11; // Cmd
         out[n++]= 0x12; // Tx
         out[n++]= 0x13; // Rx

         r= readN(in, out, n);
         for (int8_t i=0; i<r; i++)
         {
            s.print(in[i], HEX);
            s.print( st[ i>=(r-1) ] );
         }
      }
      return(r);
   }

}; // CHackMFRC522

#endif // CMIFARE_HPP
