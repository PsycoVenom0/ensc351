#define _GNU_SOURCE
#include "hal/encoder.h"
#include "hal/pwmLed.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>
#include <errno.h>

#ifndef GPIO_CHIP_PATH
#define GPIO_CHIP_PATH "/dev/gpiochip2"
#endif

#ifndef ENCODER_LINE_A
#define ENCODER_LINE_A 7
#endif
#ifndef ENCODER_LINE_B
#define ENCODER_LINE_B 8
#endif

#define DEBOUNCE_US 5000

static int running = 0;
static pthread_t thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int freq = 10;
static EncoderFreqCB callback = NULL;

static void invoke_callback(int f) {
    if (callback) callback(f);
}

static void *encoder_thread(void *arg) {
    (void)arg;
    struct gpiod_chip *chip = NULL;
    struct gpiod_line_request *req = NULL;
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_request_config *req_cfg = NULL;
    struct gpiod_line_config *line_cfg = NULL;
    int lastA = 0;

    chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) {
        perror("encoder_thread: gpiod_chip_open");
        return NULL;
    }

    // Setup single settings object (both lines use same settings)
    settings = gpiod_line_settings_new();
    if (!settings) {
        perror("encoder_thread: gpiod_line_settings_new");
        gpiod_chip_close(chip);
        return NULL;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_debounce_period_us(settings, DEBOUNCE_US);

    // Setup request/line configs
    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "encoder");

    line_cfg = gpiod_line_config_new();
    const unsigned int offsets[2] = {ENCODER_LINE_A, ENCODER_LINE_B};

    // Use same settings for both lines
    gpiod_line_config_add_line_settings(line_cfg, offsets, 2, settings);

    // gpiod_chip_request_lines only takes 3 arguments in v2:
    req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_settings_free(settings);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);

    if (!req) {
        perror("encoder_thread: gpiod_chip_request_lines");
        gpiod_chip_close(chip);
        return NULL;
    }

    lastA = gpiod_line_request_get_value(req, ENCODER_LINE_A);
    if (lastA < 0) {
        perror("encoder_thread: gpiod_line_request_get_value (initial)");
        lastA = 0;
    }

    while (running) {
        usleep(1000);
        int a = gpiod_line_request_get_value(req, ENCODER_LINE_A);
        int b = gpiod_line_request_get_value(req, ENCODER_LINE_B);
        if (a < 0 || b < 0) {
            perror("encoder_thread: gpiod_line_request_get_value (loop)");
            continue;
        }

        if (lastA == 0 && a == 1) {
            pthread_mutex_lock(&lock);
            if (b == 1 && freq < 500)
                freq++;
            else if (b == 0 && freq > 0)
                freq--;
            int f = freq;
            pthread_mutex_unlock(&lock);
            PWM_set_frequency(f);
            invoke_callback(f);
        }
        lastA = a;
    }

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
