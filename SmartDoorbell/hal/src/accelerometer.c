#include "hal/accelerometer.h"
#include "hal/adc.h"

// --- WIRING MAPPING ---
// Accel X -> Pin 7 -> Ch 6
// Accel Y -> Pin 6 -> Ch 5
// Accel Z -> Pin 5 -> Ch 4
#define ACC_X_CH  2
#define ACC_Y_CH  1
#define ACC_Z_CH  0

void Accel_init(void) {
    // Hardware is handled by ADC_init()
}

void Accel_readXYZ(int* x, int* y, int* z) {
    if (x) *x = ADC_read_raw(ACC_X_CH);
    if (y) *y = ADC_read_raw(ACC_Y_CH);
    if (z) *z = ADC_read_raw(ACC_Z_CH);
}

void Accel_cleanup(void) {
    // Nothing to clean up here
}