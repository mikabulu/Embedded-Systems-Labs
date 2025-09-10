/* ------------------------------------------
       ECS642/ECS714 Lab3 KL28Z version

The aims of this lab exercise are:
  1. To investigate and measure timings:
        - the cycle time of the cyclic system
        - the proportion of the cycle spent execute code / waiting
  2. Implement a reaction timer, timing in milliseconds
  -------------------------------------------- */
  
#include <stdbool.h>
#include <stdint.h>
#include <MKL28Z7.h>

#include "..\inc\SysTick.h"
#include "..\inc\button.h"
#include "..\inc\rgb.h"


/* --------------------------------------
    Documentation of the Given Code
    ==============================

    Behaviour
    ---------     
    There is one task: reactionTimer 
		1. The	red	LED	is	illuminated	to	show	that	the	system	is	ready.
		2. The	user	presses	the	button	when	they	are	ready	to	be	tested.	The	red	LED	goes	off.
		3. The	system	waits	for	a	random	time	between	approximately	1	and	10	seconds	and	
		then	turns	the	green	LED	on.
		4. The	user	must	press	the	button	again	as	soon	after	the	LED	turns	on	as	possible.	
		The	system	times	the	user’s	reaction	time.	The	time	is	written	to	a	variable	and	can	
		be	inspected	using	the	debugger.	The	green	LED	flashes	to	show	that	a	time	has	
		been	recorded.	The	program	can	be	restarted	by	resetting	the	system.
		5. If	the	user	presses	the	button	before	the	green	LED	lights,	the	time	is	not	recorded.	
		Instead,	the	red	LED	flashes	until	the	system	is	reset.

    One button is configured with an interrupt:
        Button B5 - this has hardware debounce and is designed for 
                    use with an interrupt

     Files
     -----
     The following file may need to be changed to complete the exercise
     
     main.c     Contains the main loop with 1 task and an interrupt handler
     SySTick.c  Code for SySTick timer ISR
     SysTick.h  Definitions and API for SysTick timer
     
     The following files should not be changed:
       mask.h     Definition of the MASK macro
       button.h   Definitions and API for shield buttons
       button.c   Code to configure buttons and detect button position
       rgb.h      Definitions and API for tri-colour LEDs on the development board
       rgb.c      Code to configure and operate the tri-colour LEDs on the development board
 -------------------------------------- */

/*----------------------------------------------------------------------------
  Configuration of additional GPIO outputs
       PTE23 - known as OUT1
       PTE30 - known as OUT2
  These outputs are used for measurements
*----------------------------------------------------------------------------*/
#define OUT1 (23)
#define OUT2 (30)

void configureOUT() {
  // Configuration steps
  //   1. Enable clock to GPIO port E
  //   2. Connect GPIO pins to GPIO 
  //   3. Set GPIO direction to output
  //   4. Ensure outputs are off

  // Enable clock to ports E
  PCC_PORTE |= PCC_CLKCFG_CGC(1) ;


  // Make 2 pins GPIO
  PORTE->PCR[OUT1] &= ~PORT_PCR_MUX_MASK;
  PORTE->PCR[OUT1] |= PORT_PCR_MUX(1);
  PORTE->PCR[OUT2] &= ~PORT_PCR_MUX_MASK;
  PORTE->PCR[OUT2] |= PORT_PCR_MUX(1);

  // Set ports to outputs
  PTE->PDDR |= MASK(OUT1) | MASK(OUT2);

  // Turn off outputs
  PTE->PCOR = MASK(OUT1) | MASK(OUT2);
  // end of configuration code
}

/*----------------------------------------------------------------------------
 * nextRand: get next random number 
 *   Based on https://en.wikipedia.org/wiki/Linear_congruential_generator
 * --------------------------------------------------------------------------- */
uint32_t seed = 0x12345678 ;

// Returns a 32 bit number which is too long for us
uint32_t nextRand(void) {
  seed = (1103515245 * seed + 12345) ; 
  return seed ; 
}

// Generate random in range 0 to 900
//    - take top 10 bits - max is 1023
//    - reject if > 900 (about 10% probability)
uint32_t rand900(uint32_t r) {
  uint32_t r900 = (r & 0xFFC00000) >> 22 ; // top 10 bits
  while (r900 > 900) r900 = (nextRand() & 0xFFC00000) >> 22 ;
  return r900 ;
}

/*----------------------------------------------------------------------------
  Variables for communication
*----------------------------------------------------------------------------*/
volatile bool pressedB5_ev ;  // set by interrupt handler
                              //   Cleared by the task
