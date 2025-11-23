#ifndef ADC_H_
#define ADC_H_

// Initialize SPI connection
void ADC_init(void);

// Close SPI connection
void ADC_cleanup(void);

// Read raw 12-bit value (0-4095) from specific channel (0-7)
// Returns -1 on error
int ADC_read_raw(int channel);

#endif