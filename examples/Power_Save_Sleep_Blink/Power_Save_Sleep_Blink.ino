/*
 * Power Save Sleep Blink
 * Copyright (c) 2024 David G. Sparks.  All rights reserved.
 * 
 * Demonstrate waking an ATmega328 or ATmega328P
 * from Power Save sleep, at one-second intervals, 
 * by means of a Timer 2 interrupt,
 * clocking Timer 2 asynchronously
 * from a 32,768 Hz watch crystal
 * soldered onto the XTAL pins.
 * 
 * Every effort is made to minimize power consumption.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is provided in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should obtain and review a copy of the GNU Lesser General Public
 * License before using this program. Write to the 
 * Free Software Foundation, Inc.
 * 51 Franklin St, Fifth Floor
 * Boston, MA  02110-1301  USA
 */

void __WDT_off ();
volatile bool tick = false;

 void setup() {

  /* 
   * Set the System Clock Prescaler bits to 0,
   * allowing the device to run at the full 8 MHz speed
   * of the internal calibrated oscillator.
   * Note that it is a two-instruction procedure
   * that must be completed in sequence together.
   */
   CLKPR = (1<<CLKPCE); // set the CLK Prescaler Change Enable bit
   CLKPR = 0;           // clear the CLKPS3:0 bits
  
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

  /* Now modify DDRC setting pin PC5 for output */
  DDRC = (1<<DDC5);
  
  /* prepare a 1-second interrupt from Timer/Counter2 */
  /* Configure Timer/Counter2 for 32KHz operation */
  /* Follow procedure in 18.9 "Asynchronous Operation of TC2" */
  PRR &= ~(1<<PRTIM2);          // enable power to TC2
  TIMSK2 = 0;                   // disable TC2 interrupts  
  ASSR = (1<<AS2);              // switch to 32KHz crystal clock source
  TCNT2 = 0;                    // start count at zero
  TCCR2A = 0;                   // normal mode, no OC output
  /* turn on TC2, normal mode, 32768/128 gives 256 clocks/sec */
  TCCR2B = (0b101 << CS20);  
  // ignore OCR2A and OCR2B
  /* wait now for sync bits to clear */
  while (ASSR & 0b00011111) { ; /* wait */}
  TIFR2 = (0b111<<TOV2);        // clear T2 interrupt flags
  /* wait one second while the crystal gets up to speed */
  while ( ! (TIFR2 & (1<<TOV2)) ) { ; /* wait */ }
  TIFR2 = (0b111<<TOV2);        // clear the flags again  
  TIMSK2 = (1<<TOIE2);          // enable T2 overflow interrupt
  sei();                        // enable interrupts globally
  
  /* set pin 28, PC5, for output */
  DDRC = (1<<DDC5);

}

void loop() {

  if (tick)
  {
    PINC = (1<<PINC5);   // toggle the LED on pin PC5
    tick = false;        // lower the interrupt semaphore 
  }

  /* verify TC2 is ready for sleep, see datasheet section 18.9 */
  OCR2B = 0;  // write an arbitrary value to this unused register
  while (ASSR & OCR2BUB) { ; /* wait for the update bit to clear */ }
  
  /* now it is safe to put CPU to sleep */
  SMCR |= (0b0111<<SE);        // enable sleep in Power-Save mode
  asm volatile ("sleep \n\t");

}

ISR(TIMER2_OVF_vect)
{
  /* disable sleep mode */
  SMCR = 0;
  /*
   * Per the datasheet edition DS40002061B at 
   * 18.9 Asynchronous Operation of Timer/Counter2,
   * pages 161-162.
   * 
   * "When the interrupt condition is met, 
   * the wake up process is started 
   * on the following cycle of the timer clock, 
   * that is, the timer is always advanced by at least one 
   * before the processor can read the counter value." 
   * 
   * "When waking up from Power-save mode, 
   * and the I/O clock (clkI/O) again becomes active, 
   * TCNT2 will read as the previous value 
   * (before entering sleep) until the next rising TOSC1 edge. 
   * The phase of the TOSC clock 
   * after waking up from Power-save mode 
   * is essentially unpredictable, 
   * as it depends on the wake-up time."
   * 
   * "During asynchronous operation, 
   * the synchronization of the Interrupt Flags 
   * for the asynchronous timer takes
   * 3 processor cycles plus one timer cycle."
   * 
   * My solution: allow a couple of timer cycles 
   * to elapse in the ISR, so that 
   * the crystal, the cpu and the interrupt flags 
   * can wake up and synchronize before proceeding.
   */
  while (TCNT2 < 3) { ; } // 3 to be sure; < 2 might also work
   
  /* raise the interrupt semaphore */
  tick = true;
}


/*
 * Assembly code adapted from the example given in the datasheet.
 * It turns off the Watchdog Timer.
 */
void __WDT_off ()
{  asm volatile
  (
    ".equ ASM_SREG, 0x3f \r\n"
    ".equ ASM_MCUSR, 0x34 \r\n"
    ".equ ASM_WDTCSR, 0x60 \r\n"
    // Preserve SREG and r16
    "push r16 \r\n"
    "in r16, ASM_SREG \r\n"
    "push r16 \r\n"
    // Turn off global interrupt briefly
    "cli \r\n"
    // Reset Watchdog Timer
    "wdr \r\n"
    // Clear WDRF in MCUSR
    "in r16, ASM_MCUSR \r\n"
    "andi r16, 0b00000111 \r\n"
    "out ASM_MCUSR, r16 \r\n"
    // Write logical one to WDCE and WDE
    // Keeping old prescaler to prevent unintended time-out
    "lds r16, ASM_WDTCSR \r\n"
    "ori r16, 0b00011000 \r\n"
    "sts ASM_WDTCSR, r16 \r\n"
    // Turn off WDT
    "ldi r16, 0b10000000 \r\n"
    "sts ASM_WDTCSR, r16 \r\n"
     // Restore global interupt
    "sei \r\n"
    // recover SREG and r16
    "pop r16 \r\n"
    "out ASM_SREG, r16 \r\n"
    "pop r16 \r\n"
  );
}
