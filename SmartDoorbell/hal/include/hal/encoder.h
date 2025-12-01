#pragma once

typedef void (*EncoderVOLUMECB)(int newVOLUME);
typedef void (*EncoderButtonCB)(void);

/* Initialize and start the encoder reading thread */
void Encoder_init(void);

/* Stop the encoder thread and release GPIO lines */
void Encoder_cleanup(void);

/* Get the current BPM */
int Encoder_get_volume(void);

/* Set the current BPM (clamped to [40, 300]) */
void Encoder_set_volume(int bpm);

/* Register optional callbacks */
void Encoder_set_volume_callback(EncoderVOLUMECB cb);
void Encoder_set_button_callback(EncoderButtonCB cb);
