#include <16F876.h>
#include <stdio.h>

#fuses HS, NOWDT, NOPROTECT, NOLVP

#use delay(clock = 20000000)

//Declaration i2c bus
#use I2C(MASTER, sda = PIN_C4, scl = PIN_C3, SLOW)

//Declaration rs232
#use rs232(baud = 1200, parity = N, xmit = PIN_C6, FORCE_SW, rcv = PIN_C7, bits = 8, stop = 2)

#BIT TMR1IF = 0x0C .0

#BIT INTE = 0x0B .4

//Declaration of a 41 vaue array for pulse-width modulation
int t[41] = {
  65535,
  64429,
  65452,
  64512,
  65370,
  64594,
  65289,
  64675,
  65209,
  64755,
  65131,
  64833,
  65055,
  64909,
  64982,
  64982,
  64912,
  65052,
  64845,
  65119,
  64783,
  65181,
  64724,
  65240,
  64670,
  65294,
  64621,
  65343,
  64577,
  65387,
  64538,
  65425,
  64505,
  65459,
  64478,
  65486,
  64457,
  65507,
  64441,
  65523,
  64432
};

//Var for BAT conversion
int BAT;

//Interruption count var
long cp = 0;

//Var for array pointer
unsigned char i = 0;

//Sub program for I2C Init
void dec(void) {

  i2c_start();
  //Calendar Address
  i2c_write(0xA0); 
  //Contol Address
  i2c_write(0x08); 
  //Calendar init
  i2c_write(0x00);
  i2c_stop();

}

//Sub routine for calendar restet
void var(void) {

  i2c_start();
  //Calendar Address
  i2c_write(0xA0); 
  //Contol Address
  i2c_write(0x08); 
  //Daily interrupt
  i2c_write(0xCB); 
  i2c_stop();

}

int1 SECT, SORT, LBFL, j = 0;

//Declaration if the bits for battery level ADC
#bit BAT7 = BAT .7
#bit BAT6 = BAT .6
#bit BAT5 = BAT .5
#bit BAT4 = BAT .4
#bit BAT3 = BAT .3
#bit BAT2 = BAT .2
#bit BAT1 = BAT .1
#bit BAT0 = BAT .0

//Setting TIMER1
#int_TIMER1

TIMER1_isr() {
  TMR1IF = 0;

  //When last value of the table is reached
  if ((i == 40) && (j == 1)) {

    //j is reset
    j = 0;

    //And MLI gets flipped
    output_toggle(pin_C1);
  }

  //When we reach the bottom of the table
  if ((i == 1) && (j == 0)) {
    //j is flipped to 1
    j = 1;
  }
  //Waiting for the time specified in t for the pulse wave modulation
  set_timer1(t[i]);
  //MLI gets flipped again
  output_toggle(PIN_C2);

  //When j eq 0
  if ((j == 0)) {
    //Decrementing i
    i--;
  //Sinon
  } else {
    //otherwise i gets incremented
    i++;
  }
}

//Sub routing for interupt
#INT_EXT

EXT_isr() {

  //Incrementing the interrupt counter 
  cp++;
  INTE = 1;

}

//Main subroutine
void main() {}

  //Activating the converter
  setup_adc(ADC_CLOCK_DIV_32);
  //Only A0 is used here
  set_adc_channel(0);
  //TIMER1 is set to 20MHz
  setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
  //Interputs authorized on TIMER1
  enable_interrupts(INT_TIMER1);
  enable_interrupts(GLOBAL);
  //IExternal interputs authorized
  enable_interrupts(PIN_B0);

  //Driver is set to transmitting
  output_high(PIN_C0);

  //Main loop
  while (TRUE) {}

   //Init the i2c buss & reset the calendar
   dec();
   var();

   SORT = input(PIN_A1);
   SECT = input(PIN_A4);
 
   //Transmitting battery values on rs485 bus
   putc(BAT0 + (BAT1 * 2) + (BAT2 * 4) + (BAT3 * 8) + (BAT4 * 16) + (BAT5 * 32) + (BAT6 * 64) + (BAT7 * 128));

   //Lighting the LED LVSE if no power
   output_bit(!SORT, pin_B3);

   //Tempo for signal conversion
   delay_us(10); 
   //Converting the signal for Analog to Digital
   BAT = read_adc(); 

   //If there is no power
   if (SECT == 0) {
      //Setting the battery to discharge
      output_high(PIN_B4); 
      //battery is the main source of power
      output_low(PIN_B5); 
      //Tuning off LED LPRS (L1)
      output_low(PIN_B1); 
      //No longer charging the battery
      output_low(PIN_C5); 

      //If the battery level is low, turining on the LBFL LED
      if ((BAT4 | BAT5 | BAT6 | BAT7) == 0) { 
        output_high(PIN_B2);
      } else {
        //Otherwise, LBFL should be off
        output_low(PIN_B2);
      } 

   } else {
    
      //If outside power is available
      output_low(PIN_B4);
      //External power is used
      output_high(PIN_B5); 
      //Turning on LPRS
      output_high(PIN_B1); 

      //If the battery is not fully carged
      if ((!BAT4 | !BAT5 | !BAT6 | !BAT7) == 1) {
        //Activating the charging of the battery
        output_high(PIN_C5); 
        //And turning on LBFL
        output_high(PIN_B2);
      } 
      else 
      {
        //Otherwise, assuming the battery is charged
        output_high(PIN_C5); 
        //Turning off LBFL
        output_low(PIN_B2);
      } 
   }
   //When 30 days have passed (ie. cp conter = 30), it is necessarry to emtpy the battery & recharge it
   if (cp >= 30) {
   {
      //cp is reset
      cp = 0; 
      //Until the battery is charged
      while ((BAT0 | BAT1 | BAT2 | BAT3 | !SECT) == 0) {
        //Deactivation the charge
        output_low(PIN_C5); 
        //And activating the discharge
        output_high(PIN_B4); 

      }
    }
  }
}
