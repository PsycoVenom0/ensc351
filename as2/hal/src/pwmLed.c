// hal/src/pwmLed.c
#define _GNU_SOURCE
#include "hal/pwmLed.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#ifndef PWM_SYS_DIR
#define PWM_SYS_DIR "/dev/hat/pwm/GPIO12/"  /* Default path per PWM guide */
#endif

#define PWM_MAX_HZ 500
#define PWM_DEFAULT_HZ 10

static pthread_mutex_t pwm_lock = PTHREAD_MUTEX_INITIALIZER;
static int current_hz = PWM_DEFAULT_HZ;

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
    long long period_ns = 1000000000LL / current_hz;
    write_int("period", period_ns);
    write_int("duty_cycle", period_ns / 2);
    write_int("enable", 1);
    pthread_mutex_unlock(&pwm_lock);
}

void PWM_cleanup(void) {
    pthread_mutex_lock(&pwm_lock);
    write_int("enable", 0);
    pthread_mutex_unlock(&pwm_lock);
}

int PWM_set_frequency(int hz) {
    if (hz < 0) hz = 0;
    if (hz > PWM_MAX_HZ) hz = PWM_MAX_HZ;

    pthread_mutex_lock(&pwm_lock);
    if (hz == current_hz) {
        pthread_mutex_unlock(&pwm_lock);
        return 0;
    }

    if (hz == 0) {
        write_int("enable", 0);
        current_hz = 0;
        pthread_mutex_unlock(&pwm_lock);
        return 0;
    }

    long long period_ns = 1000000000LL / hz;
    write_int("duty_cycle", 0);
    write_int("period", period_ns);
    write_int("duty_cycle", period_ns / 2);
    write_int("enable", 1);

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
