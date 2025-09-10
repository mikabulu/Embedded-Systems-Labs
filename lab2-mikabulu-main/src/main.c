/* ------------------------------------------
       ECS642/ECS714 Lab2 KL28Z version

   The aim of this lab exercise is to understand how to react
   to button presses.
  -------------------------------------------- */
  
#include <stdbool.h>
#include <stdint.h>
#include <MKL28Z7.h>
#include "..\inc\SysTick.h"
#include "..\inc\button.h"
#include "..\inc\led.h"



/* --------------------------------------
     Documentation of the Given Code
     ==============================
1
     Behaviour
     ---------     
     Two buttons on the shield are used:
        * Button B1 is polled. Each press toggles the state to change the direction of the single LEDs turning on.
        * Button B5 has an interrupt handler. Each press toggles the state to inverse the LEDs so all LEDs are lit except the one previously lit.
     
     Files
     -----
     The following file may need to be changed to complete the exercise
     
     main.c  Contains the main loop with 3 tasks and an interrupt handler
     
     The following files should not be changed:
       mask.h   Definition of the MASK macro
       button.h  Definitions and API for shield buttons
       button.c  Code to configure buttons and detect button position
       led.h   Definitions and API for shield LEDs
       led.c   Code to configure and switch shield LEDs
       SysTick.h  Definitions and API for SysTick timer
       SySTick.c  Code for SySTick timer
 -------------------------------------- */

/*----------------------------------------------------------------------------
  Variables for communication
*----------------------------------------------------------------------------*/
bool pressedB1_ev ;  // set by task1 (polling) and cleared by task 2
volatile bool pressedB5_ev ;  // Set by interrupt handler
                              // Cleared by the task3
															
/*----------------------------------------------------------------------------
 Enumeration to define the four states. 
 
 States determine whether to move the lit or unlit LED from either LED1 to LED5
 or LED5 to LED1. 
*----------------------------------------------------------------------------*/
typedef enum{
    MoveLit1to5, 
    MoveLit5to1,
    MoveUnlit1to5, 
    MoveUnlit5to1
} State_t;

/*----------------------------------------------------------------------------
initNextFunctions
 
Initialise the variables used in nextLit and nextUnlit
*----------------------------------------------------------------------------*/
State_t currentState; // LED sequence state 
int ledIndex; // Track the current led position
int counter; // Timing LEDs to be on/off 

void initNextLit() {
	//Initialise variables
	currentState = MoveLit1to5;
	ledIndex = 1; //start LED direction from LED1
	counter = 100;
	
	//Light initial LED
	ledOnOff(LED1, LED_ON);
}
                             
/*----------------------------------------------------------------------------
  task1pollB1
  
  This task polls button B1
*----------------------------------------------------------------------------*/
int b1State ;        // Current state - corresponds to position
int b1BounceCount ;

void initTask1() {
    b1State = BOPEN ;
    pressedB1_ev = false ; 
    b1BounceCount = 0 ;
}

void task1pollB1() {
    if (b1BounceCount > 0) b1BounceCount -- ;
    switch (b1State) {
        case BOPEN:
            if (isPressed(B1)) {
                b1State = BCLOSED ;
                pressedB1_ev = true ; 
            }
          break ;

        case BCLOSED:
            if (!isPressed(B1)) {
                b1State = BBOUNCE ;
                b1BounceCount = BOUNCEDELAY ;
            }
            break ;

        case BBOUNCE:
            if (isPressed(B1)) {
                b1State = BCLOSED ;
            }
            else if (b1BounceCount == 0) {
                b1State = BOPEN ;
            }
            break ;
    }                
}

/*----------------------------------------------------------------------------
 * Interrupt Handler GPIO E
 *    - Clear the pending request
 *    - Test the bit to see if it generated the interrupt 
 *    - Switches states when B5 is pressed 
  ---------------------------------------------------------------------------- */
void PORTE_IRQHandler(void) {  
    NVIC_ClearPendingIRQ(PORTE_IRQn);
    if ((PORTE->ISFR & B5)) {
        pressedB5_ev = true ;
			  if (currentState == MoveLit1to5) {
            currentState = MoveUnlit1to5;
        } else if (currentState == MoveLit5to1) {
            currentState = MoveUnlit5to1;
        } else if (currentState == MoveUnlit1to5) {
            currentState = MoveLit1to5;
        } else if (currentState == MoveUnlit5to1) {
            currentState = MoveLit5to1;
        }    
    }
    // Clear status flags 
    PORTE->ISFR = B5 ; 
}

