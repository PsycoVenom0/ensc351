#ifndef LED_H_
#define LED_H_

// Initializes LEDs (sets trigger to none)
void LED_init(void);

// Turns green LED on/off
void LED_greenOn(void);
void LED_greenOff(void);

// Turns red LED on/off
void LED_redOn(void);
void LED_redOff(void);

// Flash LED n times within given duration (ms)
void LED_flashGreen(int times, long long durationMs);
void LED_flashRed(int times, long long durationMs);

// Turns both LEDs off
void LED_cleanup(void);

#endif
