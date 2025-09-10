
/*----------------------------------------------------------------------------
    Given code for Embedded Systems Lab 7
    
    Poll the touch pad on the FRDM KL28Z development and write distance to the 
    terminal.

    There are two threads
       t_touch: polls the touch pad every 25ms seconds to get touch distance; write
           message to the serial terminal. Sets event flags based on different touch distance
					 ranges. 
       t_redBlueLED: soft keypad used to control the brightness of the blue and red LEDs. 
    
 *---------------------------------------------------------------------------*/
 
#include "cmsis_os2.h"
#include <MKL28Z7.h>
#include <stdbool.h>
#include "../inc/rgb.h"
#include "../inc/serialPort.h"
#include "../inc/TSI.h"
#include "../inc/TPMPWM.h"
#include "../inc/triColorLedPWM.h"

osEventFlagsId_t touchEvtFlags ;          // event flags


/*--------------------------------------------------------------
 *   Thread t_touch
 *      Read touch position periodically. Set flags based on the touch distance. 
 *      Each key defined by a sub-range of touch distance values. 
 *--------------------------------------------------------------*/
osThreadId_t t_touch;      /* id of thread to control brightness of red and blue LED*/

//define states (softkeys) 
#define LeftOut (0)
#define RightOut (1)
#define LeftIn (2)
#define RightIn (3)
#define None (4)

//define flags 
#define LEFTOUT_FLAG  (1)  
#define LEFTIN_FLAG   (2)  
#define RIGHTIN_FLAG  (4)  
#define RIGHTOUT_FLAG (8)  


// convert unsigned 8-bit to XXX
void uToString(uint8_t a, char* s) {  

    // array of digits
    int digit[3] ;  // [0] least significant
    uint8_t a_dash ;

    // digits
    int ix;
    for (ix = 0 ; ix < 3; ix++) {
        a_dash = a / 10 ;   // 234 --> 23 
        digit[2-ix] = a - (10 * a_dash) ;  // 234 - 230 --> 4
        a = a_dash ;    // 23
    }
    
    // skip leading zero
    ix = 0 ;
    while ( (ix < 2) && (digit[ix] == 0) ) {
        s[ix] = ' ' ;
        ix++ ;
    }
    
    // characters
    while (ix < 3) {
        s[ix] = digit[ix] + '0' ;
        ix++ ;
    }
}

char distStr[] = "dist = XXX" ;
//                01234567890

void touchThread(void *arg) {
		uint8_t tsiPos; // touch position
		uint8_t lastPos = 0; // store last position
		int touchState = None; // state of touch pad 
    // data buffer
    uint8_t touchDist ; 
        
    // loop polling touch sensor every 25ms
    while(1) {
        osDelay(25) ;
        tsiPos = readTSIDistance(); 
			
			// print distance only if position has changed 
        if (tsiPos != lastPos) {
            uToString(tsiPos, &distStr[7]);
            sendMsg(distStr, CRLF);
            lastPos = tsiPos;
        }
				
			  //set flags on different softkey presses 
			  switch (touchState) {
					case None: 
						if ((tsiPos > 3) && (tsiPos < 9)){ //outlerleft softkey range 4-8
							osEventFlagsSet(touchEvtFlags, LEFTOUT_FLAG) ;
							touchState = LeftOut;
						}
						else if ((tsiPos > 13) && (tsiPos < 19)) { //inerrleft softkey range 14-18
							osEventFlagsSet(touchEvtFlags, LEFTIN_FLAG) ;
							touchState = LeftIn;
						}
						else if ((tsiPos > 23) && (tsiPos < 29)) { //innerright softkey range 24-28
							osEventFlagsSet(touchEvtFlags, RIGHTIN_FLAG) ;
							touchState = RightIn;
						}
						else if (tsiPos > 33){ //outerright softkey range 34-39
							osEventFlagsSet(touchEvtFlags, RIGHTOUT_FLAG) ;
							touchState = RightOut;
						}
						break;
					
					//if in inactive zone, switch state to None
					case LeftOut: 
						if ((tsiPos < 3) || (tsiPos > 9)){
							touchState = None;
						}
						break;
						
					case LeftIn: 
						if ((tsiPos < 13) || (tsiPos > 19)){
							touchState = None;
						}
						break;
						
					case RightIn: 
						if ((tsiPos < 23) || (tsiPos > 29)){
							touchState = None;
						}
						break;
						
					case RightOut: 
						if (tsiPos < 33){
							touchState = None;
						}
						break;
				}
		}
	}