/*----------------------------------------------------------------------------
  nextLit 

  Function to turn off all LEDs and light the next one based on the state.
*----------------------------------------------------------------------------*/
void nextLit(){
	
	//Decrement the counter
	if (counter>0){
		counter--;
	}
	
	//Only move LED when the counter reaches 0
	if (counter==0){
		// Turn of all LEDs first
    ledOnOff(LED1, LED_OFF);
    ledOnOff(LED2, LED_OFF); 
    ledOnOff(LED3, LED_OFF); 
    ledOnOff(LED4, LED_OFF); 
    ledOnOff(LED5, LED_OFF); 

    switch(currentState){
        case MoveLit1to5:
					ledIndex++; //Move forward
				  if (ledIndex > 5) ledIndex = 1; //Wrap around to LED1 if exceeds 5
				  if (pressedB1_ev){
						pressedB1_ev = false; 
						currentState = MoveLit5to1;    
					}
					break; 
					
				case MoveLit5to1:
					ledIndex--; //Move backward
				  if (ledIndex < 1) ledIndex = 5; //Wrap around to LED5 if decrements past 1
				  if (pressedB1_ev){
						pressedB1_ev = false;
						currentState = MoveLit1to5;
					}
					break;
		}
		
		// Light the corresponding LED based on ledIndex 
    switch (ledIndex){
        case 1: ledOnOff(LED1, LED_ON); break;
        case 2: ledOnOff(LED2, LED_ON); break;
        case 3: ledOnOff(LED3, LED_ON); break;
        case 4: ledOnOff(LED4, LED_ON); break;
        case 5: ledOnOff(LED5, LED_ON); break;
    }
		
		counter = 100; //Reset the counter for the next second 
	
	}
}

/*----------------------------------------------------------------------------
  nextUnlit 

  Function to turn on all the LEDs and turn off the next one based on the state.
*----------------------------------------------------------------------------*/
void nextUnlit(){
	
	//Decrement the counter
	if (counter>0){
		counter--;
	}
	
	//Only move LED when the counter reaches 0
	if (counter==0){
		// Turn on all LEDs first
    ledOnOff(LED1, LED_ON);
    ledOnOff(LED2, LED_ON); 
    ledOnOff(LED3, LED_ON); 
    ledOnOff(LED4, LED_ON); 
    ledOnOff(LED5, LED_ON); 

    switch(currentState){
        case MoveUnlit1to5:
					ledIndex++; //Move forward
				  if (ledIndex > 5) ledIndex = 1; 
				  if (pressedB1_ev){
						pressedB1_ev = false;
						currentState = MoveUnlit5to1;    
					}
					break; 
					
				case MoveUnlit5to1:
					ledIndex--; //Move backward
				  if (ledIndex < 1) ledIndex = 5; //Wrap around to LED5 if decrements past 1
				  if (pressedB1_ev){
						pressedB1_ev = false;
						currentState = MoveUnlit1to5;
					}
					break;
		}
		
		// Light the corresponding LED based on ledIndex
    switch (ledIndex){
        case 1: ledOnOff(LED1, LED_OFF); break;
        case 2: ledOnOff(LED2, LED_OFF); break;
        case 3: ledOnOff(LED3, LED_OFF); break;
        case 4: ledOnOff(LED4, LED_OFF); break;
        case 5: ledOnOff(LED5, LED_OFF); break;
    }
		
		counter = 100; //Reset the counter for the next second 
	
	}
}



/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
    configureLEDs() ;                 // configure LED1-5 using GPIO
    configureButtons(B1 | B5, true) ; // configures buttons B1 and B5 (with an interrupt)
    initTask1() ;  // initialise task1 state
	  initNextLit() ; //initialise nextLit state
    Init_SysTick(1000) ; // initialse SysTick every 1ms
    waitSysTickCounter(10) ;

    while (1) {      // this runs for ever
        task1pollB1() ;
        // Switch function based on current state
        switch(currentState) {
            case MoveLit1to5:
            case MoveLit5to1:
                nextLit();
                break;
            case MoveUnlit1to5:
            case MoveUnlit5to1:
                nextUnlit();
                break;
        }
        // delay
        waitSysTickCounter(10) ;  // cycle every 10 ms - not explained until week 4
    }
}


