#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "hal/led.h"
#include "hal/joystick.h"

static long long getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
    return milliSeconds;
}

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

int main(void)
{
    printf("Hello embedded world, from Palash!\n\n");
    printf("When the LEDs light up, press the joystick in that direction!\n");
    printf("(Press left or right to exit)\n");

    srand(time(NULL));
    LED_init();
    Joystick_init();

    long long bestTime = 999999;

    while (true) {
        printf("\nGet ready...\n");
        for (int i = 0; i < 4; i++) {
            LED_greenOn(); sleepForMs(250);
            LED_greenOff();
            LED_redOn(); sleepForMs(250);
            LED_redOff();
        }

        // Wait until joystick released
        if (Joystick_read() != JS_NONE) {
            printf("Please let go of joystick...\n");
            while (Joystick_read() != JS_NONE) sleepForMs(100);
        }

        // Random delay
        long long delay = 500 + rand() % 2500;
        sleepForMs(delay);

        JoystickDirection dir = Joystick_read();

        if (dir == JS_LEFT || dir == JS_RIGHT) {
            printf("User selected to quit.\n");
            goto END;
        } else if (dir == JS_UP || dir == JS_DOWN) {
            printf("Too soon!\n");
            continue;
        }

        bool isUp = rand() % 2;
        if (isUp) {
            printf("Press UP now!\n");
            LED_greenOn();
        } else {
            printf("Press DOWN now!\n");
            LED_redOn();
        }

        long long start = getTimeInMs();
        while (true) {
            dir = Joystick_read();
            if (dir != JS_NONE) break;
            if (getTimeInMs() - start > 5000) {
                printf("No input within 5000ms; quitting!\n");
                goto END;
            }
            sleepForMs(10);
        }

        long long elapsed = getTimeInMs() - start;
        LED_greenOff();
        LED_redOff();

        if (dir == JS_LEFT || dir == JS_RIGHT) {
            printf("User selected to quit.\n");
            break;
        }

        bool correct = (isUp && dir == JS_UP) || (!isUp && dir == JS_DOWN);
        if (correct) {
            printf("Correct!\n");
            if (elapsed < bestTime) {
                bestTime = elapsed;
                printf("New best time!\n");
            }
            printf("Your reaction time was %lld ms; best so far is %lld ms.\n",
                   elapsed, bestTime);
            LED_flashGreen(5, 1000);
        } else {
            printf("Incorrect.\n");
            LED_flashRed(5, 1000);
        }
    }

END:
    LED_cleanup();
    Joystick_cleanup();
    return 0;
}
