# Duino
Embedded stuff - builds under Arduino IDE

---
## Miscellaneous Project Design Materials

A dumping ground, perhaps temporary...  

### Thermistor NTC signal conditioning

The charactersitic function of a thermistor is exponential: Rt ~ exp(t)  
//#! <html><\propto></html> ???  

A thermistor in a voltage divider circuit exhibits a linearised response, allowing
a simple model to provide reasonable accuracy over a limited range.
(For the NTC thermistor, linearity appears better at higher temperatures.)
A high side divider inverts the gradient of the NTC response whereas a low side divider
maintains a negative temperature coefficient.

A resistor in parallel with a thermistor also has a linearising effect. Other than
permitting the voltage range versus resolution of the signal to be conditioned, this
offers no advantage over a simple voltage divider.

![Thermistor Conditioning Graph](Doc/ThermNTCResDiv.png)


