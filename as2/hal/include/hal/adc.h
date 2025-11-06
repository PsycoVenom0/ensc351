#pragma once
/**
 * adc.h
 *
 * Hardware Abstraction Layer for an SPI-connected ADC.
 * Provides a simple API to initialize, read, and clean up the ADC.
 *
 * All voltages are returned in volts (0.0 - ADC_REF_V).
 */

void ADC_init(void);
void ADC_cleanup(void);

/* Reads the configured ADC channel and returns voltage (in volts). */
double ADC_read_voltage(void);
