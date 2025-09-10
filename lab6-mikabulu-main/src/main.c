
/*----------------------------------------------------------------------------
    Given code for Embedded Systems Lab 6
    
    This project illustrates the use of 
      - threads
      - a message queue

    The behaviour is:
        - the speed of the green LED and red LED flashing can be turned changed using 
				  command entered on a terminal emulator and send over the USB link
        - the system is initialised in the RED state 
        
    There are two threads
       t_serial: waits for input from the terminal; sends message to t_flashLED
       t_flashLED: alternates turning the green LED and red LED on. On time changes with the 
			 commands sent from the terminal, there are 8 different on times. 
   
    
    Message queue: 
       * Message are put in the queue t_serial
       * Messages are read from the queue by t_flashLED

 *---------------------------------------------------------------------------*/
 
#include "cmsis_os2.h"
#include "string.h"
#include <stdio.h>

#include <MKL28Z7.h>
#include <stdbool.h>
#include "..\inc\rgb.h"
#include "..\inc\mask.h"
#include "..\inc\serialPort.h"

#define RESET_EVT (1)
osEventFlagsId_t errorFlags ;       // id of the event flags
osMessageQueueId_t controlMsgQ ;    // id for the message queue

/*--------------------------------------------------------------
 *   Thread t_flashLED
 *      Set the on time of the green LED and red LED alternating, on receipt of message. 
        There are 8 different speeds (delay times) 0.5-4 seconds. 
				faster: increasing by 0.5s
				slower: decreasing by 4s 
				Using command immediately starts with the new on-time.
 *      Messages: faster, slower (uses an enum)
 *      Acceptable message
 *         - in faster or slower state expect faster or slower message.  
 *--------------------------------------------------------------*/
osThreadId_t t_flashLED;      /* id of thread to flash LEDs */

// Green LED states
#define GREEN (0)
#define RED (1)
#define ERROR (2)
#define NUM_SPEEDS 8

const uint32_t speedTimes[NUM_SPEEDS] = {500, 1000, 1500, 2000, 2500, 3000, 3500, 4000};  // 0.5s to 4s
enum controlMsg_t {faster, slower} ;  // type for the messages

void flashLEDThread (void *arg) {
		char debugMsg[50];
    int ledState = RED;
    enum controlMsg_t msg ;
    osStatus_t status ;   // returned by message queue get 
		int speedIndex = 0;  // Start at fastest speed (500ms)
	
		uint32_t currentDelay = speedTimes[speedIndex]; //current LED ON time 
		uint32_t lastSwitchTime = osKernelGetTickCount();  // Record start time
		uint32_t newDelay; // new time updated from array 
	
		setRGB(GREEN, RGB_OFF);
    setRGB(RED, RGB_ON);
	
	  
    while (1) {
		
      uint32_t timeElapsed = osKernelGetTickCount() - lastSwitchTime; // time LED has been on
      uint32_t remainingTime = currentDelay - timeElapsed;            // time until the next switch
			
      // wait for message from queue
      status = osMessageQueueGet(controlMsgQ, &msg, NULL, remainingTime); //wake up on msg or after remainingTime  
			if (status == osOK) {    // change speed on msg
				//determine newDelay based on command 
            switch (msg) {
                case faster:
                    speedIndex = (speedIndex == 0) ? NUM_SPEEDS-1 : speedIndex-1;  // if at fastest index (0), wrap around to slowest speed otherwise decrease
                    break;
                case slower:
                    speedIndex = (speedIndex == NUM_SPEEDS-1) ? 0 : speedIndex+1;  // if at slowest index, wrap around to fastest speed otherwise increase
                    break;
            }
            newDelay = speedTimes[speedIndex];
						
						if (timeElapsed >= newDelay){ //if time passed is more than or equal to newDelay, switch immediately 
							switch (ledState) {
							case RED: 
								setRGB(GREEN, RGB_ON);
							  setRGB(RED, RGB_OFF);
							  ledState = GREEN; 
							  break;
							
							case GREEN:
								setRGB(GREEN, RGB_OFF);
							  setRGB(RED, RGB_ON);
							  ledState = RED; 
							  break;
						}
						lastSwitchTime = osKernelGetTickCount(); //update lastSwitchTime every LED switch 
            currentDelay = newDelay; //set new flash time
						}
						else { 
							remainingTime = newDelay - timeElapsed;
              currentDelay = newDelay;
							}
						}
			else { //normal timeout occured, switch LEDs
				switch (ledState) {
							case RED: 
								setRGB(GREEN, RGB_ON);
							  setRGB(RED, RGB_OFF);
							  ledState = GREEN;
							  break;
							
							case GREEN:
								setRGB(GREEN, RGB_OFF);
							  setRGB(RED, RGB_ON);
							  ledState = RED; 
							  break;
						}
					lastSwitchTime = osKernelGetTickCount();
					//remaining time calculated next loop 
					}
		}
}

/*------------------------------------------------------------
 *  Thread t_command
 *      Request user command
 *      
 *
 *------------------------------------------------------------*/
osThreadId_t t_command;        /* id of thread to receive command */

/* const */ char prompt[] = "Command: faster / slower >" ;
/* const */ char empty[] = "" ;

void commandThread (void *arg) {
    char response[7] ;  // buffer for response string
    enum controlMsg_t msg ;
    bool valid ;
    while (1) {
        //sendMsg(empty, CRLF) ;
        sendMsg(prompt, NOLINE) ;
        readLine(response, 7) ;  
  			valid = true ;
        if (strcmp(response, "faster") == 0) {
            msg = faster ;
        } else if (strcmp(response, "slower") == 0) {
            msg = slower ;
        } else valid = false ;
        
        if (valid) {
            osMessageQueuePut(controlMsgQ, &msg, 0, NULL);  // Send Message
        } else {
            sendMsg(response, NOLINE) ;
            sendMsg(" not recognised", CRLF) ;
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

    // Initialise peripherals
    configureRGB() ;
    init_UART0(115200) ;
    //init_UART0(9600) ;

    // Initialize CMSIS-RTOS
    osKernelInitialize();
    
    // Create event flags
    errorFlags = osEventFlagsNew(NULL);
    
    // create message queue
    controlMsgQ = osMessageQueueNew(2, sizeof(enum controlMsg_t), NULL) ;

    // initialise serial port 
    initSerialPort() ;

    // Create threads
		t_flashLED = osThreadNew(flashLEDThread, NULL, NULL);
    t_command = osThreadNew(commandThread, NULL, NULL); 
    
    osKernelStart();    // Start thread execution - DOES NOT RETURN
    for (;;) {}         // Only executed when an error occurs
}
