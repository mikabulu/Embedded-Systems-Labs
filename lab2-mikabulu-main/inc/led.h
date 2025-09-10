/*----------------------------------------------------------------------------
    Header file for shield LEDs, KL28Z version
    
    Functions:
      configureLEDs  Configure all the shield LEDs
      ledOnOff       Turn LEDs on or off
 *---------------------------------------------------------------------------*/

// LED	Header/PIN	GPIO
//  1		J2/6	PTD0
//  2		J2/8	PTD2
//  3		J2/10	PTD3
//  4		J1/6	PTB2 <--- port B
//  5		J2/4	PTD7

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

// ===============
// LEDs 1 - 5
// ===============

// There are 5 LEDs, wired active low
typedef enum {LED1, LED2, LED3, LED4, LED5} LED_t ;

#define LED1pin (0)    // on port D 
#define LED2pin (2)    // on port D 
#define LED3pin (3)    // on port D
#define LED4pin (2)    // on port BBBBB
#define LED5pin (7)    // on port D

// LED states
#define LED_ON (1)
#define LED_OFF (0)

// API
void configureLEDs(void) ;    // configure all the LEDs using GPIO
void ledOnOff (LED_t l, int onOff) ;  // turn an LED on or off

#endif