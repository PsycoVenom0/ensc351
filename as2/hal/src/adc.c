#include "hal/adc.h"
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>

#define SPIDEV_DEVICE "/dev/spidev0.0"
#define ADC_CHANNEL 0
#define ADC_REF_V 3.3

static int spi_fd = -1;
static uint32_t spi_speed = 1000000;
static uint8_t spi_mode = 0;
static uint8_t spi_bits = 8;

void ADC_init(void) {
    spi_fd = open(SPIDEV_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("hal_adc: open spidev");
        return;
    }
    ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
}

void ADC_cleanup(void) {
    if (spi_fd >= 0) close(spi_fd);
    spi_fd = -1;
}

static int read_adc_channel(int ch) {
    if (spi_fd < 0) return -1;
    uint8_t tx[3] = {0};
    uint8_t rx[3] = {0};

    tx[0] = 0x06 | ((ch & 0x04) >> 2);
    tx[1] = (ch & 0x03) << 6;
    tx[2] = 0x00;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = spi_speed,
        .bits_per_word = spi_bits,
    };
    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

double ADC_read_voltage(void) {
    int raw = read_adc_channel(ADC_CHANNEL);
    if (raw < 0) return 0.0;
    return ((double)raw * ADC_REF_V) / 4095.0;
}
