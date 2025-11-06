#pragma once
/**
 * light_sampler.h
 *
 * Light Sampler Module
 * --------------------
 * Handles continuous background sampling of light intensity using the ADC.
 * A separate thread collects samples at ~1 kHz, stores them, and maintains
 * timing statistics using the periodTimer module.
 *
 * Once per second, the most recent second of samples is rotated into
 * a "history" buffer that other modules (UDP server, dip analyzer, etc.)
 * can access.
 *
 * Provides accessors for:
 *  - current average light level (exponential smoothing)
 *  - total samples collected
 *  - previous-second history buffer
 *  - timing statistics
 */

void LightSampler_init(void);
void LightSampler_cleanup(void);

/* Rotate current-second data into history (call once per second). */
void LightSampler_moveCurrentDataToHistory(void);

/* Returns malloc’d array of the previous second’s samples (caller frees). */
double *LightSampler_getHistory(int *size);

/* Returns average light level (volts). */
double LightSampler_getAverage(void);

/* Returns total samples collected since start. */
long long LightSampler_getTotalSamples(void);

/* Timing statistics (ms) for the last second of sampling. */
double LightSampler_getMinIntervalMs(void);
double LightSampler_getMaxIntervalMs(void);
double LightSampler_getAvgIntervalMs(void);
int    LightSampler_getNumIntervals(void);
