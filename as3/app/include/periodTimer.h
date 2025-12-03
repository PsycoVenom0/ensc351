#ifndef _PERIOD_TIMER_H_
#define _PERIOD_TIMER_H_

#define MAX_EVENT_TIMESTAMPS (1024*4)

enum Period_whichEvent {
    PERIOD_EVENT_AUDIO_BUFFER, // Tracks audio buffer fill rate
    PERIOD_EVENT_ACCEL_READ,   // Tracks accelerometer polling rate
    NUM_PERIOD_EVENTS
};

typedef struct {
    int numSamples;
    double minPeriodInMs;
    double maxPeriodInMs;
    double avgPeriodInMs;
} Period_statistics_t;

void Period_init(void);
void Period_cleanup(void);
void Period_markEvent(enum Period_whichEvent whichEvent);
void Period_getStatisticsAndClear(enum Period_whichEvent whichEvent, Period_statistics_t *pStats);

#endif