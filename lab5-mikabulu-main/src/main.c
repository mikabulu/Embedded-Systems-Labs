/* ------------------------------------------
       ECS642/ECS714 Lab 5 Given Code
       KL28Z Version
    
   Press B1 to use ADC to measure voltage
   When green LED is on, min of VR1 and VR2 is measured
   When red LED is on, max of VR1 and VR2 is measured
   Both measurements update 'scaled' variable    
  -------------------------------------------- */

#include <stdbool.h>
#include <stdint.h>
#include <MKL28Z7.h>
#include "..\inc\SysTick.h"
#include "..\inc\button.h"
#include "..\inc\rgb.h"
#include "..\inc\led.h"
#include "..\inc\adc.h"

/* --------------------------------------
     Documentation
     =============
     This is a cyclic system with a cycle time of 10ms

     The file has a main function, with two tasks
        1. task1PollB1 polls button B1
        2. task2MeasureVR uses the ADC to continuously measure the voltage of VR1 and VR2
				3. task3FlashLEDs flashes the shield LEDs where the on and off time are determined by the recorded voltages 
 -------------------------------------- */
 
/* --------------------------------------------
  Variables for communication
*----------------------------------------------------------------------------*/
bool pressedB1_ev ;  // set by task1 (polling) and cleared by task 2

/*----------------------------------------------------------------------------
  task1pollB1
  
  This task polls button B1. Keep this task.
*----------------------------------------------------------------------------*/
int b1State ;        // Current state - corresponds to position
int b1BounceCount ;

void initTask1PollB1() {
    b1State = BOPEN ;
    pressedB1_ev = false ; 
    b1BounceCount = 0 ;
}