/*--------------------------------------------------------------
 *   Thread t_redBlueLED
 *  - two outer softkeys control brightness. Outer left increases, outer right decreases. 
 *  - single press of key changes brightness by one. when brightness reaches max/min 
 *    values further presses have no effect. 
 *  - two inner keys select the LED being controlled. Inner left for red, inner right for
 *    blue. 
 *--------------------------------------------------------------*/

osThreadId_t t_redBlueLED;        /* id of thread to flash red led */

#define RED (0)
#define BLUE (1)
#define IDLE (2)

void LEDThread (void *arg) {
    uint32_t flags ;                // returned by osEventFlagWait
	  
	  unsigned int redBright = 0; //red off 
		unsigned int blueBright = 0; //blue off
	  unsigned int greenBright = 0; //green off
	  //set all LEDs to lowest brightness (off) 
	  setLEDBrightness(Red, redBright);
		setLEDBrightness(Blue, blueBright);
	  setLEDBrightness(Green, blueBright);
	
	  int LEDState = IDLE; //start in IDLE state, not in control of either LED 
	
    while (1) {
			//wait for any flag 
			flags = osEventFlagsWait(touchEvtFlags, LEFTOUT_FLAG | LEFTIN_FLAG | RIGHTIN_FLAG | RIGHTOUT_FLAG, 
                         osFlagsWaitAny, osWaitForever); 
			
			//print msg to acknowledge which softkey pressed (flag set)
			if (flags == LEFTOUT_FLAG) {
				sendMsg("Outer left key pressed", CRLF);
			}
			else if (flags == LEFTIN_FLAG){
				sendMsg("Inner left key pressed", CRLF);
			}
			else if (flags == RIGHTIN_FLAG){
				sendMsg("Inner right key pressed", CRLF);
			}
			else if (flags == RIGHTOUT_FLAG){
				sendMsg("Outer right key pressed", CRLF);
			}
			
			//respond to flags to control LEDs 
			switch(LEDState){ 
				case IDLE:
					if (flags == LEFTIN_FLAG){ //innerleft softkey press switch to RED 
						LEDState = RED;
					}
					else if (flags == RIGHTIN_FLAG){ //innerright softkey press switch to BLUE 
						LEDState = BLUE; 
					}
					break;
					
				case RED: //in control of RED LED 
					if ((flags == LEFTOUT_FLAG) && (redBright < 7)) { //on every outerleft press, increase brightness (MAX 7)
						redBright++;
						setLEDBrightness(Red, redBright);
					}
					else if ((flags == RIGHTOUT_FLAG) && (redBright > 0)){ //on every outerright press, decrease brightness (MIN 0)
						redBright--;
						setLEDBrightness(Red, redBright);
					}
					else if (flags == RIGHTIN_FLAG){ // innerright soft key press switch colour control to BLUE 
						LEDState = BLUE; 
					}
					break;
					
				case BLUE: //in control of BLUE LED 
					if ((flags == LEFTOUT_FLAG) && (blueBright < 7)) { 
						blueBright++;
						setLEDBrightness(Blue, blueBright);
					}
					else if ((flags == RIGHTOUT_FLAG) && (blueBright > 0)){
						blueBright--;
						setLEDBrightness(Blue, blueBright);
					}
					else if (flags == LEFTIN_FLAG){ // innerright soft key press switch colour control to RED
						LEDState = RED; 
					}
					break;
				}
		}
}
/*----------------------------------------------------------------------------
 * Application main
 *   Initialise I/O
 *   Initialise kernel
 *   Create threads
 *   Start kernel
 *---------------------------------------------------------------------------*/

int main (void) { 
    
    // System Initialization
    SystemCoreClockUpdate();
  
    // Initialise RGB LEDs 
    configureRGB() ;
	
	  // configure PWM 
    configureLEDforPWM();
    configureTPMClock();
    configureTPM0forPWM();


    //configureGPIOinput();
    init_UART0(115200) ;

    // Initialize CMSIS-RTOS
    osKernelInitialize();
    
    // initialise serial port 
    initSerialPort() ;

    // initialise touch sensor
    TSI_init() ;

    // Create event flags
    touchEvtFlags = osEventFlagsNew(NULL);
    
    // Create threads
    t_touch = osThreadNew(touchThread, NULL, NULL); 
    t_redBlueLED = osThreadNew(LEDThread, NULL, NULL); 
	
 
    osKernelStart();    // Start thread execution - DOES NOT RETURN
    for (;;) {}         // Only executed when an error occurs
}
