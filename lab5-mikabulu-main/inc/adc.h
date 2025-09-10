/*----------------------------------------------------------------------------
       ECS642/ECS714 Lab Work

    Header File for potentiometers and ADC
    
    Functions:
       void initADC(void)              - initialise ADC for conversion on s/w request    
       uint8_t ADC_Cal(ADC_MemMapPtr)  - calibrate; call once
       void initVR1pin(void)           - initialise the VR1 pin
       void initVR2pin(void)           - initialse the VR2 pin
       uint16_t MeasureVR(VR_t vr)     - measure on one pin
       int16_t MeasureVRDiff(void)     - differential measurement

 *---------------------------------------------------------------------------*/

#ifndef ADC_H
#define ADC_H

#include <MKL28Z7.h>
#include <stdbool.h>
#include <stdint.h>

//   There are three possible measurements
typedef enum {VR1, VR2, VRDiff} VR_t ;

/* 
      The pins are:
         * PTE20 for VR1
         * PTE21 for VR2

      These correspond to the following ADC channels
         * PTE20: Single ended SE0; differential DP0
         * PTE21: Single ended SE4a; differential DM0
         
      The configuration is:
         * SE0: SC1n[DIFF] = 0, SC1n[ADCH] = 00000
         * SE4a: SC1n[DIFF] = 0, SC1n[ADCH] = 00100
         * DP0/DM0: SC1n[DIFF] = 1, SC1n[ADCH] = 00000
*/

#define VR1pin (20)    // on port E
#define VR2pin (21)    // on port E

#define VR1_CHANNEL (0)	// channel for VR1pin
#define VR2_CHANNEL (4)	// channel for VR2pin
#define VRDIFF_CHANNEL (0)	// channel for differential: VR1 minus VR2

// API
void initADC(void) ; // initialise the ADC for single s/w conversion
uint8_t ADC_Cal(void) ;
void initVR1pin(void) ;
void initVR2pin(void) ; 
uint16_t MeasureVR(VR_t vr) ;
int16_t MeasureVRDiff(void) ;

#endif
