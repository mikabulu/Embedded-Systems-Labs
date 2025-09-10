#ifndef GPIO_H
#define GPIO_H

// ECS714P/ECS642U Lab 1 definitions - provided version

// Create a bit mask (32 bits) with only bit x set
#define MASK(x) (1UL << (x))

// Freedom KL28Z LEDs pin numbers
#define RED_LED_POS (29)		// on port E
#define GREEN_LED_POS (4)	    // on port C
#define BLUE_LED_POS (31)		// on port E


// Symbols for constants
#define OFF 0
#define ON 1
#define ONPERIOD 100 // time in 10ms units
#define OFFPERIOD 100 // time in 10ms units
#define OFFPERIOD2 200
#define ONPERIOD2 200
#define REDONPERIOD 500
#define REDOFFPERIOD 500
#define GREENONPERIOD 300
#define GREENOFFPERIOD 300


// States
#define REDOFF 0
#define REDON 1
#define GREENOFF 2
#define GREENON 3
#define BLUEOFF 4
#define BLUEON 5
#define CYANON 6
#define MAGENTAON 7
#define YELLOWON 8
#define WHITEON 9 

#endif
