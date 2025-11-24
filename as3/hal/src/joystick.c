#include "hal/joystick.h"
#include "hal/adc.h"
#include <stdlib.h>

// --- WIRING MAPPING ---
// Joystick Y -> Pin 8 -> Ch 7
#define JOY_Y_CH  7 

// Joystick Thresholds (0-4095)
#define CENTER 2048
#define DEADZONE 1500 
#define UPPER_LIMIT (CENTER + DEADZONE)
#define LOWER_LIMIT (CENTER - DEADZONE)

void Joystick_init(void) {
    // Hardware is handled by ADC_init()
}

JoystickDirection Joystick_read(void) {
    // We only read Y for Volume control
    int y = ADC_read_raw(JOY_Y_CH);
    
    if (y == -1) return JS_NONE;

    // "Up" on the stick usually reads as 0 (Low Voltage) or 4095 (High)
    // depending on orientation. 
    // Assumption: Low Voltage (0) is UP. Swap returns if inverted.
    if (y < LOWER_LIMIT) return JS_DOWN;   
    if (y > UPPER_LIMIT) return JS_UP; 
    
    return JS_NONE;
}

void Joystick_cleanup(void) {
    // Nothing to clean up here
}