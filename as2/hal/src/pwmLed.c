#define _GNU_SOURCE
#include "hal/pwmLed.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Using GPIO13 (Pin 33) due to hardware conflict on GPIO12
#ifndef PWM_SYS_DIR
#define PWM_SYS_DIR "/dev/hat/pwm/GPIO13/"
#endif

#define PWM_MAX_HZ 500
#define PWM_DEFAULT_HZ 10

// Mutex to protect file access
static pthread_mutex_t pwm_lock = PTHREAD_MUTEX_INITIALIZER;
static int current_hz = PWM_DEFAULT_HZ;

// Helper to write an integer to a sysfs file
static int write_int(const char *name, long long value) {
    char path[256];
    snprintf(path, sizeof(path), "%s%s", PWM_SYS_DIR, name);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "%lld\n", value);
    fclose(f);
    return 0;
}

void PWM_init(void) {
    pthread_mutex_lock(&pwm_lock);
    write_int("duty_cycle", 0);
    if (current_hz <= 0) current_hz = PWM_DEFAULT_HZ;
    // Period is in nanoseconds
    long long period_ns = 1000000000LL / current_hz;
    write_int("period", period_ns);
    write_int("duty_cycle", period_ns / 2); // 50% duty cycle
    write_int("enable", 1);
    pthread_mutex_unlock(&pwm_lock);
}

void PWM_cleanup(void) {
    pthread_mutex_lock(&pwm_lock);
    write_int("enable", 0);
    pthread_mutex_unlock(&pwm_lock);
}

int PWM_set_frequency(int hz) {
    // Clamp frequency to allowed range [0, 500]
    if (hz < 0) hz = 0;
    if (hz > PWM_MAX_HZ) hz = PWM_MAX_HZ;

    pthread_mutex_lock(&pwm_lock);
    
    // Do not re-set if already at the desired frequency
    if (hz == current_hz) {
        pthread_mutex_unlock(&pwm_lock);
        return 0;
    }

    // Handle 0Hz (LED off)
    if (hz == 0) {
        write_int("enable", 0);
        current_hz = 0;
        pthread_mutex_unlock(&pwm_lock);
        return 0;
    }

    // Set new frequency
    long long period_ns = 1000000000LL / hz;
    write_int("duty_cycle", 0); // Must set duty=0 before period
    write_int("period", period_ns);
    write_int("duty_cycle", period_ns / 2);
    write_int("enable", 1); // Ensure it's on

    current_hz = hz;
    pthread_mutex_unlock(&pwm_lock);
    return 0;
}

int PWM_get_frequency(void) {
    pthread_mutex_lock(&pwm_lock);
    int f = current_hz;
    pthread_mutex_unlock(&pwm_lock);
    return f;
}