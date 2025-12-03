#pragma once
/**
 * pwmLed.h
 *
 * Simple PWM LED driver abstraction.
 * Provides functions to control LED flashing frequency (Hz)
 * using the system’s PWM device.
 */

void PWM_init(void);
void PWM_cleanup(void);

/* Sets LED flash frequency in Hz (0 disables output). */
int PWM_set_frequency(int hz);

/* Returns the current frequency setting. */
int PWM_get_frequency(void);
