#include "hal/adc.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// SPI Configuration
#define SPI_DEVICE "/dev/spidev0.0"

static int spi_fd = -1;
static uint32_t speed = 1000000; // 1MHz

void ADC_init(void) {
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("ADC_init: Failed to open SPI device");
        exit(1);
    }
    
    // Configure SPI mode and bits
    uint8_t mode = 0;
    uint8_t bits = 8;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) perror("ADC Init: Set Mode");
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) perror("ADC Init: Set Bits");
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) perror("ADC Init: Set Speed");
}

void ADC_cleanup(void) {
    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
    }
}

int ADC_read_raw(int channel) {
    if (spi_fd < 0 || channel < 0 || channel > 7) return -1;

    // MCP3208 Protocol: Start Bit + SGL/DIFF + D2 + D1 + D0
    // Byte 0: 0000 01(Start) (SGL/DIFF) (D2)
    // Byte 1: (D1) (D0) ...
    
    uint8_t tx[3] = {0};
    tx[0] = 0x06 | ((channel & 0x04) >> 2);
    tx[1] = (uint8_t)((channel & 0x03) << 6);
    tx[2] = 0x00;

    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed,
        .bits_per_word = 8,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("ADC_read_raw: ioctl failed");
        return -1;
    }

    // Combine the 12 bits (Last 4 bits of rx[1] + all of rx[2])
    return ((rx[1] & 0x0F) << 8) | rx[2];
}