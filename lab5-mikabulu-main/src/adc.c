#include <MKL28Z7.h>
#include "..\inc\adc.h"

/* ================================

QUERIES: what is the relationship between the shield VR and the ADC?/
HOW should this be structured? 

   This file provides the following functions
      Init_ADC  - intialises the ADC
      MeasureVR1 - result is unsigned
      MeasureVR2 - result is unsigned
      MeasureVRDiff - result is signed
      ADC_Cal - calibration the ADC
      
      
      The pins are:
         * PTE20 for VR1 (name is VR1pin)
         * PTE21 for VR2 (name is VR2pin)

      These correspond to the following ADC channels
         * PTE20: Single ended SE0; differential DP0
         * PTE21: Single ended SE4a; differential DM0
         
      The configuration is:
         * SE0: SC1n[DIFF] = 0, SC1n[ADCH] = 00000
         * SE4a: SC1n[DIFF] = 0, SC1n[ADCH] = 00100
         * DP0/DM0: SC1n[DIFF] = 1, SC1n[ADCH] = 00000
   
   See https://community.nxp.com/t5/Kinetis-Microcontrollers/KL28-ADC-clocking-how-to-use-other-sources-than-the-ADCK/m-p/771059 
   for a long discussion on why the DIV3 clock is not used as peripheral function clock here.

   ================================ */


/* ------------------------------------
   Initialise ADC 
     Use S/W trigger, single conversion, no average and 
     no comparison. 16 bit resolution, with a long sample time.

     Note: there is no initialisation of the PORT/PCR as
      the ADC alternative is the default. 
   ------------------------------------- */

void initADC(void) {
    // Enable clock to ADC
    PCC_ADC0 |= PCC_CLKCFG_CGC(1) ;

    // Set the ADC0_CFG1 to 0x9C, which is 1001 1100
    //   1 --> low power conversion
    //   00 --> ADIV is 1, no divide
    //   1 --> ADLSMP is long sample time
    //   11 --> MODE is 16 bit conversion
    //   01 --> ADIClK is bus clock / 2
    ADC0->CFG1 = 0x9C ;

    // Set the ADC0_SC2 register to 0
    //   0 --> DATRG - s/w trigger
    //   0 --> ACFE - compare disable
    //   0 --> ACFGT - n/a when compare disabled
    //   0 --> ACREN - n/a when compare disabled
    //   0 --> DMAEN - DMA is disabled
    //   00 -> REFSEL - defaults V_REFH and V_REFL selected
    ADC0->SC2 = 0 ;
}

void initVR1pin(void) {
    // Enable clock to ports E
    PCC_PORTE |= PCC_CLKCFG_CGC(1) ;
    PORTE->PCR[VR1pin] &= ~PORT_PCR_MUX_MASK;
}

void initVR2pin(void) {
    // Enable clock to ports E
    PCC_PORTE |= PCC_CLKCFG_CGC(1) ;
    PORTE->PCR[VR2pin] &= ~PORT_PCR_MUX_MASK;
}

///* --------------------------------------------
//    Variables to hold result
//       Declare volatile to ensure changes seen in debugger
//   -------------------------------------------- */

//volatile uint16_t sres ;        // raw value - single 
//volatile int16_t dres ;         // raw value - differential

/* ---------------------------------------
     Take one measurement of the ADC input for VR1 or VR2
        vr Value of VR_t enum, assume VR2 if not VR1
        Returns a uint16_t

     The clock to the port MUST BE ENABLED: this is not done here.
   --------------------------------------- */
uint16_t MeasureVR(VR_t vr) {
    ADC0->SC1[0] = 
//    ADC_SC1_REG(ADC0,0) =       // Write to ADC0_SC1A 
        //   0 --> AIEN Conversion interrupt diabled
        ADC_SC1_AIEN(0) |
        //   0 --> DIFF single end conversion 
        ADC_SC1_DIFF(0) |
        // channel determined by pin  
        (vr == VR1 ? ADC_SC1_ADCH(VR1_CHANNEL) : ADC_SC1_ADCH(VR2_CHANNEL)) ;
        // writing to this clears the COCO flag 

    // test the conversion complete flag, which is 1 when completed
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
        ; // empty loop

    // Read results from ADC0_RA as an unsigned integer    
    return ADC0->R[0] ; // reading this clears the COCO flag  
}

