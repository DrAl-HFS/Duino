// Duino/STM32/BlinkST/morseSymbol.h - International Morse Code bit
// (pulse) patterns for punctuation and other symbols in ASCII.
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
}

#if 0 //def __cplusplus
} // extern "C"
#endif

#endif // MORSE_PUNC_SYM_H