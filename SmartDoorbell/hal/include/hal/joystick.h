#ifndef HAL_JOYSTICK_H
#define HAL_JOYSTICK_H

#include <stdint.h>

typedef enum {
    JOY_NONE = 0,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    JOY_CENTER
} joystick_dir_t;

//initialize joystick 
int hal_joystick_init(const char *spi_device, uint32_t spi_speed_hz);


void hal_joystick_cleanup(void);

//to find direction of joystick
joystick_dir_t hal_joystick_read_direction(void);

//waiting to release joystick
void hal_joystick_wait_until_released(void);

//to help read adc values
int hal_joystick_read_raw(int *x_out, int *y_out);

#endif 

