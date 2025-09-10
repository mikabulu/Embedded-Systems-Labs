/* ------------------------------------------
       ECS642/ECS714 Lab4

   The LEDs in two patterns are displayed at different brightness levels using PWM.
   The PIT is used to time the transition between the brightness levels
   A button press (B1) switches between two rates (by changing the PIT load value): 
       * a fast one cycles through all the brightness levels in 2 s
			 * a medium one takes 5 s 
       * a slow one takes 9 s
	 B2 changes the pattern being executed: PatternA or PatternB. 
  -------------------------------------------- */

#include <MKL28Z7.h>
#include <stdbool.h>
#include "../inc/SysTick.h"
#include "../inc/button.h"

#include "../inc/clock.h"
#include "../inc/lpit.h"
#include "../inc/TPMPWM.h"
#include "../inc/triColorLedPWM.h"


/* --------------------------------------
     Documentation
     =============
     This is a cyclic system with a cycle time of 10ms

     The file has a main function, two tasks
       1. pollB1Task: this polls shield button B1
			 2. pollB2Task: this polls shield button B2 
       3. toggleRateTask: this toggles between a fast, medium and slow rate for changing the LED brightness
     and the PIT interrupt service routine which  calls ledPattern to change the brightness of 
     one of the LEDs
		 
		 Behaviour
		 -----------
		 1. Starts with PatternA at the SLOW rate. 
		 2. When button B1 is pressed, changes rate to MEDIUM. If pressed again changes to FAST, third press changes back to SLOW and so on. 
		 3. When button B2 is pressed, pattern executed changes to PatternB. Pressed again will switch the pattern again. 
 -------------------------------------- */
 
/* --------------------------------------------
  Variables for communication
*----------------------------------------------------------------------------*/
bool pressedB1_ev ;  // set by task1 (polling) and cleared by task 2
bool pressedB2_ev;

// Brightness level for each LED used in both patterns
unsigned int redBright; 
unsigned int blueBright; 
unsigned int greenBright; 
/*----------------------------------------------------------------------------
  task1pollB1
  
  This task polls button B1. Keep this task.
*----------------------------------------------------------------------------*/
int b1State ;        // Current state - corresponds to position
int b1BounceCount ;

void initPollB1Task() {
    b1State = BOPEN ;
    pressedB1_ev = false ; 
    b1BounceCount = 0 ;
}

