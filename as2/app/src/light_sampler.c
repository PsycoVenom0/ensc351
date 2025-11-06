#define _GNU_SOURCE
#include "light_sampler.h"
#include "hal/adc.h"
#include "periodTimer.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SAMPLE_SLEEP_US 1000     // 1 ms between samples (~1000 samples/sec)
#define EMA_ALPHA 0.001          // exponential moving average weight

static pthread_t sampler_thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int running = 0;

static double *curr_buf = NULL;
static int curr_len = 0;
static int curr_cap = 0;

static double *hist_buf = NULL;
static int hist_len = 0;

static double avg_val = 0.0;
static int avg_initialized = 0;
static long long total_samples = 0;

static double stat_min_ms = 0.0;
static double stat_max_ms = 0.0;
static double stat_avg_ms = 0.0;
static int stat_count = 0;

/* --- internal helpers --- */
static void ensure_capacity(int needed)
{
    if (curr_cap >= needed) return;
    int new_cap = curr_cap ? curr_cap * 2 : 1024;
    while (new_cap < needed)
        new_cap *= 2;
    curr_buf = realloc(curr_buf, sizeof(double) * new_cap);
    curr_cap = new_cap;
}

/* --- background sampling thread --- */
static void *sampler_thread_func(void *arg)
{
    (void)arg;
    (void)Period_init; // This line seems to be a no-op, but is harmless.

    Period_init();

    while (running) {
        // NOTE: This relies on adc.c being hardcoded to the correct channel
        double v = ADC_read_voltage(); 
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);

        pthread_mutex_lock(&lock);

        ensure_capacity(curr_len + 1);
        curr_buf[curr_len++] = v;
        total_samples++;

        if (!avg_initialized) {
            avg_val = v;
            avg_initialized = 1;
        } else {
            // Apply exponential moving average
            avg_val = (1.0 - EMA_ALPHA) * avg_val + EMA_ALPHA * v;
        }

        pthread_mutex_unlock(&lock);
        usleep(SAMPLE_SLEEP_US);
    }

    return NULL;
}

/* --- public API --- */

void LightSampler_init(void)
{
    ADC_init();
    pthread_mutex_lock(&lock);
    running = 1;
    curr_len = 0;
    hist_len = 0;
    total_samples = 0;
    avg_initialized = 0;
    pthread_create(&sampler_thread, NULL, sampler_thread_func, NULL);
    pthread_mutex_unlock(&lock);
}

void LightSampler_cleanup(void)
{
    running = 0;
    pthread_join(sampler_thread, NULL);
    ADC_cleanup();

    pthread_mutex_lock(&lock);
    free(curr_buf); curr_buf = NULL; curr_len = curr_cap = 0;
    free(hist_buf); hist_buf = NULL; hist_len = 0;
    pthread_mutex_unlock(&lock);

    Period_cleanup();
}

void LightSampler_moveCurrentDataToHistory(void)
{
    Period_markEvent(PERIOD_EVENT_MARK_SECOND);

    pthread_mutex_lock(&lock);
    free(hist_buf);
    hist_buf = NULL;
    hist_len = 0;

    if (curr_len > 0) {
        hist_buf = malloc(sizeof(double) * curr_len);
        memcpy(hist_buf, curr_buf, sizeof(double) * curr_len);
        hist_len = curr_len;
    }
    curr_len = 0;

    // ================== FIX ==================
    // The original code had compilation errors here.
    // 1. Declare the struct defined in periodTimer.h
    Period_statistics_t ps; 
    
    // 2. Pass a pointer to the void function
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &ps); 
    
    // 3. Use the correct member names from the struct
    stat_min_ms = ps.minPeriodInMs;
    stat_max_ms = ps.maxPeriodInMs;
    stat_avg_ms = ps.avgPeriodInMs;
    stat_count = ps.numSamples;
    // =============== END FIX ===============

    pthread_mutex_unlock(&lock);
}

double *LightSampler_getHistory(int *size)
{
    pthread_mutex_lock(&lock);
    int n = hist_len;
    *size = n;
    double *copy = NULL;
    if (n > 0) {
        copy = malloc(sizeof(double) * n);
        memcpy(copy, hist_buf, sizeof(double) * n);
    }
    pthread_mutex_unlock(&lock);
    return copy;
}

double LightSampler_getAverage(void)
{
    pthread_mutex_lock(&lock);
    double v = avg_val;
    pthread_mutex_unlock(&lock);
    return v;
}

long long LightSampler_getTotalSamples(void)
{
    pthread_mutex_lock(&lock);
    long long t = total_samples;
    pthread_mutex_unlock(&lock);
    return t;
}

double LightSampler_getMinIntervalMs(void) { return stat_min_ms; }
double LightSampler_getMaxIntervalMs(void) { return stat_max_ms; }
double LightSampler_getAvgIntervalMs(void) { return stat_avg_ms; }
int    LightSampler_getNumIntervals(void)  { return stat_count; }