//states for reactionTimer
#define Ready (0)
#define RandomWait (1)
#define Timing (2)
#define SuccessOn (3)
#define SuccessOff (4)
#define ErrorOn (5)
#define ErrorOff (6)
#define ONESEC (100)

int state; //cirrent state in reactionTimer 
uint32_t randomCounter;    // counter for random wait time before green LED lit 
uint32_t capturedTime; // variable for reaction time to be copied to 
int flashCounter; //one second counter for green/red flash time 
/*----------------------------------------------------------------------------
 * Interrupt Handler GPIO E
 *    - Clear the pending request
 *    - Test the bit to see if it generated the interrupt 
  ---------------------------------------------------------------------------- */
void PORTE_IRQHandler(void) {  
    NVIC_ClearPendingIRQ(PORTE_IRQn);
    if ((PORTE->ISFR & B5)) {
        pressedB5_ev = true ;
			  if (state == Timing){
					capturedTime = reactionTimeCounter; // copy the current value in reaction time when B5 pressed 
				}
			}
    // Clear status flags 
    PORTE->ISFR = B5 ; 
	}

/*----------------------------------------------------------------------------
initReactionTimer 

	* initialise variables and state for reactionTimer  
*----------------------------------------------------------------------------*/
void initReactionTimer() {
	state = Ready;     // initialise the state 
  setRGB(RED, RGB_ON) ;    // turn the red LED on
	randomCounter = (nextRand() % 901) + 100; //random wait time between 1 and 10 sec  
	flashCounter = ONESEC; //flash time: LEDs on/off for one second 
}

/*----------------------------------------------------------------------------
reactionTimer

	* Records the time taken in milliseconds for user to press button after light turns green 
*----------------------------------------------------------------------------*/
void reactionTimer(void) {
	if (flashCounter > 0) flashCounter--;
	if (randomCounter > 0) randomCounter--;

	switch (state) {
		case Ready:  
			if (pressedB5_ev) {
				pressedB5_ev = false; // acknowledge button press
				setRGB(RED, RGB_OFF); 
				state = RandomWait; 
			}
			break;

		case RandomWait:
			if (randomCounter == 0) {
				setRGB(GREEN, RGB_ON); // turn green LED on after random wait complete 
				reactionTimeCounter = 0; // set reactionTimeCounter to 0 before incrementing 
				state = Timing;
			} else if (pressedB5_ev) {
				pressedB5_ev = false;
				setRGB(RED, RGB_ON);
				state = ErrorOn; // if button pressed before green LED lit, flash red 
				flashCounter = ONESEC; // light for one second 
			}
			break;

		case Timing:					
			if (pressedB5_ev) {
				pressedB5_ev = false; // acknowledge button press, records reaction time 
				setRGB(GREEN, RGB_OFF);
				state = SuccessOff; // flash green if button pressed after green LED 
			}
			break;

		case SuccessOff: 
			if (flashCounter == 0) {  // turn green LED off on time completed
				setRGB(GREEN, RGB_ON); // turn green LED on for new state 
				state = SuccessOn; 
				flashCounter = ONESEC; 
			}
			break;

		case SuccessOn: 
			if (flashCounter == 0) { // turn green LED on time completed 
				setRGB(GREEN, RGB_OFF); // turn green LED off for new state 
				state = SuccessOff;
				flashCounter = ONESEC;
			}
			break;

		case ErrorOn: 
			if (flashCounter == 0) { // turn red LED on time completed 
				setRGB(RED, RGB_OFF); // turn red LED off for new state 
				state = ErrorOff;
				flashCounter = ONESEC; 		
			}
			break;

		case ErrorOff:
			if (flashCounter == 0) { // turn red LED off time completed 
				setRGB(RED, RGB_ON); // turn red LED on for new state 
				state = ErrorOn;
				flashCounter = ONESEC;
			}
			break;
	}
}

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
    configureRGB() ;                 // configure RGB using GPIO
    configureButtons(B5, true) ;     // configure button B5 (with an interrupt)
    configureOUT() ;                 // configure additional outputs
    Init_SysTick(1000) ;             // initialse SysTick every 1ms
	  initReactionTimer(); //initialise reactionTimer state 
    waitSysTickCounter(10) ;
    while (1) {                   // this runs for ever

        // Set OUT2 at start of task
        PTE->PSOR = MASK(OUT2) ;
		    reactionTimer();
        // Clear OUT2 at end of task
        PTE->PCOR = MASK(OUT2) ;

        // toggle OUT1 every cycle
        PTE->PTOR = MASK(OUT1) ;
        
        // delay
        waitSysTickCounter(10) ;  // cycle every 10 ms
    }
}