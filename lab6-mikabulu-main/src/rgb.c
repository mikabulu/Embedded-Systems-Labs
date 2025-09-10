/*----------------------------------------------------------------------------
    Freedom Board RGB LED
 *---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <MKL28Z7.h>
#include "..\inc\rgb.h"
#include "..\inc\mask.h"



/*----------------------------------------------------------------------------
  Configuration
  The GPIO ports for the LEDs are configured. 
*----------------------------------------------------------------------------*/
void configureRGB() {
  // Configuration steps
  //   1. Enable clock to GPIO ports
  //   2. Enable GPIO ports
  //   3. Set GPIO direction to output
  //   4. Ensure LEDs are off

  // Enable clock to ports C and E
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
  Turn LEDs on or off
    onOff can be ON or OFF
*----------------------------------------------------------------------------*/

void setRGB(RGB_t rgb, int onOff) {
    switch (rgb) {
        case RED:
            if (onOff == RGB_ON) {
                PTE->PCOR = MASK(RED_LED_POS) ; 
            } else {
                PTE->PSOR =  MASK(RED_LED_POS) ;
            }
            break ;
        case GREEN:
            if (onOff == RGB_ON) {
                PTC->PCOR = MASK(GREEN_LED_POS) ; 
            } else  {
                PTC->PSOR = MASK(GREEN_LED_POS) ; 
            }
            break ;
        case BLUE:
            if (onOff == RGB_ON) {
                PTE->PCOR = MASK(BLUE_LED_POS) ;
            } else {
                PTE->PSOR = MASK(BLUE_LED_POS) ;
            }
            break ;
    }
}