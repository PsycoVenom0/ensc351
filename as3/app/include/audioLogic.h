#ifndef AUDIOLOGIC_H
#define AUDIOLOGIC_H

#include <stdbool.h>

// Initialize/Cleanup
void Beatbox_init(void);
void Beatbox_cleanup(void);

// Control
void Beatbox_setMode(int mode);
int Beatbox_getMode(void);
void Beatbox_cycleMode(void);

void Beatbox_setBPM(int bpm);
int Beatbox_getBPM(void);
void Beatbox_changeBPM(int amount);

void Beatbox_setVolume(int volume);
int Beatbox_getVolume(void);
void Beatbox_changeVolume(int amount);

void Beatbox_playSound(int soundIndex);

// Graceful Shutdown
void Beatbox_markStopping(void);
bool Beatbox_isStopping(void);

#endif