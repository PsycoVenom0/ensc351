#define _GNU_SOURCE
#include "hal/adc.h"
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>

/* ===== Configuration ===== */
#ifndef SPIDEV_DEVICE
#define SPIDEV_DEVICE "/dev/spidev0.0"
#endif
#ifndef ADC_CHANNEL
#define ADC_CHANNEL 0
#endif
#ifndef ADC_REF_V
#define ADC_REF_V 3.3
#endif
#ifndef SPI_SPEED_HZ
#define SPI_SPEED_HZ 1000000U   /* 1 MHz default */
#endif

static int spi_fd = -1;
static const uint8_t spi_mode = 0;
static const uint8_t spi_bits = 8;
static const uint32_t spi_speed = SPI_SPEED_HZ;

/* ===== Implementation ===== */

void ADC_init(void) {
    spi_fd = open(SPIDEV_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("ADC_init: open spidev");
        spi_fd = -1;
        return;
    }
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode) < 0)
        perror("ADC_init: SPI_IOC_WR_MODE");
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits) < 0)
        perror("ADC_init: SPI_IOC_WR_BITS_PER_WORD");
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0)
        perror("ADC_init: SPI_IOC_WR_MAX_SPEED_HZ");
}

void ADC_cleanup(void) {
    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
    }
}

/* Perform single-ended read for channel ch (0..7). */
static int ADC_read_raw(int ch) {
    if (spi_fd < 0) return -1;
    if (ch < 0 || ch > 7) return -1;

    uint8_t tx[3] = {0}, rx[3] = {0};
    tx[0] = 0x06 | ((ch & 0x04) >> 2);
    tx[1] = (uint8_t)((ch & 0x03) << 6);

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = spi_speed,
        .bits_per_word = spi_bits,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1)
        return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2];  /* 12-bit value */
}

double ADC_read_voltage(void) {
    int raw = ADC_read_raw(ADC_CHANNEL);
    if (raw < 0) {
        /* fallback simulated voltage for testing */
        static double t = 0.0;
        t += 0.01;
        if (t > ADC_REF_V) t = 0.0;
        return t;
    }
    return ((double)raw) * (ADC_REF_V / 4095.0);
}
