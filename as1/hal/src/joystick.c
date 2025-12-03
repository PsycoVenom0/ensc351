#include "hal/joystick.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>

#define SPI_DEVICE "/dev/spidev0.0"
#define X_CH 0
#define Y_CH 1
static uint32_t speed = 250000;
static int fd = -1;

// Thresholds
#define CENTER 2048
#define DEADZONE 500   // ±500 counts around center
#define UPPER_LIMIT (CENTER + DEADZONE)
#define LOWER_LIMIT (CENTER - DEADZONE)

// Helper: read 12-bit value from MCP3208 channel
static int read_ch(int ch)
{
    uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
                      (uint8_t)((ch & 0x03) << 6),
                      0x00 };
    uint8_t rx[3] = { 0 };

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1)
        return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

void Joystick_init(void)
{
    fd = open(SPI_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    uint8_t mode = 0, bits = 8;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

JoystickDirection Joystick_read(void)
{
    int x = read_ch(X_CH);
    int y = read_ch(Y_CH);

    if (x == -1 || y == -1)
        return JS_NONE;

    // Center dead-zone check
    if (x > LOWER_LIMIT && x < UPPER_LIMIT && y > LOWER_LIMIT && y < UPPER_LIMIT)
        return JS_NONE;

    // Direction detection
    if (x < LOWER_LIMIT) return JS_LEFT;
    if (x > UPPER_LIMIT) return JS_RIGHT;
    if (y > LOWER_LIMIT) return JS_UP;
    if (y < UPPER_LIMIT) return JS_DOWN;

    return JS_NONE;
}

void Joystick_cleanup(void)
{
    if (fd >= 0)
        close(fd);
}

