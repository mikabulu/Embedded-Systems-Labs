/*----------------------------------------------------------------------------
    Shield LEDs KL28Z version
    
    Functions:
      configureLEDs  Configure all the shield LEDs
      ledOnOff       Turn LEDs on or off
 *---------------------------------------------------------------------------*/
 
// LED are active low

// LED	Header/PIN	GPIO
//  1		J2/6	PTD0
//  2		J2/8	PTD2
//  3		J2/10	PTD3
//  4		J1/6	PTB2
//  5		J2/4	PTD7


#include <MKL28Z7.h>
#include <stdbool.h>
#include "..\inc\led.h"
#include "..\inc\mask.h"

/* ----------------------------------------
   Configure GPIO output for shield LEDs 
     1. Enable clock to GPIO ports
     2. Enable GPIO ports
     3. Set GPIO direction to output
     4. Ensure LEDs are off
 * ---------------------------------------- */
void configureLEDs(void) {
 
    // Enable clock to ports B and D
    PCC_PORTB |= PCC_CLKCFG_CGC(1) ;
    PCC_PORTD |= PCC_CLKCFG_CGC(1) ;

    // Make 5 pins GPIO
    PORTD->PCR[LED1pin] &= ~PORT_PCR_MUX_MASK;          
    PORTD->PCR[LED1pin] |= PORT_PCR_MUX(1);          
    PORTD->PCR[LED2pin] &= ~PORT_PCR_MUX_MASK;          
    PORTD->PCR[LED2pin] |= PORT_PCR_MUX(1);          
    PORTD->PCR[LED3pin] &= ~PORT_PCR_MUX_MASK;          
    PORTD->PCR[LED3pin] |= PORT_PCR_MUX(1);          
    
    PORTB->PCR[LED4pin] &= ~PORT_PCR_MUX_MASK;          
    PORTB->PCR[LED4pin] |= PORT_PCR_MUX(1);          
    
    PORTD->PCR[LED5pin] &= ~PORT_PCR_MUX_MASK;          
    PORTD->PCR[LED5pin] |= PORT_PCR_MUX(1);          

    // Set ports to outputs
    PTD->PDDR |= MASK(LED1pin) | MASK(LED2pin) | MASK(LED3pin) | MASK(LED5pin) ;
    PTB->PDDR |= MASK(LED4pin) ;

    // Turn off LEDs
    PTD->PSOR = MASK(LED1pin) | MASK(LED2pin) | MASK(LED3pin) | MASK(LED4pin) | MASK(LED5pin) ;
    PTB->PSOR = MASK(LED4pin) ;
}

/*----------------------------------------------------------------------------
  Function that turns LED on or off
 *----------------------------------------------------------------------------*/
const uint32_t ledMask[] = {MASK(LED1pin), MASK(LED2pin), MASK(LED3pin), MASK(LED4pin), MASK(LED5pin)} ;

void ledOnOff (LED_t l, int onOff) {
    if (l == LED4) {
        if (onOff == LED_ON) {
            PTB->PCOR = MASK(LED4pin) ;  // LEDs are active low
        } else {
            PTB->PSOR = MASK(LED4pin) ;
        }
    } else {
        if (onOff == LED_ON) {
            PTD->PCOR = ledMask[l] ;  // LEDs are active low
        } else {
            PTD->PSOR = ledMask[l] ;
        }
    }
}
