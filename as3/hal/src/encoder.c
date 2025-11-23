#include "hal/encoder.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Pins for BeagleY-AI
// A = Pin 11 (GPIO17), B = Pin 13 (GPIO27), Btn = Pin 15 (GPIO22)
#define LINE_A "GPIO17"
#define LINE_B "GPIO27"
#define LINE_BTN "GPIO22"

static struct gpiod_chip *chipA = NULL;
static struct gpiod_chip *chipB = NULL;
static struct gpiod_chip *chipBtn = NULL;
static struct gpiod_line *lineA = NULL;
static struct gpiod_line *lineB = NULL;
static struct gpiod_line *lineBtn = NULL;

static pthread_t encoderThreadId;
static int running = 0;
static int tickCount = 0; // Accumulated rotation ticks
static pthread_mutex_t countMutex = PTHREAD_MUTEX_INITIALIZER;

// Helper to configure a line as input
// Returns the line, and sets the chip pointer so we can close it later.
static struct gpiod_line* config_line(const char* name, struct gpiod_chip **chipOut) {
    struct gpiod_line *line = gpiod_line_find(name);
    if (!line) {
        printf("ERROR: Encoder line %s not found!\n", name);
        return NULL;
    }
    
    *chipOut = gpiod_line_get_chip(line);

    // Request as input
    if (gpiod_line_request_input(line, "beatbox_encoder") < 0) {
        printf("ERROR: Failed to request input for %s\n", name);
        return NULL;
    }
    return line;
}

static void* encoderThread(void* arg)
{
    int lastA = gpiod_line_get_value(lineA);
    
    while (running) {
        int currentA = gpiod_line_get_value(lineA);
        
        // Detect edge on A
        if (currentA != lastA) {
            // Simple Quadrature Logic:
            // If A changed, check B. 
            // If A == B, we moved one way. If A != B, we moved the other.
            int currentB = gpiod_line_get_value(lineB);
            
            pthread_mutex_lock(&countMutex);
            if (currentA == currentB) {
                tickCount++; // CW
            } else {
                tickCount--; // CCW
            }
            pthread_mutex_unlock(&countMutex);
            
            // Debounce delay
            usleep(2000); 
        }
        
        lastA = currentA;
        usleep(1000); // Poll rate ~1kHz
    }
    return NULL;
}

void Encoder_init(void)
{
    lineA = config_line(LINE_A, &chipA);
    lineB = config_line(LINE_B, &chipB);
    lineBtn = config_line(LINE_BTN, &chipBtn);

    if (!lineA || !lineB || !lineBtn) {
        printf("ERROR: Encoder init failed (pins not found).\n");
        exit(1);
    }

    running = 1;
    pthread_create(&encoderThreadId, NULL, encoderThread, NULL);
}

void Encoder_cleanup(void)
{
    running = 0;
    pthread_join(encoderThreadId, NULL);

    if (lineA) gpiod_line_release(lineA);
    if (lineB) gpiod_line_release(lineB);
    if (lineBtn) gpiod_line_release(lineBtn);
    
    // Close chips if they were opened by gpiod_line_find
    if (chipA) gpiod_chip_close(chipA);
    // Note: If multiple lines share a chip, we should be careful closing.
    // However, libgpiod handles this fairly gracefully usually, or we can just skip closing
    // since the OS cleans up on exit.
}

int Encoder_getTickCount(void)
{
    pthread_mutex_lock(&countMutex);
    int ret = tickCount;
    tickCount = 0; // Reset after reading so we get "delta" since last check
    pthread_mutex_unlock(&countMutex);
    return ret;
}

bool Encoder_isPressed(void)
{
    if (!lineBtn) return false;
    int val = gpiod_line_get_value(lineBtn);
    return (val == 0); // Active Low: 0 = Pressed
}