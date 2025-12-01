#define _GNU_SOURCE
#include "hal/encoder.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>
#include <errno.h>

// Hardware configuration
#define ENCODER_CHIP_A "/dev/gpiochip2"
#define ENCODER_LINE_A 8

#define ENCODER_CHIP_B "/dev/gpiochip1"
#define ENCODER_LINE_B 33

#define ENCODER_CHIP_BTN "/dev/gpiochip1"
#define ENCODER_LINE_BTN 41

// Debounce settings
#define ROTATION_DEBOUNCE_US 20000  // 20ms for rotation
#define BUTTON_DEBOUNCE_US 200000   // 200ms for button press

// VOLUME limits per assignment requirements
#define VOLUME_MIN 0
#define VOLUME_MAX 100
#define VOLUME_DEFAULT 50
#define VOLUME_STEP 5

static int running = 0;
static pthread_t thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int VOLUME = VOLUME_DEFAULT;
static EncoderVOLUMECB VOLUME_callback = NULL;
static EncoderButtonCB button_callback = NULL;

static void invoke_VOLUME_callback(int b) {
    if (VOLUME_callback) VOLUME_callback(b);
}

static void invoke_button_callback(void) {
    if (button_callback) button_callback();
}

// The background thread that polls the rotary encoder
static void *encoder_thread(void *arg) {
    (void)arg;
    struct gpiod_chip *chip_a = NULL;
    struct gpiod_chip *chip_b = NULL;
    struct gpiod_line_request *req_a = NULL;
    struct gpiod_line_request *req_b = NULL;
    struct gpiod_line_request *req_btn = NULL;
    int lastA = 0;
    int lastBtn = 1;  // Button idle is HIGH (pulled up)

    // Open GPIO chips
    chip_a = gpiod_chip_open(ENCODER_CHIP_A);
    if (!chip_a) {
        perror("encoder_thread: gpiod_chip_open (chip_a)");
        return NULL;
    }

    chip_b = gpiod_chip_open(ENCODER_CHIP_B);
    if (!chip_b) {
        perror("encoder_thread: gpiod_chip_open (chip_b)");
        gpiod_chip_close(chip_a);
        return NULL;
    }

    // Configure settings for encoder lines
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        perror("encoder_thread: gpiod_line_settings_new");
        gpiod_chip_close(chip_a);
        gpiod_chip_close(chip_b);
        return NULL;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_debounce_period_us(settings, 1000);

    // Request Channel A
    struct gpiod_request_config *req_cfg_a = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg_a, "encoder_a");
    struct gpiod_line_config *line_cfg_a = gpiod_line_config_new();
    unsigned int offset_a = ENCODER_LINE_A;
    gpiod_line_config_add_line_settings(line_cfg_a, &offset_a, 1, settings);
    req_a = gpiod_chip_request_lines(chip_a, req_cfg_a, line_cfg_a);
    gpiod_request_config_free(req_cfg_a);
    gpiod_line_config_free(line_cfg_a);
    if (!req_a) {
        perror("encoder_thread: gpiod_chip_request_lines (A)");
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip_a);
        gpiod_chip_close(chip_b);
        return NULL;
    }

    // Request Channel B
    struct gpiod_request_config *req_cfg_b = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg_b, "encoder_b");
    struct gpiod_line_config *line_cfg_b = gpiod_line_config_new();
    unsigned int offset_b = ENCODER_LINE_B;
    gpiod_line_config_add_line_settings(line_cfg_b, &offset_b, 1, settings);
    req_b = gpiod_chip_request_lines(chip_b, req_cfg_b, line_cfg_b);
    gpiod_request_config_free(req_cfg_b);
    gpiod_line_config_free(line_cfg_b);
    if (!req_b) {
        perror("encoder_thread: gpiod_chip_request_lines (B)");
        gpiod_line_settings_free(settings);
        gpiod_line_request_release(req_a);
        gpiod_chip_close(chip_a);
        gpiod_chip_close(chip_b);
        return NULL;
    }

    // Request Button
    struct gpiod_request_config *req_cfg_btn = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg_btn, "encoder_btn");
    struct gpiod_line_config *line_cfg_btn = gpiod_line_config_new();
    unsigned int offset_btn = ENCODER_LINE_BTN;
    gpiod_line_config_add_line_settings(line_cfg_btn, &offset_btn, 1, settings);
    req_btn = gpiod_chip_request_lines(chip_b, req_cfg_btn, line_cfg_btn);
    gpiod_request_config_free(req_cfg_btn);
    gpiod_line_config_free(line_cfg_btn);
    gpiod_line_settings_free(settings);
    
    if (!req_btn) {
        perror("encoder_thread: gpiod_chip_request_lines (BTN)");
        gpiod_line_request_release(req_a);
        gpiod_line_request_release(req_b);
        gpiod_chip_close(chip_a);
        gpiod_chip_close(chip_b);
        return NULL;
    }

    // Initialize last states
    lastA = gpiod_line_request_get_value(req_a, ENCODER_LINE_A);
    if (lastA < 0) lastA = 1;
    
    lastBtn = gpiod_line_request_get_value(req_btn, ENCODER_LINE_BTN);
    if (lastBtn < 0) lastBtn = 1;

    // Main polling loop
    while (running) {
        usleep(1000); // Poll every 1ms
        
        // Check rotation (Channel A falling edge)
        int a = gpiod_line_request_get_value(req_a, ENCODER_LINE_A);
        if (a < 0) {
            perror("encoder_thread: get_value (A)");
            continue;
        }

        if (lastA == 1 && a == 0) {
            // Falling edge on A - check B for direction
            int b = gpiod_line_request_get_value(req_b, ENCODER_LINE_B);
            if (b < 0) {
                perror("encoder_thread: get_value (B)");
                lastA = a;
                continue;
            }

            pthread_mutex_lock(&lock);
            
            // FIX: Reversed Logic
            // If B is 1 (High) -> Increase
            // If B is 0 (Low)  -> Decrease
            if (b == 1 && VOLUME < VOLUME_MAX) {
                VOLUME += VOLUME_STEP;
                if (VOLUME > VOLUME_MAX) VOLUME = VOLUME_MAX;
            } else if (b == 0 && VOLUME > VOLUME_MIN) {
                VOLUME -= VOLUME_STEP;
                if (VOLUME < VOLUME_MIN) VOLUME = VOLUME_MIN;
            }
            int current_VOLUME = VOLUME;
            pthread_mutex_unlock(&lock);
            
            invoke_VOLUME_callback(current_VOLUME);
            
            // Debounce cooldown
            usleep(ROTATION_DEBOUNCE_US);
            lastA = gpiod_line_request_get_value(req_a, ENCODER_LINE_A);
        } else {
            lastA = a;
        }

        // Check button press (falling edge)
        int btn = gpiod_line_request_get_value(req_btn, ENCODER_LINE_BTN);
        if (btn < 0) {
            perror("encoder_thread: get_value (BTN)");
            continue;
        }

        if (lastBtn == 1 && btn == 0) {
            // Button pressed
            invoke_button_callback();
            
            // Debounce cooldown
            usleep(BUTTON_DEBOUNCE_US);
            lastBtn = gpiod_line_request_get_value(req_btn, ENCODER_LINE_BTN);
        } else {
            lastBtn = btn;
        }
    }

    // Cleanup
    gpiod_line_request_release(req_a);
    gpiod_line_request_release(req_b);
    gpiod_line_request_release(req_btn);
    gpiod_chip_close(chip_a);
    gpiod_chip_close(chip_b);
    return NULL;
}

void Encoder_init(void) {
    pthread_mutex_lock(&lock);
    running = 1;
    VOLUME = VOLUME_DEFAULT;
    pthread_mutex_unlock(&lock);
    pthread_create(&thread, NULL, encoder_thread, NULL);
}

void Encoder_cleanup(void) {
    pthread_mutex_lock(&lock);
    running = 0;
    pthread_mutex_unlock(&lock);
    pthread_join(thread, NULL);
}

int Encoder_get_VOLUME(void) {
    pthread_mutex_lock(&lock);
    int b = VOLUME;
    pthread_mutex_unlock(&lock);
    return b;
}

void Encoder_set_VOLUME(int new_VOLUME) {
    pthread_mutex_lock(&lock);
    VOLUME = new_VOLUME;
    if (VOLUME < VOLUME_MIN) VOLUME = VOLUME_MIN;
    if (VOLUME > VOLUME_MAX) VOLUME = VOLUME_MAX;
    pthread_mutex_unlock(&lock);
}

void Encoder_set_VOLUME_callback(EncoderVOLUMECB cb) {
    VOLUME_callback = cb;
}

void Encoder_set_button_callback(EncoderButtonCB cb) {
    button_callback = cb;
}