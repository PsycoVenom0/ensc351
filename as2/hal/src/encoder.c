// hal/src/encoder.c
#define _GNU_SOURCE
#include "hal/encoder.h"
#include "hal/pwmLed.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h> // This header provides the v2 API functions
#include <errno.h>

#ifndef GPIO_CHIP_PATH
#define GPIO_CHIP_PATH "/dev/gpiochip0"
#endif

/* Your wiring: A=17, B=16 */
#ifndef ENCODER_LINE_A
#define ENCODER_LINE_A 17
#endif
#ifndef ENCODER_LINE_B
#define ENCODER_LINE_B 16
#endif

// Software debounce period in microseconds (e.g., 5000us = 5ms)
#define DEBOUNCE_US 5000

static int running = 0;
static pthread_t thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int freq = 10;  /* Start at 10Hz per assignment */
static EncoderFreqCB callback = NULL;

static void invoke_callback(int f) {
    if (callback) callback(f);
}

static void *encoder_thread(void *arg) {
    (void)arg;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line_request *req = NULL;
    struct gpiod_line_settings *settings = NULL;
    int lastA = 0;

    chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) {
        perror("encoder_thread: gpiod_chip_open");
        return NULL;
    }

    // 1. Create a new settings object
    settings = gpiod_line_settings_new();
    if (!settings) {
        perror("encoder_thread: gpiod_line_settings_new");
        gpiod_chip_close(chip);
        return NULL;
    }

    // 2. Configure the settings for input with debounce
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_debounce_period_us(settings, DEBOUNCE_US);
    
    // 3. Request the two lines (A and B)
    const unsigned int offsets[] = { ENCODER_LINE_A, ENCODER_LINE_B };
    req = gpiod_chip_request_lines(chip, "encoder", settings, 2, offsets);
    
    gpiod_line_settings_free(settings); // No longer needed
    if (!req) {
        perror("encoder_thread: gpiod_chip_request_lines");
        gpiod_chip_close(chip);
        return NULL;
    }

    // 4. Get the initial value of line A
    lastA = gpiod_line_request_get_value(req, ENCODER_LINE_A);
    if (lastA < 0) {
        perror("encoder_thread: gpiod_line_request_get_value (initial)");
        lastA = 0;
    }
    
    while (running) {
        usleep(1000); // 1ms polling interval

        // 5. Read the current values
        int a = gpiod_line_request_get_value(req, ENCODER_LINE_A);
        int b = gpiod_line_request_get_value(req, ENCODER_LINE_B);

        if (a < 0 || b < 0) {
            perror("encoder_thread: gpiod_line_request_get_value (loop)");
            continue;
        }
        
        /* Detect rising edge on A and determine direction by B */
        if (lastA == 0 && a == 1) {
            pthread_mutex_lock(&lock);
            
            if (b == 0 && freq < 500)
                freq++;   // clockwise
            else if (b == 1 && freq > 0)
                freq--;   // counterclockwise
            
            int f = freq;
            pthread_mutex_unlock(&lock);

            PWM_set_frequency(f);
            invoke_callback(f);
        }
        lastA = a;
    }

    // 6. Release the lines and close the chip
    gpiod_line_request_release(req);
    gpiod_chip_close(chip);
    return NULL;
}

void Encoder_init(void) {
    pthread_mutex_lock(&lock);
    running = 1;
    pthread_create(&thread, NULL, encoder_thread, NULL);
    PWM_set_frequency(freq);
    pthread_mutex_unlock(&lock);
}

void Encoder_cleanup(void) {
    pthread_mutex_lock(&lock);
    running = 0;
    pthread_mutex_unlock(&lock);
    pthread_join(thread, NULL);
}

int Encoder_get_frequency(void) {
    pthread_mutex_lock(&lock);
    int f = freq;
    pthread_mutex_unlock(&lock);
    return f;
}

void Encoder_set_callback(EncoderFreqCB cb) {
    callback = cb;
}