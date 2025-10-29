#ifndef JOYSTICK_H_
#define JOYSTICK_H_

typedef enum {
    JS_NONE,
    JS_UP,
    JS_DOWN,
    JS_LEFT,
    JS_RIGHT
} JoystickDirection;

void Joystick_init(void);
JoystickDirection Joystick_read(void);
void Joystick_cleanup(void);

#endif
