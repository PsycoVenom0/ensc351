#include "light_sampler.h"
#include "hal/adc.h"
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

static pthread_t samplerThread;
static bool keepRunning = true;
static double lastValue = 0;

static void *samplerThreadFunc(void *arg) {
    (void)arg;
    while (keepRunning) {
        lastValue = ADC_read_voltage();
        usleep(10000); // sample every 10 ms
    }
    return NULL;
}

void LightSampler_init(void) {
    ADC_init();
    pthread_create(&samplerThread, NULL, samplerThreadFunc, NULL);
}

void LightSampler_cleanup(void) {
    keepRunning = false;
    pthread_join(samplerThread, NULL);
    ADC_cleanup();
}

double LightSampler_getAverage(void) {
    return lastValue;
}
