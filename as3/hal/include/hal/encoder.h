#pragma once

/**
 * encoder.h
 *
 * Rotary encoder interface for Assignment 3.
 * Reads A/B signals and push button via GPIO (using libgpiod).
 * 
 * - Spinning the encoder adjusts BPM (beats per minute) by ±5
 * - Pressing the button cycles through drum beat modes
 * - BPM range: [40, 300] (default 120 BPM)
 */

typedef void (*EncoderBPMCB)(int newBPM);
typedef void (*EncoderButtonCB)(void);

/* Initialize and start the encoder reading thread */
void Encoder_init(void);

/* Stop the encoder thread and release GPIO lines */
void Encoder_cleanup(void);

/* Get the current BPM */
int Encoder_get_BPM(void);

/* Set the current BPM (clamped to [40, 300]) */
void Encoder_set_BPM(int bpm);

/* Register optional callbacks */
void Encoder_set_BPM_callback(EncoderBPMCB cb);
void Encoder_set_button_callback(EncoderButtonCB cb);
