#include "dipAnalyzer.h"

int DipAnalyzer_countDips(const double *samples, int n, double avg_v)
{
    if (!samples || n <= 0)
        return 0;

    // Hysteresis thresholds from assignment
    const double TRIGGER_DROP = 0.10;   // Trigger dip when voltage < (avg - 0.10)
    const double RESET_RISE   = 0.07;   // Reset when voltage > (avg - 0.07)

    const double trigger_level = avg_v - TRIGGER_DROP;
    const double reset_level   = avg_v - RESET_RISE;

    int dips = 0;
    int in_dip = 0; // State machine: 0 = not in a dip, 1 = in a dip

    for (int i = 0; i < n; i++) {
        double v = samples[i];
        
        if (!in_dip) {
            // Not in a dip, look for a trigger
            if (v <= trigger_level) {
                dips++;
                in_dip = 1; // Enter the "in_dip" state
            }
        } else {
            // In a dip, look for a reset
            if (v >= reset_level) {
                in_dip = 0; // Exit the "in_dip" state
            }
        }
    }
    return dips;
}