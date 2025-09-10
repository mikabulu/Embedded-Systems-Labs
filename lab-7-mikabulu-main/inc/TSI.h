// Header file for TSI
//   Function prototypes

#ifndef TSI_DEFS_H
#define TSI_DEFS_H

#include <MKL28Z7.h>
//#include <stdbool.h>

#define NO_TOUCH                 0
#define SLIDER_LENGTH           40 //LENGTH in mm
#define TOTAL_ELECTRODE          2

//#define TSI9         9    // TSI channel for electrode 1
//#define TSI10        10   // TSI channel for electrode 2
#define TSI_E1         2    // TSI channel for electrode 1, aka PTA1 (ALT0; disabled at reset)
#define TSI_E2         3    // TSI channel for electrode 2, aka PTA2 (ALT0; disabled at reset)


#define THRESHOLD0   100
#define THRESHOLD1   100

void TSI_init(void) ;
uint8_t readTSIDistance();

#endif
