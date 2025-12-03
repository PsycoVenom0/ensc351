#include "audioLogic.h"
#include "audioMixer.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

// Constants
#define MIN_BPM 40
#define MAX_BPM 300
#define DEFAULT_BPM 120
#define MIN_VOL 0
#define MAX_VOL 100
#define DEFAULT_VOL 80

// Files
#define FILE_BASE "beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define FILE_HIHAT "beatbox-wav-files/100053__menegass__gui-drum-cc.wav"
#define FILE_SNARE "beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"

static int bpm = DEFAULT_BPM;
static int volume = DEFAULT_VOL;
static int mode = 1; 
static volatile bool stopping = false; // Controls the main application loop
static pthread_t beatThreadId;
static pthread_mutex_t beatMutex = PTHREAD_MUTEX_INITIALIZER;

static wavedata_t baseDrum;
static wavedata_t hiHat;
static wavedata_t snare;

void Beatbox_playSound(int soundIndex) {
    switch (soundIndex) {
        case 0: AudioMixer_queueSound(&baseDrum); break;
        case 1: AudioMixer_queueSound(&snare); break;
        case 2: AudioMixer_queueSound(&hiHat); break;
    }
}

static void* beatThread(void* arg) {
    (void)arg;
    while (!stopping) {
        int currentMode, currentBPM;
        
        pthread_mutex_lock(&beatMutex);
        currentMode = mode;
        currentBPM = bpm;
        pthread_mutex_unlock(&beatMutex);

        if (currentMode == 0) {
            usleep(100000); 
            continue;
        }

        long delay_us = (60 * 1000 * 1000) / currentBPM / 2;

        // 8 half-beats for standard rock
        for (int i = 0; i < 8 && !stopping; i++) {
            pthread_mutex_lock(&beatMutex);
            if (mode != currentMode) {
                pthread_mutex_unlock(&beatMutex);
                break; 
            }
            currentBPM = bpm;
            delay_us = (60 * 1000 * 1000) / currentBPM / 2;
            pthread_mutex_unlock(&beatMutex);

            if (currentMode == 1) { // Rock
                if(i==0 || i==2 || i==4 || i==6) AudioMixer_queueSound(&hiHat);
                if(i==0 || i==4) AudioMixer_queueSound(&baseDrum);
                if(i==2 || i==6) AudioMixer_queueSound(&snare);
            } 
            else if (currentMode == 2) { // Custom
                if (i == 0 || i == 3 || i == 4) AudioMixer_queueSound(&baseDrum);
                if (i == 2 || i == 6) AudioMixer_queueSound(&snare);
                if (i == 0 || i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 6 || i == 7) AudioMixer_queueSound(&hiHat);
            }

            usleep(delay_us);
        }
    }
    return NULL;
}

void Beatbox_init(void) {
    AudioMixer_readWaveFileIntoMemory(FILE_BASE, &baseDrum);
    AudioMixer_readWaveFileIntoMemory(FILE_HIHAT, &hiHat);
    AudioMixer_readWaveFileIntoMemory(FILE_SNARE, &snare);
    stopping = false;
    pthread_create(&beatThreadId, NULL, beatThread, NULL);
}

void Beatbox_cleanup(void) {
    stopping = true;
    pthread_join(beatThreadId, NULL);
    AudioMixer_freeWaveFileData(&baseDrum);
    AudioMixer_freeWaveFileData(&hiHat);
    AudioMixer_freeWaveFileData(&snare);
}

void Beatbox_setMode(int newMode) {
    pthread_mutex_lock(&beatMutex);
    mode = newMode;
    // Mode 0=None, 1=Rock, 2=Custom. Cycle 0->1->2->0
    if (mode > 2) mode = 0; 
    pthread_mutex_unlock(&beatMutex);
}
int Beatbox_getMode(void) { return mode; }
void Beatbox_cycleMode(void) { Beatbox_setMode(mode + 1); }

void Beatbox_setBPM(int newBPM) {
    pthread_mutex_lock(&beatMutex);
    bpm = newBPM;
    if (bpm < MIN_BPM) bpm = MIN_BPM;
    if (bpm > MAX_BPM) bpm = MAX_BPM;
    pthread_mutex_unlock(&beatMutex);
}
int Beatbox_getBPM(void) { return bpm; }
void Beatbox_changeBPM(int amount) { Beatbox_setBPM(bpm + amount); }

void Beatbox_setVolume(int newVol) {
    pthread_mutex_lock(&beatMutex);
    volume = newVol;
    if (volume < MIN_VOL) volume = MIN_VOL;
    if (volume > MAX_VOL) volume = MAX_VOL;
    AudioMixer_setVolume(volume);
    pthread_mutex_unlock(&beatMutex);
}
int Beatbox_getVolume(void) { return volume; }
void Beatbox_changeVolume(int amount) { Beatbox_setVolume(volume + amount); }

// Shutdown Flags
void Beatbox_markStopping(void) { stopping = true; }
bool Beatbox_isStopping(void) { return stopping; }