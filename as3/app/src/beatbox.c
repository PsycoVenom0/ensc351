#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include "audioMixer.h"
#include "audioLogic.h"
#include "udpServer.h"
#include "hal/joystick.h"
#include "hal/accelerometer.h"
#include "hal/encoder.h"
#include "hal/adc.h"
#include "periodTimer.h"

// Helper to get time in milliseconds
static long long getTimeMs(void) {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

// Encoder Callbacks
void on_bpm_change(int newBpm) {
    Beatbox_setBPM(newBpm);
}

void on_mode_button_press(void) {
    Beatbox_cycleMode();
}

int main(void) {
    printf("Starting Beatbox...\n");

    // 1. Initialize Hardware & Modules
    // FIX: Initialize PeriodTimer FIRST so other threads can use it immediately
    Period_init(); 
    
    ADC_init();
    Joystick_init();
    Accel_init(); 
    Encoder_init();
    
    // Register encoder callbacks
    Encoder_set_BPM_callback(on_bpm_change);
    Encoder_set_button_callback(on_mode_button_press);

    AudioMixer_init();
    Beatbox_init();
    UDP_init();

    long long lastStatTime = getTimeMs();
    long long lastJoyTime = 0;
    long long lastAccelTime = 0;
    
    // Debounce duration for accel shakes (ms)
    const int ACCEL_DEBOUNCE = 200; 

    // 2. Main Loop
    while (!Beatbox_isStopping()) {
        long long now = getTimeMs();

        // --- Joystick (Volume) ---
        // Debounce joystick by time (200ms), not sleep
        if (now - lastJoyTime > 200) {
            JoystickDirection dir = Joystick_read();
            if (dir == JS_UP) {
                Beatbox_changeVolume(5);
                lastJoyTime = now;
            } else if (dir == JS_DOWN) {
                Beatbox_changeVolume(-5);
                lastJoyTime = now;
            }
        }

        // --- Accelerometer (Air Drumming) ---
        int x, y, z;
        Accel_readXYZ(&x, &y, &z);
        Period_markEvent(PERIOD_EVENT_ACCEL_READ);

        // Simple shake detection logic
        // Center is roughly 2048 (12-bit ADC). 
        // Threshold deviations:
        int thresh = 1000; // Sensitivity

        if (now - lastAccelTime > ACCEL_DEBOUNCE) {
            bool triggered = false;
            // X-Axis Shake
            if (abs(x - 2048) > thresh) {
                Beatbox_playSound(1); // Snare
                triggered = true;
            }
            // Y-Axis Shake
            else if (abs(y - 2048) > thresh) {
                Beatbox_playSound(2); // Hi-Hat
                triggered = true;
            }
            // Z-Axis Shake (Gravity is ~1G down, so resting is different, likely ~2048+offset or -offset)
            // We just check large deviation from what is likely 'flat' (2048)
            else if (abs(z - 2048) > thresh) {
                Beatbox_playSound(0); // Base
                triggered = true;
            }

            if (triggered) {
                lastAccelTime = now;
            }
        }
        
        // --- 1 Second Status Print ---
        if (now - lastStatTime >= 1000) {
            Period_statistics_t audioStats;
            Period_statistics_t accelStats;
            
            Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO_BUFFER, &audioStats);
            Period_getStatisticsAndClear(PERIOD_EVENT_ACCEL_READ, &accelStats);

            printf("M%d %dbpm vol:%d  Audio[%.3f, %.3f] avg %.3f/%d  Accel[%.3f, %.3f] avg %.3f/%d\n",
                Beatbox_getMode(),
                Beatbox_getBPM(),
                Beatbox_getVolume(),
                audioStats.minPeriodInMs, audioStats.maxPeriodInMs, audioStats.avgPeriodInMs, audioStats.numSamples,
                accelStats.minPeriodInMs, accelStats.maxPeriodInMs, accelStats.avgPeriodInMs, accelStats.numSamples
            );
            
            lastStatTime = now;
        }

        usleep(10000); // Poll at 100Hz
    }

    // 3. Cleanup
    printf("Cleaning up...\n");
    UDP_cleanup();
    Beatbox_cleanup();
    AudioMixer_cleanup();
    Encoder_cleanup();
    Accel_cleanup();
    ADC_cleanup();
    Period_cleanup();
    
    printf("Program Terminated.\n");
    return 0;
}