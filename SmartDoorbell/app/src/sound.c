#define _GNU_SOURCE
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

// --- CONFIGURATION ---
// Ensure these files exist relative to where you run the program!
#define DOORBELL_WAV "audio-files/dingdong.wav"
#define ALARM_WAV    "audio-files/alarm.wav"

// To play sound on a specific card (if needed), use "aplay -D plughw:1,0"
#define APLAY_CMD "aplay -q" 

static pthread_t sound_thread;
static pthread_mutex_t sound_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sound_cond  = PTHREAD_COND_INITIALIZER;

static bool running = true;
static int  command = 0; // 0=None, 1=Doorbell, 2=Alarm, 3=Stop

static void* sound_thread_func(void* args) {
    (void)args;
    while (running) {
        int current_cmd = 0;

        // Wait for a command
        pthread_mutex_lock(&sound_mutex);
        while (command == 0 && running) {
            pthread_cond_wait(&sound_cond, &sound_mutex);
        }
        current_cmd = command;
        command = 0; // Reset command after reading
        pthread_mutex_unlock(&sound_mutex);

        if (!running) break;

        // Execute Command
        char cmd_buffer[256];
        switch (current_cmd) {
            case 1: // Doorbell
                snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s", APLAY_CMD, DOORBELL_WAV);
                system(cmd_buffer); 
                break;
            case 2: // Alarm
                snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s", APLAY_CMD, ALARM_WAV);
                system(cmd_buffer);
                break;
            case 3: // Stop
                system("killall -q aplay");
                break;
        }
    }
    return NULL;
}

void sound_init(void) {
    running = true;
    command = 0;
    pthread_create(&sound_thread, NULL, sound_thread_func, NULL);
}

void sound_cleanup(void) {
    pthread_mutex_lock(&sound_mutex);
    running = false;
    pthread_cond_signal(&sound_cond);
    pthread_mutex_unlock(&sound_mutex);
    pthread_join(sound_thread, NULL);
}

void sound_play_doorbell(void) {
    pthread_mutex_lock(&sound_mutex);
    command = 1;
    pthread_cond_signal(&sound_cond);
    pthread_mutex_unlock(&sound_mutex);
}

void sound_play_alarm(void) {
    pthread_mutex_lock(&sound_mutex);
    command = 2;
    pthread_cond_signal(&sound_cond);
    pthread_mutex_unlock(&sound_mutex);
}

void sound_stop(void) {
    pthread_mutex_lock(&sound_mutex);
    command = 3;
    pthread_cond_signal(&sound_cond);
    pthread_mutex_unlock(&sound_mutex);
}