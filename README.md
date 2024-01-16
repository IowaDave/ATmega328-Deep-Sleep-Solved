# ATmega328-Power-Save-Sleep-Solved
Adapt Arduino programs to run for weeks or months on batteries.

### Introduction
Power Save sleep mode can reduce the operating current of an ATmega328 or ATmega328P microcontroller (a '328) below 1.5 *micro*Amps, the least level from which the device can still wake itself autonomously with an internal timer interrupt.

At that rate it could easily sleep for a year on a pair of AAA alkaline batteries yet still wake up with power to spare for useful work.

This project solders a 32kHz watch crystal onto the XTAL pins of a '328, to pursue maximum power reduction and longest battery life.

The crystal provides a clock source into Timer/Counter2 during Power Save sleep.  A timer interrupt wakes the controller at precise intervals.

So modified, the '328 needs no other, additional, external Real Time Clock device to wake it up periodically.

#### Disclaimer
Beginners may find this article difficult to follow. It is not a beginner-level project. 

Readers will benefit from prior knowledge and experience with: 

* assembling and soldering electronic components; 
* the ATmega328(P) datasheet, available online from Microchip, its manufacturer, [here](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf); 
* writing code directly addressing named memory locations in the '328; and 
* using the AVRDUDE utility plus an ICSP programmer to upload code into a bare controller.

#### First Things First
I learned a lot doing this project. Before I share it all, let me first list the keys for repeatedly entering Power Save Sleep then waking up by a timer interrupt.

**32K Watch Crystal**

Solder one of these little cylindrical devices onto the TOSC1 and TOSC2 pins (pins 9 and 10 on the DIP28 version of a '328.) The capacitance of the crystal should be 12.5 pf or less. Those of 12.5 pf are fairly easy to procure inexpensively. Their Equivalent Series Resistance should not exceed 30 kOhms. I have not needed to add external capacitors with these 32k crystals. The datasheet indicates that the '328 implements the necessary components internally.

Do solder their tiny legs onto the pins, however. It won't work reliably to poke the tiny leads into a breadboard.

**Fuses**

Accept the default fuse settings as they come in new '328s from Microchip. They are:

| Fuse | Value | Remark |
| ---- | ----- | ------ |
| Extended | ```0xff``` | Disables the Brown-Out Detector |
| High | ```0xd9``` | Allows the Watchdog Timer to be turned off at run-time |
| Low | ```0x62``` | Selects the 8 MHz Internal Calibrated Oscillator as the system clock |

**Clock Prescaler Register, CLKPR**

Set the CLKPS3:0 bits to 0000, to run the system at 8 MHz. Observe the timed sequence spelled out on pages 46-47 of the datasheet.

**Select the crystal source for Timer 2**

Write the AS2 bit in the Asynchronous Status Register, ASSR, to logic level 1. It selects the crystal to be the clock source for Timer/Counter 2. But note the precautions discussed next, below.

**Synchronize Synchronize Synchronize**

When Timer 2 is being clocked by the 32k crystal, it runs independently of the system clock. The two clocks' signals rise and fall at different speeds and at different times. The differences impose necessary delays when writing to or reading from certain registers in the Timer 2 peripheral. 

As I understand it, the delays last until the two clocks "synchronize" for an instant. This occurs upon the first or second subsequent rising edge on the TOSC1 pin attached to the crystal oscillator. Your code must allow for the delays.

Five of the bits in the Asynchronous Status Register, ASSR, give indication when writes to certain registers have not yet synchronized. Certain other delays can be addressed by a counting method. The example program demonstrates both approaches. 

Refer to the section "18.9 Asynchronous Operation of Timer/Counter2" on pages 160-161. Take particular note of the following events:

* Follow procedure 18.9.a for entering asynchronous mode. Writing "1" to the AS2 bit is step 1 in the procedure. Writing to certain other registers in Timer 2 is step 2. Waiting for those writes to be completed is step 3. There are additional steps. Do all of them, to be certain of the values contained in the registers.

* Allow a full second to elapse after entering asynchronous mode, giving the crystal time to stabilize. This pause is needed only once, after powering-up or resetting the '328.

* Just before entering Power Save Sleep, write an arbitrary value to a Timer 2 register that you are not using. I would suggest writing to the Output Compare Register B. Wait for the corresponding synchronization bit to clear in the ASSR register.

* Synchronization is necessary again as the processor wakes up in the Timer 2 interrupt service routine (ISR). I follow this sequence:

    * Turn off sleep mode
    * Wait in the ISR for the Timer 2 counter register to count up to 3. The wait allows the timer to synchronize with the system clock, so that the interrupt flags can resolve properly.

Insufficient attention to any of the synchronization steps can give unexpected results. The timer interrupt might never happen, or might trigger rapidly multiple times, or the device might never wake up from sleep. 

After I feel satisfied that I have described the salient points sufficiently, then I will write down more of what I learned about minimizing the power consumption of '328s. Some of that content already exists in draft form, below.

#### The Goal of This Project
Putting the '328 to sleep is one part of a larger strategy toward minimizing current consumption.

I have used this solution to build very accurate, long-running, battery-operated Real Time Clocks. This project will demonstrate it more simply, by turning an LED on and off periodically at precise intervals.

Power Save sleep mode is documented in the official '328 data sheet. The technical writing was not immediately clear to me. It took a while for me to locate key details, and then translate them into my non-professional vocabulary. One has to learn the tricks.

My goal here is to explain the necessary procedures, as I understand them, with a worked example.

Topics include:

* Mounting the 32 KHz crystal onto the microcontroller
* Operating Timer/Counter2 "asynchronously" using the crystal
* Configuring Timer/Counter2 to interrupt at 1-second intervals
* [Minimizing power consumption by turning off unneeded peripherals](#turn-off-unneeded-peripherals)
* Synchronizing the timer with the system clock, which has to be done twice:
    * first, before entering sleep mode
    * again, in the Interrupt Service Routine (ISR), after waking the controller


### Turn Off Unneeded Peripherals
The biggest reduction in power consumption is achieved by taking the '328 off the Arduino. This step, alone, can cut the battery drain by nearly 90%.

The bare '328 will consume less than 6 mA at worst, compared to the 50 mA draw often seen for an Arduino board. 

Here's the math on that. Firstly, remove the board's energy-draining voltage regulators, USB-to-Serial coprocessor and various LEDs. Away goes 40 mA of unneeded power drain.

Secondly, a '328 running at 16 MHz and 5 volts on an Arduino board consumes nearly 10 mA of current. A bare '328 takes only 6 mA at 5 volts to run its CPU at the top speed of its internal, 8MHz oscillator.

Six milliAmps is merely a good start. We can reduce power usage in the CPU by lowering its frequency:

* 4 MHz takes a bit less than 4 mA.
* 1 MHz takes less than 1 mA.

>Let's put 1 MHz into perspective. <br><br> At that rate the '328 executes up to one million machine instructions per second. It takes only a single instruction to toggle an LED.<br><br>The 32K of program memory in a '328 can contain up to 16,000 or so machine instructions. If program memory were nearly full, the CPU could step through all of it as many as 60 times in a second. <br><br>A '328 running at 1 MHz can still conduct Serial communications at speeds up to 9600 baud. <br><br>Our Apple ][ computers ran at 1 MHz, for cryin' out loud.

It's easy to manage CPU speed. Merely write a selected value to the System Clock Prescaler register, CLKPR. Refer to pages 46-47 in the datasheet. The example selects the 8 MHz system clock speed:

```
  /* 
   * Set the System Clock Prescaler bits to 0,
   * allowing the device to run at the full 8 MHz speed
   * of the internal calibrated oscillator.
   * Note that it is a two-instruction procedure
   * that must be completed in sequence together.
   */
   CLKPR = (1<<CLKPCE); // set the CLK Prescaler Change Enable bit
   CLKPR = 0;           // clear the CLKPS3:0 bits
```

See pages 46-47 in the datasheet version DS40002061B dated 2020. 

Even though we can reduce power consumption to 1 mA just by reducing the frequency. This project does not do that, for a simple reason. 8 MHz is 8 times faster while using only 6 times more power. 

During the brief intervals when the processor is awake, it gets its work done with less total energy cost at 8 MHz. 

The faster we get back to sleep, the more energy we conserve.

Both awake and sleeping, we can achieve further savings by turning off unneeded peripherals inside the '328 itself. The following data are given in the datasheet for an ATmega328 operating a 5 volts.


| Peripheral | Power Drain<br>(microAmps) |
| :---       |                       ---: |
| USART | 100.25 |
| TWI (I2C) | 199.25 |
| Timer/Counter 0 | 61.13 |
| Timer/Counter 1 | 176.25 |
| Timer/Counter 2 | 224.25 |
| SPI | 186.50 |
| ADC | 295.38 |
| Subtotal | 1,243.01 |

In addition to the peripherals listed above, the Analog Comparator, the Internal Bandgap Voltage Reference, the Watchdog Timer and the Brown-Out Detector (BOD) consume energy. For demonstration purposes, these resources also are deactivated. 

Your project might be well advised to retain the Watchdog, the BOD, or both; that discussion is outside the scope of this article.

This project uses only Timer 2. The example program turns off the other peripherals with a few lines of code, listed below.

```
  /* Minimize power consumption by disabling and 
   * turning off power to unused peripherals. 
   */
  ADCSRA = 0;       // Disable the Analog-to_Digital Converter
  PRR = 0b10101111; // Turn off the TWI, TIM0, TIM1, SPI, USART, ADC
  ACSR |= (1<<ACD); // Disable the Analog Comparator
  /* 
   * The Watchdog-Timer-Always-On bit 
   * is disabled by default in the fuses. 
   * This setting allows us to turn off the WDT at run-time.
   */
  __WDT_off();      // Disable the Watchdog Timer

  /*
   * The Internal Bandgap Reference turns off when ADC and AC are off.
   * The Brown Out Detector is turned off by default in the fuses.
   * 
   * Unused I/O pins should be placed in the INPUT_PULLUP state.
   * Input-Pullup minimizes power usage by unattached I/O pins
   * This is done by writing to the Data Direction and the Port registers.
   * It may be redundant, but for a complete example
   * also disable the input buffers of those pins that have
   * analog input capability.
   */
  DIDR1 = 0b11; // disable digital input buffers of AIN1 and AIN0 pins
  DIDR0 = 0b00111111; // disable digital input buffers of the analog pins
  DDRB = 0;   // All Port B pins set to input
  PORTB = 0xff;  // All Port B pins set to pull-up
  DDRC = 0;   // All Port C pins set to input
  PORTC = 0x7f;  // All Port C pins set to pullup
  DDRD = 0;   // All Port D pins set to input
  PORTD = 0xff;  // All Port D pins set to pullup
```














