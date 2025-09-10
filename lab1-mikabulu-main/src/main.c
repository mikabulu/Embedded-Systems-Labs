#include <MKL28Z7.h>
#include "SysTick.h"
#include "gpio.h"


/* ------------------------------------------
       ECS642/ECS714 Lab1

   Demonstration of simple digital output using KL28Z
   Use RGB LED on Freedom board
   Introduction to cyclic systems
  -------------------------------------------- */

/* --------------------------------------
     Documentation
     =============
     WARNING: SOME PARTS OF THIS CODE ARE NOT EXPLAINED IN FULL IN WEEK 1

     The code has three principal functions
     1. main: this is where the program starts
        DO NOT CHANGE THIS FUNCTION
     2. configure: this setup the peripherals so the LEDs can be used
        DO NOT CHANGE THIS FUNCTION
     3. every10ms: this function runs every 10 ms
        *****EDIT THIS FUNCTION******

     There are also functions setRedLED, setGreenLED, setBlueLED
     Call these but do not change them

     FILE gpio.h
     - - - - - -
     This file contains some (macro) constants that are used here
     You may need to add or change constants
 -------------------------------------- */

/*----------------------------------------------------------------------------
  Turn LEDs on or off
    onOff can be ON or OFF
*----------------------------------------------------------------------------*/
void setRedLED(int onOff) {
  if (onOff == ON) {
    PTE->PCOR = MASK(RED_LED_POS) ;
  }
  if (onOff == OFF) {
    PTE->PSOR =  MASK(RED_LED_POS) ;
  }
  // no change otherwise
}

void setGreenLED(int onOff) {
  if (onOff == ON) {
    PTC->PCOR = MASK(GREEN_LED_POS) ;
  }
  if (onOff == OFF) {
    PTC->PSOR = MASK(GREEN_LED_POS) ;
  }
  // no change otherwise
}

void setBlueLED(int onOff) {
  if (onOff == ON) {
    PTE->PCOR = MASK(BLUE_LED_POS) ;
  }
  if (onOff == OFF) {
    PTE->PSOR = MASK(BLUE_LED_POS) ;
  }
  // no change otherwise
}

/*----------------------------------------------------------------------------
  every10ms - this function runs every 10ms

This function evaluates whether the system should change state (only occassionally)

The system stays in each state for a number of cycles, counted by the 'count'
variable. Each cycle is 10ms long, so 100 cycles gives 100 x 10ms = 1 sec
*----------------------------------------------------------------------------*/
int redstate = REDOFF ;  // this variable holds the current state
int greenstate = GREENOFF; 
int redcount =  REDOFFPERIOD; // this counter variable to decremented to zero
int greencount = GREENOFFPERIOD;

void Task1Red() {
  if (redcount > 0) redcount -- ; // decrement the counter

  switch (redstate) {

    // there is one case for each state
    // each case has the same structure

    case REDOFF:  // the state names are defined in the gpio.h file
      if (redcount == 0) {    // now time to change state
        setRedLED(ON) ;    // set the LEDs for the new state
        redstate = REDON ;    // ... the new state
        redcount = REDONPERIOD ; // reset the counter
      }
      break ;

    case REDON:
      if (redcount == 0) {
        setRedLED(OFF) ;     // set the LEDs for the new state
        redstate = REDOFF ;
        redcount = REDOFFPERIOD ;
      }
      break ;
    
  }
}

void Task2Green() {
  if (greencount > 0) greencount -- ; // decrement the counter

  switch (greenstate) {

    // there is one case for each state
    // each case has the same structure

    case GREENOFF:  // the state names are defined in the gpio.h file
      if (greencount == 0) {    // now time to change state
        setGreenLED(ON) ;    // set the LEDs for the new state
        greenstate = GREENON;    // ... the new state
        greencount = GREENONPERIOD ; // reset the counter
      }
      break ;

    case GREENON:
      if (greencount == 0) {
        setGreenLED(OFF) ;     // set the LEDs for the new state
        greenstate = GREENOFF ;
        greencount = GREENOFFPERIOD ;
      }
      break ;
  }
}

/*----------------------------------------------------------------------------
  Configuration
  The GPIO ports for the LEDs are configured. This is not explained in week 1
*----------------------------------------------------------------------------*/
void configure() {
  // Configuration steps
  //   1. Enable clock to GPIO ports
  //   2. Enable GPIO ports
  //   3. Set GPIO direction to output
  //   4. Ensure LEDs are off

  // Enable clock to ports B and D
  //SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTD_MASK;
  PCC_PORTC |= PCC_CLKCFG_CGC(1) ;
  PCC_PORTE |= PCC_CLKCFG_CGC(1) ;

  // Make 3 pins GPIO
  PORTE->PCR[RED_LED_POS] &= ~PORT_PCR_MUX_MASK;
  PORTE->PCR[RED_LED_POS] |= PORT_PCR_MUX(1);
  PORTC->PCR[GREEN_LED_POS] &= ~PORT_PCR_MUX_MASK;
  PORTC->PCR[GREEN_LED_POS] |= PORT_PCR_MUX(1);
  PORTE->PCR[BLUE_LED_POS] &= ~PORT_PCR_MUX_MASK;
  PORTE->PCR[BLUE_LED_POS] |= PORT_PCR_MUX(1);

  // Set ports to outputs
  PTE->PDDR |= MASK(RED_LED_POS) | MASK(BLUE_LED_POS);
  PTC->PDDR |= MASK(GREEN_LED_POS);

  // Turn off LEDs
  PTE->PSOR = MASK(RED_LED_POS) | MASK(BLUE_LED_POS);
  PTC->PSOR = MASK(GREEN_LED_POS);
  // end of configuration code
}

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
  configure() ;     // configure the GPIO outputs for the LED
  setRedLED(OFF) ;  // ensure all the LEDs are off
  setGreenLED(OFF) ;
  setBlueLED(OFF) ;
  Init_SysTick(1000) ; // initialse SysTick every 1ms
  waitSysTickCounter(10) ;
  while (1) {      // this runs for ever
    //every10ms() ;  // call this every 10ms
    // delay
    Task1Red();
    Task2Green();
    waitSysTickCounter(10) ;  // cycle every 10 ms - not explained in week 1
  }
}