void pollB1Task() {
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
  task1pollB2
  
  This task polls button B2. 
*----------------------------------------------------------------------------*/

int b2State;        
int b2BounceCount;


void initPollB2Task() {
    b2State = BOPEN;
    pressedB2_ev = false; 
    b2BounceCount = 0;
}


void pollB2Task() {
    if (b2BounceCount > 0) b2BounceCount--;
    switch (b2State) {
        case BOPEN:
            if (isPressed(B2)) {
                b2State = BCLOSED;
                pressedB2_ev = true; 
            }
            break;

        case BCLOSED:
            if (!isPressed(B2)) {
                b2State = BBOUNCE;
                b2BounceCount = BOUNCEDELAY;
            }
            break;

        case BBOUNCE:
            if (isPressed(B2)) {
                b2State = BCLOSED;
            }
            else if (b2BounceCount == 0) {
                b2State = BOPEN;
            }
            break;
    }                
}

/*----------------------------------------------------------------------------
   Task: initPatternA()

- initialise states and variabels for PatternA

*----------------------------------------------------------------------------*/


//define states 
#define BlueInc 0
#define RedInc 1
#define GreenInc 2
#define BlueDec 3
#define RedDec 4
#define GreenDec 5

int ledStateA; //set state of LED brightness being increased/decreased


// initial state of task
void initPatternA() {
	redBright = MAXBRIGHTNESS; //red max brightness, start with red being on 
	blueBright = 0; //blue off
	greenBright = 0; //green off
	setLEDBrightness(Red, redBright);
	setLEDBrightness(Blue, blueBright);
	setLEDBrightness(Green, greenBright);
	ledStateA = BlueInc;	//start increasing blue LED brightness
}

/*----------------------------------------------------------------------------
   Task: initPatternB()

- initalise state for PatternB
*----------------------------------------------------------------------------*/

//define states 
#define LedsOff 0
#define RedBlueInc 1
#define BlueDecGreenInc 2
#define RedDecBlueInc 3

int ledStateB; //set state of LED brightness being increased/decreased


// initial state of task
void initPatternB() {
	ledStateB = LedsOff;	//start with all leds off 
}

	/*----------------------------------------------------------------------------
   Task: ledPatterns()

- switches the pattern being executed based on the current state in ledStatePattern. 
- PatternA executes the pattern: RED, MAGENTA, BLUE, CYAN, GREEN, YELLOW and repeat. 
- Pattern B executes the pattern: OFF (all LEDS off), MAGENTA, YELLOW CYAN and repeat. 
*----------------------------------------------------------------------------*/
#define PatternA 0
#define PatternB 1
	
int ledStatePattern = PatternA; //start with PatternA
	
	
void ledPatterns() {
	switch(ledStatePattern) {
		case PatternA: //execute PatternA, starts with red (initPatternA)
			switch(ledStateA) {
				case BlueInc:
					if (blueBright < MAXBRIGHTNESS) { //if blue is not MAX brightness, increase. (MAGENTA)
						blueBright++;
						setLEDBrightness(Blue, blueBright);
					} else {
						ledStateA = RedDec;
					}
					break;
				
				case RedDec:
					if (redBright > 0) { //if red is not off, decrease till off. (BLUE)
						redBright--;
						setLEDBrightness(Red, redBright);
					} else {
						ledStateA = GreenInc;
					}
					break;
				
				case GreenInc:
					if (greenBright < MAXBRIGHTNESS) { //if green is less than MAX brightness, increase (CYAN)
						greenBright++;
						setLEDBrightness(Green, greenBright);
					} else {
						ledStateA = BlueDec;
					}
					break;
				
				case BlueDec:
					if (blueBright > 0) { //if blue is not off, decrease (GREEN)
						blueBright--;
						setLEDBrightness(Blue, blueBright);
					} else {
						ledStateA = RedInc;
					}
					break;
					
				case RedInc:
					if (redBright < MAXBRIGHTNESS) { //if red is less than MAX brightness, increase (YELLOW)
						redBright++;
						setLEDBrightness(Red, redBright);
					} else {
						ledStateA = GreenDec;
					}
					break;
					
				case GreenDec:
					if (greenBright > 0) { //if green is not off, decrease (back to RED)
						greenBright--;
						setLEDBrightness(Green, greenBright);
					} else {
						ledStateA = BlueInc;
					}
					break;
			}
			break;
			
		case PatternB:
			switch(ledStateB) {
				case LedsOff: //decrease brightness of all LEDS, going down the 32 levels 
					if (redBright > 0 || blueBright > 0 || greenBright > 0) { // Check if any of the lights are on 
						if (redBright > 0) {
							redBright--;
							setLEDBrightness(Red, redBright);
						}
						if (blueBright > 0) {
							blueBright--;
							setLEDBrightness(Blue, blueBright);
						}
						if (greenBright > 0) {
							greenBright--;
							setLEDBrightness(Green, greenBright);
						}
					} else {
						ledStateB = RedBlueInc; // Once all off, move to next state 
					}
					break;
					
				case RedBlueInc:
					if (redBright < MAXBRIGHTNESS && blueBright < MAXBRIGHTNESS) { //if red and blue not at MAX, increase (MAGENTA)
						redBright++;
						blueBright++;
						setLEDBrightness(Red, redBright);
						setLEDBrightness(Blue, blueBright);
					}
					if (redBright == MAXBRIGHTNESS && blueBright == MAXBRIGHTNESS) { // Only once both at MAX, move to next state 
						ledStateB = BlueDecGreenInc;
					}
					break;

				case BlueDecGreenInc:
					if (blueBright > 0 && greenBright < MAXBRIGHTNESS) { //if blue is not off and green is less than MAX, dec blue, inc green (YELLOW)
						blueBright--;
						greenBright++;
						setLEDBrightness(Blue, blueBright);
						setLEDBrightness(Green, greenBright);
					}
					if (blueBright == 0 && greenBright == MAXBRIGHTNESS) { // Only once blue is off, green is MAX move to next state 
						ledStateB = RedDecBlueInc;
					}
					break;

				case RedDecBlueInc:
					if (redBright > 0 && blueBright < MAXBRIGHTNESS) { //if red is not off and blue is less than MAX, dec red, inc blue (CYAN)
						redBright--;
						blueBright++;
						setLEDBrightness(Red, redBright);
						setLEDBrightness(Blue, blueBright);
					}
					if (redBright == 0 && blueBright == MAXBRIGHTNESS) { // Only once red is off, blue is MAX, turn LEDs off 
						ledStateB = LedsOff;
					}
					break;
				}
				break;
			}
		}
				
/* -------------------------------------
    Programmable Interrupt Timer (PIT) interrupt handler

      Check each channel to see if caused interrupt
      Write 1 to TIF to reset interrupt flag

    Executes letPatterns() and changes the LED brightness. 
   ------------------------------------- */

// PIT load values
// The larger the count, the lower the frequency of interrupts
// different number of steps for PatternA and B: different load values to ensure both patterns take the same time to complete at each rate
	
// Pattern A: all 32 levels in 9s, 5s or 2s, 6 transitions (32 x 6)
const uint32_t pitSlowCountA = PITCLOCK * 9 / 192 ;  //rate: FAST
const uint32_t pitMediumCountA = PITCLOCK * 5 / 192 ; //rate: MEDIUM
const uint32_t pitFastCountA = PITCLOCK * 2 / 192 ; //rate: SLOW
// Pattern B: all 32 levels in 9s, 5s or 2s, 4 transitions (32 x 4)
const uint32_t pitSlowCountB = PITCLOCK * 9 / 128 ; 
const uint32_t pitMediumCountB = PITCLOCK * 5 / 128 ; 
const uint32_t pitFastCountB = PITCLOCK * 2 / 128;  


void LPIT0_IRQHandler() {
  NVIC_ClearPendingIRQ(LPIT0_IRQn);
	
  // check source of interrupt - LPIT0 channel 0
  if (LPIT0->MSR & LPIT_MSR_TIF0_MASK) {
		ledPatterns();
  }

  // Clear all
  LPIT0->MSR = LPIT_MSR_TIF0(1) | LPIT_MSR_TIF1(1) | LPIT_MSR_TIF2(1)
             | LPIT_MSR_TIF3(1) ; // write to clear
}  

/*----------------------------------------------------------------------------
   Task: toggleRateTask

   Toggle the rate of updates to the LEDs on every signal. 


*----------------------------------------------------------------------------*/

//define the rates
#define FAST 0 
#define SLOW 1
#define MEDIUM 2

int rateState ;  // this variable holds the current state

// initial state of task
void initToggleRateTask() {
    setTimer(0, pitSlowCountA);
    rateState = SLOW ;
}

//changes the rate everytime button B1 is pressed. 
void toggleRateTask() {
    switch (rateState) {
			case SLOW: //9 seconds to complete pattern
            if (pressedB1_ev) {                // signal received
							pressedB1_ev = false ;            // acknowledge
							if (ledStatePattern == PatternA){
                setTimer(0, pitMediumCountA) ;  // update PIT
            }
						else {
							setTimer(0, pitMediumCountB);
						}
						rateState = MEDIUM ;           // ... the new state
					}
					break ;
						
				case MEDIUM:  //5 seconds to complete pattern
            if (pressedB1_ev) {                   // signal received
							pressedB1_ev = false ;            // acknowledge
							if (ledStatePattern == PatternA){
                setTimer(0, pitFastCountA) ;  // update PIT
								}
							else {
								setTimer(0, pitFastCountB);
							}
							rateState = FAST ;           // ... the new state
						}
            break ;
					
				case FAST:  //2 seconds to complete pattern 
            if (pressedB1_ev) {                   // signal received
							pressedB1_ev = false ;            // acknowledge
							if (ledStatePattern == PatternA){
                setTimer(0, pitSlowCountA) ;  // update PIT
								}
							else {
								setTimer(0, pitSlowCountB);
							}
							rateState = SLOW ;           // ... the new state
						}
            break ;    
  }
}

/*----------------------------------------------------------------------------
   Task: switchPattern ()

- switch the pattern being executed when button B2 is pressed. 
- check the current rate to maintain the same rate even when the pattern switches. 

*----------------------------------------------------------------------------*/
void switchPattern(){
    switch(ledStatePattern){
        case PatternA:
            if(pressedB2_ev){
                pressedB2_ev = false;
                ledStatePattern = PatternB; //change pattern to B
                initPatternB(); //initalise states/variables 
                if (rateState == SLOW) { //maintain the rate
                    setTimer(0, pitSlowCountB);
                } 
								else if (rateState == MEDIUM) {
                    setTimer(0, pitMediumCountB);
                } else{
									setTimer(0, pitFastCountB);
								}
            }
            break;
        
        case PatternB:
            if(pressedB2_ev){
                pressedB2_ev = false;
                ledStatePattern = PatternA; //change pattern to A
                initPatternA(); //initalise states/variables 
                if (rateState == SLOW) { //maintain the rate 
                    setTimer(0, pitSlowCountA);
                } 
								else if (rateState == MEDIUM) {
                    setTimer(0, pitMediumCountA);
                } else {
                    setTimer(0, pitFastCountA);
                }
            }
            break;
    }
}

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {
    // enable the peripheralclock
    enablePeripheralClock() ;

    // Configure pin multiplexing
    configureLEDforPWM() ;            // Configure LEDs for PWM control
  
    // Configure button B1
    configureButtons(B1, false) ; // ConfigureButtons B1 for polling
	  configureButtons(B2, false);
  
    // Configure LPIT0 channel 0 to generate interrupts
    configureLPIT_interrupt(0) ;

    // Configure TPM
    configureTPMClock() ;    // clocks to all TPM modules
    configureTPM0forPWM() ;  // configure PWM on TPM0 (all LEDs)
   
    Init_SysTick(1000) ;  // initialse SysTick every 1 ms

    initPollB1Task() ;       // initialise task state
	  initPollB2Task();
    initToggleRateTask() ;   // initialise task state
		initPatternA(); //initalise pattern state 
    // start the PIT
    startTimer(0) ;
    waitSysTickCounter(10) ;  // initialise delay counter
    while (1) {      // this runs forever
        pollB1Task() ;       // Poll button B1
			  pollB2Task();        //Poll button B2
			  switchPattern(); //switch the pattern on every press signal 
        toggleRateTask();    // Toggle LED update rate on every press signal
        // delay
        waitSysTickCounter(10) ;  // cycle every 10 ms 
    }
}
