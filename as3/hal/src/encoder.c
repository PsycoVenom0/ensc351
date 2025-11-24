#define _GNU_SOURCE
#include "hal/encoder.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>
#include <errno.h>

// Hardware configuration found using `gpioinfo`
#ifndef GPIO_CHIP_PATH
#define GPIO_CHIP_PATH "/dev/gpiochip2"
#endif

#ifndef ENCODER_LINE_A
#define ENCODER_LINE_A 7  // "GPIO16" is Line 7 on chip 2
#endif
#ifndef ENCODER_LINE_B
#define ENCODER_LINE_B 8  // "GPIO17" is Line 8 on chip 2
#endif

// Cooldown period to prevent contact bounce (20ms)
#define DEBOUNCE_COOLDOWN_US 20000

static int running = 0;
static pthread_t thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int freq = 10; // Start at 10Hz
static EncoderFreqCB callback = NULL;

static void invoke_callback(int f) {
    if (callback) callback(f);
}

// The background thread that polls the rotary encoder
static void *encoder_thread(void *arg) {
    (void)arg;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line_request *req = NULL;
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_request_config *req_cfg = NULL;
    struct gpiod_line_config *line_cfg = NULL;
    int lastA = 0;

    // Open the GPIO chip
    chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) {
        perror("encoder_thread: gpiod_chip_open");
        return NULL;
    }

    // Configure settings for the GPIO lines
    settings = gpiod_line_settings_new();
    if (!settings) {
        perror("encoder_thread: gpiod_line_settings_new");
        gpiod_chip_close(chip);
        return NULL;
    }
    // Configure lines as input
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    // Attempt to set internal pull-up (matches external hardware)
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_debounce_period_us(settings, 1000); // 1ms hardware debounce

    // Configure the request
    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "encoder");

    // Configure the specific lines (A and B)
    line_cfg = gpiod_line_config_new();
    const unsigned int offsets[2] = {ENCODER_LINE_A, ENCODER_LINE_B};
    gpiod_line_config_add_line_settings(line_cfg, offsets, 2, settings);

    // Request the lines from the kernel
    req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_settings_free(settings);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);

    if (!req) {
        perror("encoder_thread: gpiod_chip_request_lines");
        gpiod_chip_close(chip);
        return NULL;
    }

    // Initialize lastA to the current state (should be HIGH/1)
    lastA = gpiod_line_request_get_value(req, ENCODER_LINE_A);
    if (lastA < 0) {
        perror("encoder_thread: gpiod_line_request_get_value (initial)");
        lastA = 1; // Assume idle HIGH state
    }

    // Main polling loop
    while (running) {
        // Poll every 1ms
        usleep(1000); 
        
        int a = gpiod_line_request_get_value(req, ENCODER_LINE_A);
        if (a < 0) {
            perror("encoder_thread: gpiod_line_request_get_value (A)");
            continue;
        }

        // Look for a FALLING edge (1 -> 0)
        if (lastA == 1 && a == 0) {
            
            // A has fallen, IMMEDIATELY check B to get direction
            int b = gpiod_line_request_get_value(req, ENCODER_LINE_B);
            if (b < 0) {
                 perror("encoder_thread: gpiod_line_request_get_value (B)");
                 continue;
            }

            pthread_mutex_lock(&lock);
            
            // Quadrature logic:
            // Swapped to make b=0 increase and b=1 decrease (matches HW)
            if (b == 0 && freq < 500) {
                freq++; // Increase frequency
            } else if (b == 1 && freq > 0) {
                freq--; // Decrease frequency
            }

            int f = freq;
            pthread_mutex_unlock(&lock);
            
            PWM_set_frequency(f);
            invoke_callback(f);
            
            // DEBOUNCE COOLDOWN:
            // Wait for contacts to settle before looking for the next click.
            usleep(DEBOUNCE_COOLDOWN_US);
            
            // Re-read 'a' to get the settled state
            lastA = gpiod_line_request_get_value(req, ENCODER_LINE_A);
        } else {
            // No event, just update lastA
            lastA = a;
        }
    }

    // Cleanup
    gpiod_line_request_release(req);
    gpiod_chip_close(chip);
    return NULL;
}

void Encoder_init(void) {
    pthread_mutex_lock(&lock);
    running = 1;
    pthread_create(&thread, NULL, encoder_thread, NULL);
    PWM_set_frequency(freq); // Set initial 10Hz frequency
    pthread_mutex_unlock(&lock);
}

void Encoder_cleanup(void) {
    pthread_mutex_lock(&lock);
    running = 0;
    pthread_mutex_unlock(&lock);
    pthread_join(thread, NULL); // Wait for thread to finish
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