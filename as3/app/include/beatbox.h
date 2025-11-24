#ifndef BEATBOX_H
#define BEATBOX_H

// Initialize the beatbox (load sounds, start thread)
void Beatbox_init(void);

// Cleanup (free sounds, stop thread)
void Beatbox_cleanup(void);

// Beatbox Control Functions
void Beatbox_setMode(int mode);
int Beatbox_getMode(void);
void Beatbox_cycleMode(void);

void Beatbox_setBPM(int bpm);
int Beatbox_getBPM(void);
void Beatbox_changeBPM(int amount);

void Beatbox_setVolume(int volume);
int Beatbox_getVolume(void);
void Beatbox_changeVolume(int amount);

// Play a specific sound index (0=Base, 1=Snare, 2=HiHat)
void Beatbox_playSound(int soundIndex);

#endif