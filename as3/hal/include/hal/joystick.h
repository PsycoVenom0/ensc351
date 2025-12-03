#ifndef JOYSTICK_H_
#define JOYSTICK_H_

typedef enum {
    JS_NONE,
    JS_UP,
    JS_DOWN,
    JS_LEFT,
    JS_RIGHT
} JoystickDirection;

// Initialize joystick logic (no hardware init needed, relies on ADC)
void Joystick_init(void);

// Cleanup (currently empty)
void Joystick_cleanup(void);

// Get current joystick direction (Only reads Y axis for assignment)
JoystickDirection Joystick_read(void);

#endif