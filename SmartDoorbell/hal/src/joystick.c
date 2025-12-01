#define _GNU_SOURCE
#include "hal/joystick.h"
#include "hal/spi_adc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

static int spi_fd = -1;
static uint32_t spi_speed = 250000;

static int x_center = 2048;
static int y_center = 2048;

//helper to get ms 
static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int hal_joystick_init(const char *spi_device, uint32_t spi_speed_hz)
{
    spi_fd = hal_spi_adc_open(spi_device, spi_speed_hz);
    if (spi_fd < 0) return -1;
    spi_speed = spi_speed_hz;

    //auto-calibration: joystick is at rest/center 
    int samples = 20;
    long long start = now_ms();
    long long timeout = start + 1000; 
    long sum_x = 0, sum_y = 0;
    int cnt = 0;
    for (int i = 0; i < samples && now_ms() < timeout; ++i) {
        int xv = hal_spi_adc_read_ch(spi_fd, 6, spi_speed);
        int yv = hal_spi_adc_read_ch(spi_fd, 7, spi_speed);
        if (xv >= 0 && yv >= 0) {
            sum_x += xv;
            sum_y += yv;
            ++cnt;
        }
        struct timespec req = {0, 20000000}; //
        nanosleep(&req, NULL);
    }
    if (cnt > 0) {
        x_center = (int)(sum_x / cnt);
        y_center = (int)(sum_y / cnt);
    } else {
        x_center = 2048; y_center = 2048;
    }

    return 0;
}

void hal_joystick_cleanup(void) 
{
    hal_spi_adc_close(spi_fd);
    spi_fd = -1;
}

//read raw ADCs
int hal_joystick_read_raw(int *x_out, int *y_out)
{
    if (spi_fd < 0) return -1;
    int xv = hal_spi_adc_read_ch(spi_fd, 6, spi_speed);
    int yv = hal_spi_adc_read_ch(spi_fd, 7, spi_speed);
    if (xv < 0 || yv < 0) return -1;
    if (x_out) *x_out = xv;
    if (y_out) *y_out = yv;
    return 0;
}

//map raw to direction
joystick_dir_t hal_joystick_read_direction(void)
{
 int xv, yv;
    hal_joystick_read_raw(&xv, &yv);

    //center values (roughly)
    const int x_center = 2048;
    const int y_center = 2047;

    //threshold
    const int threshold = 1000;  //can change doesn't really matter

    int dx = xv - x_center;
    int dy = yv - y_center;

    int absdx = dx < 0 ? -dx : dx;
    int absdy = dy < 0 ? -dy : dy;

    if (absdy > threshold && absdy >= absdx) {
        if (dy > 0) return JOY_UP;     //pushing up CH1 value go up
        else return JOY_DOWN;          //pushing down decreases CH1
    } else if (absdx > threshold && absdx > absdy) {
        if (dx > 0) return JOY_RIGHT;  //pushing right CH0 value go up
        else return JOY_LEFT;           //pushing left CH0 value to down
    }

    return JOY_NONE;  //joystick is centered
}

//wait until joystick releases
void hal_joystick_wait_until_released(void)
{

    for (;;) {
        joystick_dir_t d = hal_joystick_read_direction();
        if (d == JOY_NONE) return;
        struct timespec req = {0, 50000000};
        nanosleep(&req, NULL);
    }
}

