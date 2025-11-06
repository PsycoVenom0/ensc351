#pragma once
/**
 * encoder.h
 *
 * Rotary encoder interface.
 * Reads A/B signals via GPIO (using libgpiod) and updates the
 * LED flash frequency. All threading and GPIO details are hidden
 * inside this module.
 *
 * The encoder starts at 10 Hz, increments/decrements frequency by 1 Hz
 * per detent, and clamps between 0 and 500 Hz.
 */

typedef void (*EncoderFreqCB)(int newFreq);

/* Initialize and start the encoder reading thread */
void Encoder_init(void);

/* Stop the encoder thread and release GPIO lines */
void Encoder_cleanup(void);

/* Get the current frequency (Hz) */
int Encoder_get_frequency(void);

/* Register an optional callback called on frequency change */
void Encoder_set_callback(EncoderFreqCB cb);
