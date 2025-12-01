#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <math.h> 

#include "hal/led.h"
#include "hal/joystick.h"
#include "hal/accelerometer.h" 
#include "sound.h"
#include "camera.h"
#include "udp_client.h"

// --- CONFIG ---
#define ESP32_IP "192.168.4.1" 
#define PIN_LENGTH 4
static const joystick_dir_t SECRET_PIN[PIN_LENGTH] = { JOY_LEFT, JOY_LEFT, JOY_UP, JOY_DOWN };

#define TAMPER_THRESHOLD 500 

long long current_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

int main(void) {
    hal_led_init();
    camera_init();
    udp_init();
    sound_init(); 
    Accel_init(); 

    if (hal_joystick_init("/dev/spidev0.0", 250000) != 0) {
        printf("Joystick Init Failed! (Continuing anyway...)\n");
    }

    joystick_dir_t input_buffer[PIN_LENGTH];
    int input_count = 0;
    long long last_motion_check = 0;
    bool button_was_pressed = false;

    int last_x = 0, last_y = 0, last_z = 0;
    Accel_readXYZ(&last_x, &last_y, &last_z); 

    printf("=== BEAGLEY-AI SMART DOORBELL STARTED ===\n");
    hal_led_red_on(); 

    while (1) {
        // --- 1. DOORBELL BUTTON ---
        bool button_is_pressed = hal_joystick_is_pressed();
        
        if (button_is_pressed && !button_was_pressed) {
            printf("[DOORBELL] Button Pressed! Ding Dong!\n");
            sound_play_doorbell(); 
            udp_send("Doorbell Button Pressed");
        }
        button_was_pressed = button_is_pressed;

        // --- 2. PIN CODE LOGIC ---
        joystick_dir_t dir = hal_joystick_read_direction();
        
        if (dir != JOY_NONE && dir != JOY_CENTER) {
            printf("[INPUT] Direction: %d\n", dir);
            
            // REMOVED: sound_play_input(); 

            input_buffer[input_count++] = dir;
            
            // Visual feedback
            hal_led_red_off(); hal_led_green_on();
            usleep(100000); 
            hal_led_green_off(); hal_led_red_on();

            hal_joystick_wait_until_released();

            if (input_count >= PIN_LENGTH) {
                bool correct = true;
                for(int i=0; i<PIN_LENGTH; i++) {
                    if(input_buffer[i] != SECRET_PIN[i]) correct = false;
                }

                if (correct) {
                    printf("[ACCESS] UNLOCKING DOOR\n");
                    sound_play_correct(); 
                    udp_send("Door Unlocked by PIN");
                    
                    hal_led_red_off(); hal_led_green_on();
                    sleep(3); 
                    hal_led_green_off(); hal_led_red_on();
                } else {
                    printf("[ACCESS] DENIED\n");
                    sound_play_incorrect(); 
                    hal_led_flash_red_n_times(3, 500);
                }
                input_count = 0; 
            }
        }

        // --- 3. TAMPER DETECTION ---
        int x, y, z;
        Accel_readXYZ(&x, &y, &z);
        int delta = abs(x - last_x) + abs(y - last_y) + abs(z - last_z);
        
        if (delta > TAMPER_THRESHOLD) {
            printf("[ALARM] TAMPER DETECTED! Delta: %d\n", delta);
            sound_play_alarm();
            udp_send("TAMPER DETECTED: Device Shaken!");
            
            for(int i=0; i<5; i++) {
                hal_led_red_on(); usleep(50000);
                hal_led_red_off(); usleep(50000);
            }
            hal_led_red_on();
            
            sleep(2); 
            Accel_readXYZ(&last_x, &last_y, &last_z);
        } else {
            last_x = x; last_y = y; last_z = z;
        }

        // --- 4. MOTION LOGIC ---
        long long now = current_ms();
        if (now - last_motion_check > 200) {
            if (input_count == 0) {
                if (camera_capture(ESP32_IP) == 0) {
                    if (camera_check_motion()) {
                        printf("[MOTION] Movement detected!\n");
                        udp_send("Motion Detected at Front Door");
                        sleep(2); 
                    }
                }
            }
            last_motion_check = now;
        }

        usleep(10000); 
    }

    sound_cleanup();
    Accel_cleanup();
    hal_joystick_cleanup();
    hal_led_cleanup();
    udp_cleanup();
    return 0;
}