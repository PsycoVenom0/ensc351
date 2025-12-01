#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "hal/led.h"
#include "hal/joystick.h"
#include "sound.h"
#include "camera.h"
#include "udp_client.h"

// --- CONFIG ---
#define ESP32_IP "192.168.4.1" 
#define PIN_LENGTH 4
static const joystick_dir_t SECRET_PIN[PIN_LENGTH] = { JOY_LEFT, JOY_LEFT, JOY_UP, JOY_DOWN };

// Timing helper
long long current_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

int main(void) {
    // 1. Initialize Hardware
    hal_led_init();
    camera_init();
    udp_init();
    sound_init(); // Initialize Sound
    
    // Joystick Init (Ensure /dev/spidevX.Y matches your BeagleY-AI SPI config)
    if (hal_joystick_init("/dev/spidev0.0", 250000) != 0) {
        printf("Joystick Init Failed! (Continuing anyway...)\n");
    }

    joystick_dir_t input_buffer[PIN_LENGTH];
    int input_count = 0;
    long long last_motion_check = 0;
    bool button_was_pressed = false;

    printf("=== BEAGLEY-AI SMART DOORBELL STARTED ===\n");
    hal_led_red_on(); // System Locked

    while (1) {
        // --- 1. DOORBELL BUTTON LOGIC ---
        // Check if the Joystick SEL button is pressed
        bool button_is_pressed = hal_joystick_is_pressed();
        
        if (button_is_pressed && !button_was_pressed) {
            printf("[DOORBELL] Button Pressed! Ding Dong!\n");
            sound_play_doorbell(); // Play sound in background
            udp_send("Doorbell Button Pressed");
        }
        button_was_pressed = button_is_pressed;


        // --- 2. PIN CODE LOGIC (Joystick Directions) ---
        joystick_dir_t dir = hal_joystick_read_direction();
        
        if (dir != JOY_NONE && dir != JOY_CENTER) {
            printf("[INPUT] Direction: %d\n", dir);
            
            input_buffer[input_count++] = dir;
            
            // Visual feedback (Blink Green)
            hal_led_red_off(); hal_led_green_on();
            usleep(100000); 
            hal_led_green_off(); hal_led_red_on();

            hal_joystick_wait_until_released();

            if (input_count >= PIN_LENGTH) {
                // Verify PIN
                bool correct = true;
                for(int i=0; i<PIN_LENGTH; i++) {
                    if(input_buffer[i] != SECRET_PIN[i]) correct = false;
                }

                if (correct) {
                    printf("[ACCESS] UNLOCKING DOOR\n");
                    udp_send("Door Unlocked by PIN");
                    
                    hal_led_red_off(); hal_led_green_on();
                    //sound_play_doorbell(); // Optional: sound on unlock
                    
                    sleep(3); // Keep open
                    
                    hal_led_green_off(); hal_led_red_on();
                } else {
                    printf("[ACCESS] DENIED\n");
                    hal_led_flash_red_n_times(3, 500);
                    // Optional: Play alarm on wrong PIN?
                    // hal_sound_play_alarm(); 
                }
                input_count = 0; // Reset
            }
        }

        // --- 3. MOTION LOGIC (Every 200ms) ---
        long long now = current_ms();
        if (now - last_motion_check > 200) {
            // Only check motion if user isn't typing PIN
            if (input_count == 0) {
                if (camera_capture(ESP32_IP) == 0) {
                    if (camera_check_motion()) {
                        printf("[MOTION] Movement detected!\n");
                        udp_send("Motion Detected at Front Door");
                        
                        // FUTURE TAMPER LOGIC:
                        // if (is_tampered) { hal_sound_play_alarm(); }
                        
                        sleep(5); 
                    }
                }
            }
            last_motion_check = now;
        }

        usleep(10000); // 10ms loop delay
    }

    // Cleanup
    sound_cleanup();
    hal_joystick_cleanup();
    hal_led_cleanup();
    udp_cleanup();
    return 0;
}