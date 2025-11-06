#pragma once

// Initialize the ADC hardware (MCP3208 via SPI)
void ADC_init(void);

// Clean up ADC resources
void ADC_cleanup(void);

// Read voltage from channel (scaled 0.0–3.3 V)
double ADC_read_voltage(void);

