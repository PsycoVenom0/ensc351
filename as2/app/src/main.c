// app/src/main.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "light_sampler.h"
#include "dipAnalyzer.h"
#include "udpServer.h"
#include "hal/adc.h"
#include "hal/pwmLed.h"
#include "hal/encoder.h"

static volatile int keepRunning = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    keepRunning = 0;
}

int main(void)
{
    signal(SIGINT, handle_sigint);
    printf("============================================\n");
    printf(" Light Sampling and Dip Detection System\n");
    printf("============================================\n");
    printf("Starting modules...\n");

    ADC_init();
    PWM_init();
    Encoder_init();
    LightSampler_init();
    UDPServer_init();

    printf("Running. Press Ctrl+C or send 'stop' command via UDP to exit.\n\n");

    while (keepRunning && UDPServer_isRunning()) {
        sleep(1);

        // Move latest samples to history
        LightSampler_moveCurrentDataToHistory();

        int n = 0;
        double *data = LightSampler_getHistory(&n);
        double avg = LightSampler_getAverage();
        int dips = DipAnalyzer_countDips(data, n, avg);
        int freq = Encoder_get_frequency();

        double minT = LightSampler_getMinIntervalMs();
        double maxT = LightSampler_getMaxIntervalMs();
        double avgT = LightSampler_getAvgIntervalMs();
        int countT  = LightSampler_getNumIntervals();

        printf("# Smpl/s=%4d  Flash@%3dHz  Avg=%.3fV  Dips=%3d  "
               "Δt[min=%.3fms max=%.3fms avg=%.3fms n=%d]\n",
               n, freq, avg, dips, minT, maxT, avgT, countT);

        // Print 10 evenly spaced samples from history
        if (n > 0) {
            int show = (n < 10) ? n : 10;
            for (int i = 0; i < show; i++) {
                int idx = (int)((double)i * (double)(n - 1) / (double)(show - 1) + 0.5);
                printf("%4d:%.3f ", idx, data[idx]);
            }
            printf("\n");
        }

        free(data);
    }

    printf("\nShutting down...\n");
    UDPServer_cleanup();
    Encoder_cleanup();
    PWM_cleanup();
    ADC_cleanup();
    LightSampler_cleanup();
    printf("Goodbye!\n");
    return 0;
}
