#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>

// Initialize the encoder (configure GPIOs)
void Encoder_init(void);

// Cleanup GPIO resources
void Encoder_cleanup(void);

// Get the number of "ticks" since the last call.
// Positive for clockwise, negative for counter-clockwise.
int Encoder_getTickCount(void);

// Check if the button is currently pressed (Active Low logic handled internally)
bool Encoder_isPressed(void);

#endif