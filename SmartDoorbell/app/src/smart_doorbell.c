#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "hal/led.h"
#include "hal/joystick.h"
#include "camera.h"
#include "udp_client.h"

// --- CONFIG ---
#define ESP32_IP "192.168.4.1" // Default AP IP
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
    
    // Joystick on SPI0.0
    if (hal_joystick_init("/dev/spidev0.0", 250000) != 0) {
        printf("Joystick Init Failed!\n");
        return 1;
    }

    joystick_dir_t input_buffer[PIN_LENGTH];
    int input_count = 0;
    long long last_motion_check = 0;

    printf("=== SMART DOORBELL SYSTEM STARTED ===\n");
    hal_led_red_on(); // System Locked

    while (1) {
        // --- 1. JOYSTICK LOGIC (Polling) ---
        joystick_dir_t dir = hal_joystick_read_direction();
        
        if (dir != JOY_NONE && dir != JOY_CENTER) {
            // Button Pressed!
            printf("[INPUT] Direction: %d\n", dir);
            
            input_buffer[input_count++] = dir;
            
            // Blink Green feedback
            hal_led_red_off(); hal_led_green_on();
            usleep(100000); // 100ms
            hal_led_green_off(); hal_led_red_on();

            // Wait for release (Debounce)
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
                    sleep(3); // Keep open
                    hal_led_green_off(); hal_led_red_on();
                } else {
                    printf("[ACCESS] DENIED\n");
                    hal_led_flash_red_n_times(3, 500);
                }
                input_count = 0; // Reset
            }
        }

        // --- 2. MOTION LOGIC (Every 200ms) ---
        long long now = current_ms();
        if (now - last_motion_check > 200) {
            
            // Only check motion if no one is actively typing a PIN
            if (input_count == 0) {
                if (camera_capture(ESP32_IP) == 0) {
                    if (camera_check_motion()) {
                        printf("[MOTION] Movement detected!\n");
                        // Trigger UDP -> Node.js -> Discord
                        udp_send("Motion Detected at Front Door");
                        
                        // Wait a bit to avoid spamming Discord
                        sleep(5); 
                    }
                }
            }
            last_motion_check = now;
        }

        usleep(10000); // 10ms loop delay
    }

    hal_joystick_cleanup();
    hal_led_cleanup();
    udp_cleanup();
    return 0;
}