/* ---------------------------------------
    Take one measurement of the ADC input for VR1 - VR2
        Differential on VR1 minus VR2
        Returns a signed int16_t

    The clock to the port MUST BE ENABLED: this is not done here.
   --------------------------------------- */

int16_t MeasureVRDiff(void) {
    ADC0->SC1[0] = 
//    ADC_SC1_REG(ADC0,0) =       // Write to ADC0_SC1A 
        //   0 --> AIEN Conversion interrupt diabled
        ADC_SC1_AIEN(0) |
        //   1 --> DIFF differential conversion 
        ADC_SC1_DIFF(1) |
        // 01000 --> ADCH, selecting DAD0 
        ADC_SC1_ADCH(VRDIFF_CHANNEL) ;
        // writing to this clears the COCO flag 

    // test the conversion complete flag, which is 1 when completed
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
        ; // empty loop

    // Read results from ADC0_RA as a signed integer    
    return ADC0->R[0] ; // reading this clears the COCO flag  
}


/******************************************************************************
Function AUTO CAL ROUTINE   

Parameters      ADC module pointer points to adc0 or adc1 register map 
                base address.
Returns  Zero indicates success.

Notes    Calibrates the ADC16. Required to meet specifications 
         after reset and before a conversion is initiated.

Taken from
https://github.com/bingdo/FRDM-KL25Z-WIZ550io/blob/master/system/src/kl25-sc/adc16.c

Adjusted for KL28Z macro definitions - 21/8/2024
******************************************************************************/

uint8_t ADC_Cal() {
    unsigned short cal_var;
  
    ADC0->SC2 &=  ~ADC_SC2_ADTRG_MASK ; // Enable Software Conversion Trigger for Calibration Process    - ADC0_SC2 = ADC0_SC2 | ADC_SC2_ADTRGW(0);   
    ADC0->SC3 &= ( ~ADC_SC3_ADCO_MASK & ~ADC_SC3_AVGS_MASK ); // set single conversion, clear avgs bitfield for next writing
    ADC0->SC3  |= ( ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(0x3u) );  // Turn averaging ON and set at max value ( 32 )
    ADC0->SC3  |= ADC_SC3_CAL_MASK ;      // Start CAL

    while ( (ADC0->SC1[0] & ADC_SC1_COCO_MASK ) == 0 ); // Wait calibration end

    if ((ADC0->SC3 & ADC_SC3_CALF_MASK) == 1 ) {  
        return(1);    // Check for Calibration fail error and return 
    }

    // Calculate plus-side calibration
    cal_var = 0x00;
    cal_var =  ADC0->CLP0 ; 
    cal_var += ADC0->CLP1 ;
    cal_var += ADC0->CLP2 ;
    cal_var += ADC0->CLP3 ;
    cal_var += ADC0->CLP4 ;
    cal_var += ADC0->CLPS ;
    cal_var = cal_var/2;
    cal_var |= 0x8000; // Set MSB
    ADC0->PG = ADC_PG_PG(cal_var);

    // Calculate minus-side calibration
    cal_var = 0x00;
    cal_var =  ADC0->CLM0 ; 
    cal_var += ADC0->CLM1 ;
    cal_var += ADC0->CLM2 ;
    cal_var += ADC0->CLM3 ;
    cal_var += ADC0->CLM4 ;
    cal_var += ADC0->CLMS ;
    cal_var = cal_var/2;
    cal_var |= 0x8000; // Set MSB
    ADC0->MG = ADC_MG_MG(cal_var); 

     ADC0->SC3 &= ~ADC_SC3_CAL_MASK ; /* Clear CAL bit */
    return(0);
}
