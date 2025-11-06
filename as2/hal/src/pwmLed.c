#include "hal/pwmLed.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PWM_PATH "/sys/class/pwm/pwmchip0"
#define PWM_CHANNEL 0
#define PWM_PERIOD_NS 20000000 // 20 ms period (50 Hz)

static char pwm_dir[128];

void PWM_init(void) {
    FILE *f;
    snprintf(pwm_dir, sizeof(pwm_dir), "%s/pwm%d", PWM_PATH, PWM_CHANNEL);
    if (access(pwm_dir, F_OK) != 0) {
        f = fopen(PWM_PATH "/export", "w");
        if (f) { fprintf(f, "%d", PWM_CHANNEL); fclose(f); usleep(100000); }
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/period", pwm_dir);
    f = fopen(path, "w");
    if (f) { fprintf(f, "%d", PWM_PERIOD_NS); fclose(f); }

    snprintf(path, sizeof(path), "%s/enable", pwm_dir);
    f = fopen(path, "w");
    if (f) { fprintf(f, "1"); fclose(f); }
}

void PWM_setDutyCycle(double dutyPercent) {
    if (dutyPercent < 0) dutyPercent = 0;
    if (dutyPercent > 100) dutyPercent = 100;
    int duty = (int)(PWM_PERIOD_NS * (dutyPercent / 100.0));

    char path[256];
    snprintf(path, sizeof(path), "%s/duty_cycle", pwm_dir);
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%d", duty); fclose(f); }
}

void PWM_cleanup(void) {
    FILE *f = fopen(PWM_PATH "/unexport", "w");
    if (f) { fprintf(f, "%d", PWM_CHANNEL); fclose(f); }
}
