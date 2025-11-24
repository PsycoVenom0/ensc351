#include <stdio.h>
#include <unistd.h>
#include "audioMixer.h"
#include "audioLogic.h"
#include "udpServer.h"
#include "hal/joystick.h"
#include "hal/accelerometer.h"
#include "hal/encoder.h"
#include "hal/adc.h"
#include "periodTimer.h"

int main(void) {
    printf("Starting Beatbox...\n");

    // 1. Initialize Hardware & Modules
    ADC_init();
    Joystick_init();
    Accle_init(); // Typo fix in header: Accel_init()
    Encoder_init();
    AudioMixer_init();
    Beatbox_init();
    UDP_init();
    Period_init();

    // 2. Main Loop
    while (1) {
        // --- Joystick (Volume) ---
        JoystickDirection dir = Joystick_read();
        if (dir == JS_UP) {
            Beatbox_changeVolume(5);
            usleep(200000); // Debounce
        } else if (dir == JS_DOWN) {
            Beatbox_changeVolume(-5);
            usleep(200000);
        }

        // --- Encoder (Tempo & Mode) ---
        int ticks = Encoder_getTickCount();
        if (ticks != 0) {
            Beatbox_changeBPM(ticks * 5);
        }
        if (Encoder_isPressed()) {
            Beatbox_cycleMode();
            usleep(200000); // Debounce
        }

        // --- Accelerometer (Air Drumming) ---
        int x, y, z;
        Accel_readXYZ(&x, &y, &z);
        // Thresholds (Assuming ~2048 is center/1G)
        // Need to calibrate these based on testing!
        if (x > 2500 || x < 1500) {
            Beatbox_playSound(1); // Snare on X shake
            usleep(100000); // Debounce
        }
        // Y/Z checks similar...

        // --- Stats ---
        // (Call Period_markEvent() in other files if needed)
        
        usleep(10000); // 10ms poll rate
    }

    // 3. Cleanup (Unreachable in infinite loop, but good practice)
    UDP_cleanup();
    Beatbox_cleanup();
    AudioMixer_cleanup();
    Encoder_cleanup();
    Accel_cleanup();
    ADC_cleanup();
    return 0;
}