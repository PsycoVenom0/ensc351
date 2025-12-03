#pragma once

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
