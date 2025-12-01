#ifndef SOUND_H
#define SOUND_H

// Initialize the sound module (starts background thread)
void sound_init(void);

// Clean up resources and stop thread
void sound_cleanup(void);

// Play the "dingdong.wav" file (non-blocking)
void sound_play_doorbell(void);

// Play the "alarm.wav" file (non-blocking)
void sound_play_alarm(void);

// Stop any currently playing sound
void sound_stop(void);

#endif