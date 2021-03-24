// DuinoCommon/Morse/morseSymbol.h - International Morse Code bit
// (pulse) patterns for punctuation and other symbols in ASCII.
// Structured as 3 lookup tables corresponding to ASCII codes - this
// wastes a little storage but is overall an efficient approach.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef MORSE_PUNC_SYM_H
#define MORSE_PUNC_SYM_H

#if 0 //def __cplusplus
extern "C" {
#endif

// 5-7 pulse encoding needed for symbols, (and up to 9 for prosigns)
#define IMC12_N_SHIFT 12
#define IMC12_C_MASK ((1<<IMC5_N_SHIFT)-1)
// declaration macro
#define IMC12(n,c) ( ((n) << IMC12_N_SHIFT) | ((c) & IMC12_C_MASK) )

const uint16_t gSym1IMC12[]= // ASCII characters not mappable: 3/15
{
   IMC12(6, 0b101011), IMC12(6, 0b010010), IMC12(0, 0),      // ! " #
   IMC12(7, 0b0001001), IMC12(0, 0), IMC12(5, 0b0100),       // $ % &
   IMC12(5, 0b01110),  IMC12(5, 0b10110), IMC12(6, 0b101101),// ' ( )
   IMC12(0, 0),  IMC12(5, 0b01010),  IMC12(5, 0b110011),     // * + ,
   IMC12(6, 0b10001), IMC12(6, 0b010101), IMC12(5, 0b10010)  // - . /
};
const uint16_t gSym2IMC12[]=  //    2/7
{
   IMC12(6, 0b111000), IMC12(6, 0b101010), IMC12(0, 0),      // : ; <
   IMC12(5, 0b10001), IMC12(0,0), IMC12(6, 0b001100),        // = > ?
   IMC12(6, 0b011010)                                        // @
};
const uint16_t gSym3IMC12[]=  //    6/9 -> 2/5
{
// IMC12(0, 0), IMC12(0, 0), IMC12(0, 0), IMC12(0, 0),   // [ \ ] ^
   IMC12(6, 0b001101), IMC12(0,0), IMC12(0,0),     // _ ` >
   IMC12(6, 0b001100), IMC12(6, 011010)      // ? @
};
// const uint8_t gSym4IMC5[]= //  { | } ~   4/4

int8_t unpackIMC12 (uint16_t code[], const uint16_t imc12)
{
   code[0]= imc12 & IMC12_C_MASK;
   return(imc12>>IMC12_N_SHIFT);
} // unpackIMC12

// Prosigns
const uint16_t gProsIMC12[]=
{
   IMC12(4,0b0101), IMC12(5,0b01010), IMC12(5,0b01000),     // <AA> <AR> <AS>
   IMC12(5,0b10001), IMC12(5,0b10101), IMC12(8,0b00000000), // <BT> <CT> <HH>
   IMC12(5,0b00101), IMC12(5,0b10101), IMC12(5,0b00010),    // <INT> <KA> <VE>=<SN>
   IMC12(5,0b10110), IMC12(6, 0b100111), IMC12(6,0b000101), // <KN> <NJ> <SK>
   IMC12(9, 0b000111000)    // <SOS>
}; // gProsIMC12

#if 0 //def __cplusplus
} // extern "C"
#endif

#endif // MORSE_PUNC_SYM_H
