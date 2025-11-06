// hal/src/encoder.c
#define _GNU_SOURCE
#include "hal/encoder.h"
#include "hal/pwmLed.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>

#ifndef GPIO_CHIP_PATH
#define GPIO_CHIP_PATH "/dev/gpiochip0"
#endif

/* These GPIO line numbers may vary depending on wiring */
#ifndef ENCODER_LINE_A
#define ENCODER_LINE_A 17
#endif
#ifndef ENCODER_LINE_B
#define ENCODER_LINE_B 16
#endif

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
    struct gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) return NULL;

    struct gpiod_line *lineA = gpiod_chip_get_line(chip, ENCODER_LINE_A);
    struct gpiod_line *lineB = gpiod_chip_get_line(chip, ENCODER_LINE_B);
    if (!lineA || !lineB) {
        gpiod_chip_close(chip);
        return NULL;
    }

    gpiod_line_request_input(lineA, "encoder");
    gpiod_line_request_input(lineB, "encoder");

    int lastA = gpiod_line_get_value(lineA);

    while (running) {
        int a = gpiod_line_get_value(lineA);
        int b = gpiod_line_get_value(lineB);
        if (a < 0 || b < 0) {
            usleep(1000);
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
        usleep(1000);  // 1ms polling interval
    }

    gpiod_line_release(lineA);
    gpiod_line_release(lineB);
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
