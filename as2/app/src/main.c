#include "light_sampler.h"
#include "hal/pwmLed.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Initializing...\n");
    LightSampler_init();
    PWM_init();

    for (int i = 0; i < 100; i++) {
        double v = LightSampler_getAverage();
        printf("Light: %.2f V\n", v);
        PWM_setDutyCycle(v / 3.3 * 100.0);
        usleep(200000);
    }

    PWM_cleanup();
    LightSampler_cleanup();
    return 0;
}
