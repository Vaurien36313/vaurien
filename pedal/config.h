#pragma once




// Matrix size
#define MATRIX_ROWS 1
#define MATRIX_COLS 5

// Matrix pins
#define MATRIX_ROW_PINS { F6 }        // A1 = PF6
#define MATRIX_COL_PINS { D4, C6, D7, E6, B6 } // add pin 10 = PB6 as 5th column

#define DIODE_DIRECTION ROW2COL
#define USB_MAX_POWER_CONSUMPTION 500
// Debounce
#define DEBOUNCE 5

// Bootmagic: hold pedal on boot to enter DFU
#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 3



// LEDs 
#define LED_LAYER1_PIN D2
#define LED_LAYER2_PIN D0
#define LED_LAYER3_PIN B4
#define LED_CLUSTER_PIN B5