void task1PollB1() {
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
   Task: task2MeasureVR

   This task uses the ADC to get the voltage. Measurement frequency of 50Hz (every 20ms). 
   Measures both voltages every other cycle. 
*----------------------------------------------------------------------------*/
VR_t vrState ;        // Current state - which VR measured next

volatile uint16_t vr1 ;      // raw value - single ended, ON TIME 
volatile uint16_t vr2 ;      // raw value - single ended, OFF TIME
int scaledVR1 ; // voltage VR1 as a scaled integer - 3v shown as 300
int scaledVR2; // voltage VR2 as scaled 
volatile uint16_t counter; //counter for voltage measurement frequency 

void initTask2MeasureVR() {
	  counter = 2; //20ms 
    vrState = VR1 ; //current state 
}

void task2MeasureVR() {
	if (counter > 0) counter--;
	if (counter ==0){ //measure VR1 and VR2 every other cycle 
    vr1 = MeasureVR(VR1); 
		scaledVR1 = (vr1 * 330) / 0xFFFF;
    vr2 = MeasureVR(VR2);
		scaledVR2 = (vr2 * 330) / 0xFFFF;		
		counter = 2; //reset counter 
	}		
}

/*----------------------------------------------------------------------------
   Task: task3FlashLEDs

 After the system starts up the three colour LED shows green.
- When the button B1 is pressed the measured voltages on VR1 and VR2 are used as
the minimum.
- The three colour LED then shows red.
- When the button is pressed again the measured voltage is used as the maximum.
- Providing both the maximum values exceed their corresponding minimum values,
  the system proceeds to flash the all 5 shield LEDs and the RGB LED shows green. 
- on time controlled by VR1, off time by VR2. (max on/off time 4s, min 0.5s)
- If the maximum does not exceed the minimum, then the system entered an error
state: the three colour LED flashes blue on / off with 1 sec period and equal on and off times (i.e. 0.5s on).
- if B1 is pressed after callibration, system stops and red LED lights
*----------------------------------------------------------------------------*/
//define states
#define ShieldLEDsOn 0
#define ShieldLEDsOff 1
#define FlashOff 2
#define MeasureMin 3
#define MeasureMax 4
#define ErrOn 5
#define ErrOff 6

int shieldState; //current state of the system 
volatile uint16_t shieldCounter; //counter for shield LEDs
uint16_t VR1max, VR1min;  // For VR1's range
uint16_t VR2max, VR2min;  // For VR2's range
volatile uint16_t errorCounter; //counter for shield LEDs
//variables to ensure the min voltages are respected even after callibration 
uint16_t v1; 
uint16_t v2;

//initialise variables, states and LEDs 
void initTask3FlashLEDs(){
	errorCounter = 50; //0.5 seconds
	shieldCounter = 0; //set to 0 
	setRGB(RED, RGB_OFF) ;
	setRGB(BLUE, RGB_OFF) ;
  setRGB(GREEN, RGB_ON) ; //only GREEN on at start 
	shieldState = MeasureMin;
}

void task3FlashLEDs() {
		shieldCounter++;  // increase counter 
		switch (shieldState){
			case MeasureMin: //record the minimum voltage on button B1 press 
				if(pressedB1_ev){
					pressedB1_ev = false; //acknowledge button press 
					VR1min = vr1; //store the current voltages for VR1 and VR2 as the minimum
					VR2min = vr2;
					setRGB(RED, RGB_ON) ; //only RED LED on, acknowledge voltage recorded 
					setRGB(BLUE, RGB_OFF) ;
					setRGB(GREEN, RGB_OFF) ;
					shieldState = MeasureMax; //switch state 
				}
				break;
			
			case MeasureMax: 
				if(pressedB1_ev){
					pressedB1_ev = false;
					if(vr1 > VR1min && vr2 > VR2min){ //check the potentiometers have been turned up (more than the min recorded)
						VR1max = vr1; //store the current voltages for VR1 and VR2 as max 
						VR2max = vr2;
						setRGB(RED, RGB_OFF);
						setRGB(BLUE, RGB_OFF);
						setRGB(GREEN, RGB_ON); //only GREEN LED on, acknowledge voltage recorded 
						shieldCounter = 0; //set counter to 0 before switching state 
						shieldState = ShieldLEDsOn;
					} else { //if pots not been turned up, turn BLUE LED on, switch to error state (blue flashing)
						setRGB(RED, RGB_OFF);
						setRGB(GREEN, RGB_OFF);
						setRGB(BLUE, RGB_ON);
						shieldState = ErrOn;
					}
				}
				break;
				
			case ErrOn: //blue LED on
				if (errorCounter > 0) errorCounter--; 
			  if (errorCounter == 0){ //once 0.5s up, turn blue LED off 
					setRGB(RED, RGB_OFF);
					setRGB(GREEN, RGB_OFF);
					setRGB(BLUE, RGB_OFF);
					errorCounter = 50; //set counter back to 0.5s
					shieldState = ErrOff; //switch state 
				}
				break;
				
			case ErrOff: //blue LED off 
				if (errorCounter > 0) errorCounter--;
			  if (errorCounter == 0){ //turn blue LED on once 0.5s 
					setRGB(BLUE, RGB_ON);
					errorCounter = 50;
					shieldState = ErrOn;
				}
				break;
				
			case ShieldLEDsOn: //once successful callibration, turn all shield LEDs on 
				for (int i = LED1; i <= LED5; i++) { //turn all LEDs on 
					ledOnOff(i, LED_ON);  
				}
				v1 = vr1; //ensure current voltage value used 
				if (v1 < VR1min) v1 = VR1min; //ensures min timing of 0.5s even if pot is moved below VR1min
				if ((shieldCounter-50)*(VR1max-VR1min)>= 350*(v1-VR1min)){ //check if it's time to switch states 
					shieldState = ShieldLEDsOff; 
					shieldCounter = 0; //set counter back to 0 
				}
				if (pressedB1_ev){
					pressedB1_ev = false; 
					shieldState = FlashOff; //if button pressed while Shields on, turn system off 
				}
				break;
				
			case ShieldLEDsOff:
				for (int i = LED1; i <= LED5; i++) { //turn all LEDs off 
					ledOnOff(i, LED_OFF);  
				}
				v2 = vr2; 
				if (v2 < VR2min) v2 = VR2min; //ensures min timing of 0.5s even if pot is moved below VR2min
				if ((shieldCounter-50)*(VR2max-VR2min)>= 350*(v2-VR2min)){  
					shieldState = ShieldLEDsOn;
					shieldCounter = 0; //set counter back 
				}
				if (pressedB1_ev){
					pressedB1_ev = false;
					shieldState = FlashOff; //if button pressed while Shields off, turn system off 
				}
				break;
				
			case FlashOff:
				setRGB(RED, RGB_ON) ; //set LED red 
				setRGB(GREEN, RGB_OFF) ;
				for (int i = LED1; i <= LED5; i++) { //loop through all LEDs to turn off 
						ledOnOff(i, LED_OFF);  
					}
				if (pressedB1_ev){ //if pressed again turn system back on 
					setRGB(RED, RGB_OFF) ;
					setRGB(GREEN, RGB_ON) ; //set LED green 
					pressedB1_ev = false;
					shieldState = ShieldLEDsOn;
				}
				
		}
}

/*----------------------------------------------------------------------------
  MAIN function
 *----------------------------------------------------------------------------*/
int main (void) {

    // Initialise peripherals
    configureButtons(B1, false) ; // ConfigureButtons B1 for polling
    configureLEDs() ;                 // Configure shield LEDs
    configureRGB();                   // Configure the 3-colour LED on the FRDM board 
    
    // Initialise and calibrate ADC
    initADC() ; // Initialise ADC
    int calibrationFailed = ADC_Cal() ; // calibrate the ADC 
    while (calibrationFailed) ; // block progress if calibration failed
    initADC() ; // Reinitialise ADC
    initVR1pin() ; // Not needed usually as default use
    initVR2pin() ; // Not needed usually as default use

    // initialse SysTick every 1 ms
    Init_SysTick(1000) ;  

    // Initialise tasks and cycle counter
    initTask1PollB1() ;  
    initTask2MeasureVR() ;
		initTask3FlashLEDs();
    waitSysTickCounter(10) ;  
    
    while (1) {      // this runs forever
        task1PollB1() ;    // Generate signals for a simulated button
			  task2MeasureVR(); //measure voltage for VR1 and VR2
				task3FlashLEDs(); //flash shield LEDs
        // delay
        waitSysTickCounter(10) ;  // cycle every 10 ms 
    }
}
