#include "hal/led.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GREEN_TRIGGER "/sys/class/leds/ACT/trigger"
#define GREEN_BRIGHT  "/sys/class/leds/ACT/brightness"
#define RED_TRIGGER   "/sys/class/leds/PWR/trigger"
#define RED_BRIGHT    "/sys/class/leds/PWR/brightness"

static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;

    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;

    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

static void writeToFile(const char *fileName, const char *value)
{
    FILE *pFile = fopen(fileName, "w");
    if (pFile == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fprintf(pFile, "%s", value);
    fclose(pFile);
}

void LED_init(void)
{
    writeToFile(GREEN_TRIGGER, "none");
    writeToFile(RED_TRIGGER, "none");
    LED_greenOff();
    LED_redOff();
}

void LED_greenOn(void) { writeToFile(GREEN_BRIGHT, "1"); }
void LED_greenOff(void) { writeToFile(GREEN_BRIGHT, "0"); }
void LED_redOn(void) { writeToFile(RED_BRIGHT, "1"); }
void LED_redOff(void) { writeToFile(RED_BRIGHT, "0"); }

void LED_flashGreen(int times, long long durationMs)
{
    long long delay = durationMs / (2 * times);
    for (int i = 0; i < times; i++) {
        LED_greenOn();  sleepForMs(delay);
        LED_greenOff(); sleepForMs(delay);
    }
}

void LED_flashRed(int times, long long durationMs)
{
    long long delay = durationMs / (2 * times);
    for (int i = 0; i < times; i++) {
        LED_redOn();  sleepForMs(delay);
        LED_redOff(); sleepForMs(delay);
    }
}

void LED_cleanup(void)
{
    LED_greenOff();
    LED_redOff();
}
