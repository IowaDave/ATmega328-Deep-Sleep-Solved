# ATmega328-Power-Save-Sleep-Solved
A 32 kHz watch crystal can drive an internal alarm clock to wake up the microcontroller behind the popular Arduino Uno from its most power-thrifty mode of deep sleep.

The savings are significant. In deep sleep, with everything turned off except the crystal, the chip consumes less than 1.5 *micro*Amps of current. That is about 1/4,000th of the current during active operation. It is about 1/40,000th of the current drain of an Arduino Uno board.

What does it matter? Because it can enable Arduino-like projects to run continuously for weeks or months on a single set of rechargeable batteries.

I found everything to know about this feature described in the official data sheet. However, the wording was not immediately clear to me.

This repository will contain my translation of the data sheet into non-expert language. My goal is to explain the necessary procedures understandably to a non-professional person, with a worked example.